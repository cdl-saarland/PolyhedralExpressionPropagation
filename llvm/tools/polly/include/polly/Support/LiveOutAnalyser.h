//===--- LiveOutAnalyser.h - Perform Live Out Analysis-----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Checks if a Value is Live out after a region.
//===----------------------------------------------------------------------===//

#ifndef POLLY_LIVE_OUT_ANALYSER_H
#define POLLY_LIVE_OUT_ANALYSER_H

#include "polly/Support/ScopHelper.h"
#include "llvm/ADT/SetVector.h"
#include "polly/ScopInfo.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"

namespace llvm {
class Region;
class Function;
class Value;
class Instruction;
} // namespace llvm

namespace polly {
  class MemoryAccess;
  //bool isLiveOutScalar(Instruction *Inst, Scop &S, bool Recursive);
  bool isLiveOut(Scop &S, Value *BasePtr, TargetLibraryInfo *TLI = nullptr);
  bool isLiveOut(MemoryAccess &MA, TargetLibraryInfo *TLI = nullptr);
} // namespace polly

#endif
