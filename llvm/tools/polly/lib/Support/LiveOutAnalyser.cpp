#include "polly/Support/LiveOutAnalyser.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "polly-live-out"

STATISTIC(DETECTED_NON_LIVE_OUT_ACCESSES, "Number of non live out accesses detected.");

using namespace llvm;
namespace polly {

  bool isLiveOutScalar(Instruction *Inst, Scop &S) {
    for(auto User : Inst->users()){
      DEBUG(errs()<< "\nUser = "; User->dump(); errs() << "\n";);
      if (auto *UserInst = dyn_cast<Instruction>(User)) {
        if(isa<CastInst>(UserInst))
          return !S.contains(UserInst) || isLiveOutScalar(UserInst, S);
        return !S.contains(UserInst);
      }
    }
    return false;
  }

  bool isLiveOutPointer(Value *BasePtr, Scop &S, TargetLibraryInfo *TLI){

    auto *ExitBlock = S.getExit();
    auto *ScopFunction = ExitBlock->getParent() ;
    SmallPtrSet<BasicBlock *, 8> SuccBlocks;
    SmallPtrSet<BasicBlock *, 8> Worklist;
    Worklist.insert(ExitBlock);
    while(!Worklist.empty()) {
      auto *CurrBlock = *Worklist.begin();
      Worklist.erase(CurrBlock);
      if (!SuccBlocks.insert(CurrBlock).second)
        continue;
      for(auto *BB : successors(CurrBlock))
        if(ScopFunction == BB->getParent())
          Worklist.insert(BB);
    }
    DEBUG(errs()<< "\nBasePtr = "; BasePtr->dump(); errs() << "\n";);
    SmallPtrSet<User *, 32> UserWorklist;
    for(auto User : BasePtr->users())
      UserWorklist.insert(User);
    while(!UserWorklist.empty()){
      auto *CurrUser = *UserWorklist.begin();
      UserWorklist.erase(CurrUser);
      auto *Inst = dyn_cast<Instruction>(CurrUser);

      DEBUG(errs()<< "\nCurrUser = "; CurrUser->dump(); errs() << "\n";);
      if(Inst && SuccBlocks.count(Inst->getParent())) {
        // Inst exists in one of the succeeding basic blocks

        // If Inst is an allocation function or a call to free, we can safely
        // conclude that the memory location is not live out after it. However
        // since we don't have an ordering of the Instructions and Basic Blocks
        // we just rule out the current Inst as not live out
        if(isAllocationFn(Inst, TLI) || isFreeCall(Inst, TLI))
          continue;
        // In this case if there are reads from the memory locations or if
        // there are side effects we know that the memory location is live out.
        // If not, we still need to check the users of Inst in case there is
        // any aliasing going on.
        if(!(Inst->mayReadFromMemory() || Inst->mayHaveSideEffects())) {
          DEBUG(errs()<< "\nAdding Users of:"; Inst->dump(); errs() << "\n";);
          for(auto User : Inst->users())
            UserWorklist.insert(User);
          continue;
        }
        DEBUG(errs()<< "\nLive Out User = "; Inst->dump(); errs() << "\n";);
        return true;
      }
      else {

        if (Inst) {
          // Inst doesn't belong to succeeding basic blocks
          // In this case we want to check for aliasing and casts
          // We can rule out read instructions since they
          // don't have any aliasing going on.
          if (isa<LoadInst>(Inst))
            continue;

          if (isa<CallInst>(Inst))
            return true;
        }

        // For all other instructions we assume that they could have
        // some form of aliasing or casting involved. So we add all
        // their users to the worklist and check them for liveness
        DEBUG(errs()<< "\nAdding Users of:"; CurrUser->dump(); errs() << "\n";);
        for(auto User : CurrUser->users())
          UserWorklist.insert(User);
      }
    }

    return false;
  }

  bool isLiveOut(MemoryAccess &MA, TargetLibraryInfo *TLI) {
    if(MA.isPHIKind())
      return false;
    //if(MA.isScalarKind()) {
      //auto AccInst = MA.getAccessInstruction();
      //return isLiveOutScalar(AccInst, *S);
    //}

    Scop *S = MA.getStatement()->getParent();
    auto *BasePtr = MA.getBaseAddr();
    return isLiveOut(*S, BasePtr, TLI);
  }

  bool isLiveOut(Scop &S, Value *BasePtr, TargetLibraryInfo *TLI) {
    if (auto *GV = dyn_cast<GlobalValue>(BasePtr)) {
      GV->dump();
      errs() << GV->isInternalLinkage(GV->getLinkage()) << "\n";
      if (!GV->isInternalLinkage(GV->getLinkage()))
        return true;
    }
    if (isa<Argument>(BasePtr))
      return true;

    //auto *Alloca = dyn_cast<AllocaInst>(BasePtr);
    bool liveOut = false;
    //if (!Alloca) {
      //if(!isa<CallInst>(BasePtr))return true;
      liveOut = isLiveOutPointer(BasePtr, S, TLI);
    //}
    //else
      //liveOut = isLiveOutScalar(Alloca, *S);
    if(!liveOut) DETECTED_NON_LIVE_OUT_ACCESSES++;
    return liveOut;
  }
}
