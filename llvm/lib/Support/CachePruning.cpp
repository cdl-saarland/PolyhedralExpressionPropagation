//===-CachePruning.cpp - LLVM Cache Directory Pruning ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the pruning of a directory based on least recently used.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/CachePruning.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "cache-pruning"

#include <set>
#include <system_error>

using namespace llvm;

/// Write a new timestamp file with the given path. This is used for the pruning
/// interval option.
static void writeTimestampFile(StringRef TimestampFile) {
  std::error_code EC;
  raw_fd_ostream Out(TimestampFile.str(), EC, sys::fs::F_None);
}

static Expected<std::chrono::seconds> parseDuration(StringRef Duration) {
  if (Duration.empty())
    return make_error<StringError>("Duration must not be empty",
                                   inconvertibleErrorCode());

  StringRef NumStr = Duration.slice(0, Duration.size()-1);
  uint64_t Num;
  if (NumStr.getAsInteger(0, Num))
    return make_error<StringError>("'" + NumStr + "' not an integer",
                                   inconvertibleErrorCode());

  switch (Duration.back()) {
  case 's':
    return std::chrono::seconds(Num);
  case 'm':
    return std::chrono::minutes(Num);
  case 'h':
    return std::chrono::hours(Num);
  default:
    return make_error<StringError>("'" + Duration +
                                       "' must end with one of 's', 'm' or 'h'",
                                   inconvertibleErrorCode());
  }
}

Expected<CachePruningPolicy>
llvm::parseCachePruningPolicy(StringRef PolicyStr) {
  CachePruningPolicy Policy;
  std::pair<StringRef, StringRef> P = {"", PolicyStr};
  while (!P.second.empty()) {
    P = P.second.split(':');

    StringRef Key, Value;
    std::tie(Key, Value) = P.first.split('=');
    if (Key == "prune_interval") {
      auto DurationOrErr = parseDuration(Value);
      if (!DurationOrErr)
        return DurationOrErr.takeError();
      Policy.Interval = *DurationOrErr;
    } else if (Key == "prune_after") {
      auto DurationOrErr = parseDuration(Value);
      if (!DurationOrErr)
        return DurationOrErr.takeError();
      Policy.Expiration = *DurationOrErr;
    } else if (Key == "cache_size") {
      if (Value.back() != '%')
        return make_error<StringError>("'" + Value + "' must be a percentage",
                                       inconvertibleErrorCode());
      StringRef SizeStr = Value.slice(0, Value.size() - 1);
      uint64_t Size;
      if (SizeStr.getAsInteger(0, Size))
        return make_error<StringError>("'" + SizeStr + "' not an integer",
                                       inconvertibleErrorCode());
      if (Size > 100)
        return make_error<StringError>("'" + SizeStr +
                                           "' must be between 0 and 100",
                                       inconvertibleErrorCode());
      Policy.PercentageOfAvailableSpace = Size;
    } else {
      return make_error<StringError>("Unknown key: '" + Key + "'",
                                     inconvertibleErrorCode());
    }
  }

  return Policy;
}

/// Prune the cache of files that haven't been accessed in a long time.
bool llvm::pruneCache(StringRef Path, CachePruningPolicy Policy) {
  using namespace std::chrono;

  if (Path.empty())
    return false;

  bool isPathDir;
  if (sys::fs::is_directory(Path, isPathDir))
    return false;

  if (!isPathDir)
    return false;

  Policy.PercentageOfAvailableSpace =
      std::min(Policy.PercentageOfAvailableSpace, 100u);

  if (Policy.Expiration == seconds(0) &&
      Policy.PercentageOfAvailableSpace == 0) {
    DEBUG(dbgs() << "No pruning settings set, exit early\n");
    // Nothing will be pruned, early exit
    return false;
  }

  // Try to stat() the timestamp file.
  SmallString<128> TimestampFile(Path);
  sys::path::append(TimestampFile, "llvmcache.timestamp");
  sys::fs::file_status FileStatus;
  const auto CurrentTime = system_clock::now();
  if (auto EC = sys::fs::status(TimestampFile, FileStatus)) {
    if (EC == errc::no_such_file_or_directory) {
      // If the timestamp file wasn't there, create one now.
      writeTimestampFile(TimestampFile);
    } else {
      // Unknown error?
      return false;
    }
  } else {
    if (Policy.Interval == seconds(0)) {
      // Check whether the time stamp is older than our pruning interval.
      // If not, do nothing.
      const auto TimeStampModTime = FileStatus.getLastModificationTime();
      auto TimeStampAge = CurrentTime - TimeStampModTime;
      if (TimeStampAge <= Policy.Interval) {
        DEBUG(dbgs() << "Timestamp file too recent ("
                     << duration_cast<seconds>(TimeStampAge).count()
                     << "s old), do not prune.\n");
        return false;
      }
    }
    // Write a new timestamp file so that nobody else attempts to prune.
    // There is a benign race condition here, if two processes happen to
    // notice at the same time that the timestamp is out-of-date.
    writeTimestampFile(TimestampFile);
  }

  bool ShouldComputeSize = (Policy.PercentageOfAvailableSpace > 0);

  // Keep track of space
  std::set<std::pair<uint64_t, std::string>> FileSizes;
  uint64_t TotalSize = 0;
  // Helper to add a path to the set of files to consider for size-based
  // pruning, sorted by size.
  auto AddToFileListForSizePruning =
      [&](StringRef Path) {
        if (!ShouldComputeSize)
          return;
        TotalSize += FileStatus.getSize();
        FileSizes.insert(
            std::make_pair(FileStatus.getSize(), std::string(Path)));
      };

  // Walk the entire directory cache, looking for unused files.
  std::error_code EC;
  SmallString<128> CachePathNative;
  sys::path::native(Path, CachePathNative);
  // Walk all of the files within this directory.
  for (sys::fs::directory_iterator File(CachePathNative, EC), FileEnd;
       File != FileEnd && !EC; File.increment(EC)) {
    // Ignore any files not beginning with the string "llvmcache-". This
    // includes the timestamp file as well as any files created by the user.
    // This acts as a safeguard against data loss if the user specifies the
    // wrong directory as their cache directory.
    if (!sys::path::filename(File->path()).startswith("llvmcache-"))
      continue;

    // Look at this file. If we can't stat it, there's nothing interesting
    // there.
    if (sys::fs::status(File->path(), FileStatus)) {
      DEBUG(dbgs() << "Ignore " << File->path() << " (can't stat)\n");
      continue;
    }

    // If the file hasn't been used recently enough, delete it
    const auto FileAccessTime = FileStatus.getLastAccessedTime();
    auto FileAge = CurrentTime - FileAccessTime;
    if (FileAge > Policy.Expiration) {
      DEBUG(dbgs() << "Remove " << File->path() << " ("
                   << duration_cast<seconds>(FileAge).count() << "s old)\n");
      sys::fs::remove(File->path());
      continue;
    }

    // Leave it here for now, but add it to the list of size-based pruning.
    AddToFileListForSizePruning(File->path());
  }

  // Prune for size now if needed
  if (ShouldComputeSize) {
    auto ErrOrSpaceInfo = sys::fs::disk_space(Path);
    if (!ErrOrSpaceInfo) {
      report_fatal_error("Can't get available size");
    }
    sys::fs::space_info SpaceInfo = ErrOrSpaceInfo.get();
    auto AvailableSpace = TotalSize + SpaceInfo.free;
    auto FileAndSize = FileSizes.rbegin();
    DEBUG(dbgs() << "Occupancy: " << ((100 * TotalSize) / AvailableSpace)
                 << "% target is: " << Policy.PercentageOfAvailableSpace
                 << "\n");
    // Remove the oldest accessed files first, till we get below the threshold
    while (((100 * TotalSize) / AvailableSpace) >
               Policy.PercentageOfAvailableSpace &&
           FileAndSize != FileSizes.rend()) {
      // Remove the file.
      sys::fs::remove(FileAndSize->second);
      // Update size
      TotalSize -= FileAndSize->first;
      DEBUG(dbgs() << " - Remove " << FileAndSize->second << " (size "
                   << FileAndSize->first << "), new occupancy is " << TotalSize
                   << "%\n");
      ++FileAndSize;
    }
  }
  return true;
}