//===---- CodePreparation.cpp - Code preparation for Scop Detection -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The Polly code preparation pass is executed before SCoP detection. Its
// currently only splits the entry block of the SCoP to make room for alloc
// instructions as they are generated during code generation.
//
// XXX: In the future, we should remove the need for this pass entirely and
// instead add this spitting to the code generation pass.
//
//===----------------------------------------------------------------------===//

#include "polly/LinkAllPasses.h"
#include "polly/ScopDetection.h"
#include "polly/Options.h"
#include "polly/Support/ScopHelper.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"

using namespace llvm;
using namespace polly;

#define DEBUG_TYPE "polly-detect"

namespace {

/// Prepare the IR for the scop detection.
///
class CodePreparation : public FunctionPass {
  CodePreparation(const CodePreparation &) = delete;
  const CodePreparation &operator=(const CodePreparation &) = delete;

  LoopInfo *LI;
  ScalarEvolution *SE;

  void clear();

public:
  static char ID;

  explicit CodePreparation() : FunctionPass(ID) {}
  ~CodePreparation();

  /// @name FunctionPass interface.
  //@{
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void releaseMemory();
  virtual bool runOnFunction(Function &F);
  virtual void print(raw_ostream &OS, const Module *) const;
  //@}
};
} // namespace

void CodePreparation::clear() {}

CodePreparation::~CodePreparation() { clear(); }

void CodePreparation::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<ScalarEvolutionWrapperPass>();

  AU.addPreserved<LoopInfoWrapperPass>();
  AU.addPreserved<RegionInfoPass>();
  AU.addPreserved<DominatorTreeWrapperPass>();
  AU.addPreserved<DominanceFrontierWrapperPass>();
}

static bool getOperands(PHINode *PHI, SmallPtrSetImpl<const SCEV *> &Ops, ScalarEvolution &SE, LoopInfo &LI) {
  if (LI.isLoopHeader(PHI->getParent()))
    return false;
  for (auto &Op : PHI->operands()) {
    Loop *Scope = isa<Instruction>(Op)
                      ? LI.getLoopFor(cast<Instruction>(Op)->getParent())
                      : nullptr;
    if (auto *LoadI = dyn_cast<LoadInst>(Op)) {
      Ops.insert(SE.getSCEVAtScope(LoadI->getPointerOperand(), Scope));
      continue;
    } else if (auto *PHIOp = dyn_cast<PHINode>(Op)) {
      if (!getOperands(PHIOp, Ops, SE, LI))
        return false;
      continue;
    } else if (SE.isSCEVable(Op->getType())) {
      Ops.insert(SE.getSCEVAtScope(Op, Scope));
      continue;
    }
    return false;
  }
  return true;
}

bool CodePreparation::runOnFunction(Function &F) {
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();

  splitEntryBlockForAlloca(&F.getEntryBlock(), this);

  bool OneReturn = false;
  TerminatorInst *Return = nullptr;
  BasicBlock *ReturnBB = nullptr;
  for (BasicBlock &BB : F)
    if (isa<ReturnInst>(BB.getTerminator())) {
      OneReturn = !ReturnBB;
      ReturnBB = &BB;
      Return = BB.getTerminator();
    }
  if (ReturnBB)
    SplitBlock(ReturnBB, Return, DT, LI);

  SmallVector<PHINode *, 32> PHIs;
  for (BasicBlock &BB : F) {
    SmallVector<CallInst *, 8> Calls;
    if (OneReturn && &BB != &F.getEntryBlock()) {
      for (Instruction &I : BB) {

        if (isa<PHINode>(I))
          PHIs.push_back(cast<PHINode>(&I));

        // Move malloc and free around.
        if (auto *CI = dyn_cast<CallInst>(&I))
          Calls.push_back(CI);

      }
    }
    for (CallInst *CI : Calls) {
      if (auto *CF = CI->getCalledFunction()) {
        if (CF->getName().equals("polybench_alloc_data") ||
            CF->getName().equals("malloc")) {
          if (DT->dominates(CI, ReturnBB))
            if (std::all_of(CI->arg_begin(), CI->arg_end(),
                            [](const Use &U) { return isa<Constant>(U); }))
              CI->moveBefore(F.getEntryBlock().getTerminator());
        } else if (CF->getName().equals("free"))
          CI->moveBefore(Return);
      }
    }

    //auto *L = LI->getLoopFor(&BB);
    //if (!L)
      //continue;
    //if (!L->getSubLoopsVector().empty())
      //continue;
    //auto TC = SE->getSmallConstantTripCount(L);
    //if (TC >= 16 || TC == 0)
      //continue;
    //auto &Ctx = SE->getContext();
    //auto *URMetadata =
        //MDNode::get(Ctx, {MDString::get(Ctx, "llvm.loop.unroll.enable")});
    //L->getLoopID();
  }

  SCEVExpander Expander(*SE, F.getParent()->getDataLayout(), "");
  for (PHINode *PHI : PHIs) {
    if (PHI->getNumIncomingValues() == 1) {
      DEBUG(errs() << "Replace " << *PHI << " with " << *PHI->getIncomingValue(0) << "\n");
      PHI->replaceAllUsesWith(PHI->getIncomingValue(0));
      PHI->eraseFromParent();
      continue;
    }
    SmallPtrSet<const SCEV *, 4> Ops;
    if (!getOperands(PHI, Ops, *SE, *LI))
      continue;
    if (Ops.size() != 1)
      continue;
    DEBUG(errs() << "Replace " << *PHI << " with " << *PHI->getIncomingValue(0) << "\n");
    Type *Ty = PHI->getType();
    if ((*Ops.begin())->getType() != PHI->getType())
      Ty = Ty->getPointerTo();

    auto *Recompute = Expander.expandCodeFor(
        *Ops.begin(), Ty, &*PHI->getParent()->getFirstInsertionPt());

    if (Ty != PHI->getType())
      Recompute = new LoadInst(Recompute, PHI->getName(), &*PHI->getParent()->getFirstInsertionPt());
    assert(Recompute->getType() == PHI->getType());
    PHI->replaceAllUsesWith(Recompute);
    PHI->eraseFromParent();
  }

  return false;
}

void CodePreparation::releaseMemory() { clear(); }

void CodePreparation::print(raw_ostream &OS, const Module *) const {}

char CodePreparation::ID = 0;
char &polly::CodePreparationID = CodePreparation::ID;

Pass *polly::createCodePreparationPass() { return new CodePreparation(); }

INITIALIZE_PASS_BEGIN(CodePreparation, "polly-prepare",
                      "Polly - Prepare code for polly", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_END(CodePreparation, "polly-prepare",
                    "Polly - Prepare code for polly", false, false)
