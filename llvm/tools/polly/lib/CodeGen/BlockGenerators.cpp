//===--- BlockGenerators.cpp - Generate code for statements -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the BlockGenerator and VectorBlockGenerator classes,
// which generate sequential code and vectorized code for a polyhedral
// statement, respectively.
//
//===----------------------------------------------------------------------===//

#include "polly/CodeGen/BlockGenerators.h"
#include "polly/CodeGen/CodeGeneration.h"
#include "polly/CodeGen/IslExprBuilder.h"
#include "polly/CodeGen/RuntimeDebugBuilder.h"
#include "polly/Options.h"
#include "polly/ScopInfo.h"
#include "polly/Support/GICHelper.h"
#include "polly/Support/SCEVValidator.h"
#include "polly/Support/ScopHelper.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include "isl/aff.h"
#include "isl/ast.h"
#include "isl/ast_build.h"
#include "isl/set.h"
#include <deque>

#define DEBUG_TYPE "polly-codegen-blocks"

using namespace llvm;
using namespace polly;

static cl::opt<bool> Aligned("enable-polly-aligned",
                             cl::desc("Assumed aligned memory accesses."),
                             cl::Hidden, cl::init(false), cl::ZeroOrMore,
                             cl::cat(PollyCategory));
static cl::opt<bool> PropagateLoads("polly-load-recurences", cl::desc(""),
                                    cl::Hidden, cl::init(true), cl::ZeroOrMore,
                                    cl::cat(PollyCategory));

bool PollyDebugPrinting;
static cl::opt<bool, true> DebugPrintingX(
    "polly-codegen-add-debug-printing",
    cl::desc("Add printf calls that show the values loaded/stored."),
    cl::location(PollyDebugPrinting), cl::Hidden, cl::init(false),
    cl::ZeroOrMore, cl::cat(PollyCategory));

BlockGenerator::BlockGenerator(PollyIRBuilder &B, LoopInfo &LI,
                               ScalarEvolution &SE, DominatorTree &DT,
                               AllocaMapTy &ScalarMap,
                               EscapeUsersAllocaMapTy &EscapeMap,
                               ValueMapT &GlobalMap,
                               IslExprBuilder *ExprBuilder,
                               BasicBlock *StartBlock, ScopAnnotator &Annotator)
    : Builder(B), Annotator(Annotator), LI(LI), SE(SE),
      ExprBuilder(ExprBuilder), DT(DT), EntryBB(nullptr), ScalarMap(ScalarMap),
      EscapeMap(EscapeMap), GlobalMap(GlobalMap), StartBlock(StartBlock) {}

BlockGenerator::~BlockGenerator() {
  clearMALoopMap();
}

void BlockGenerator::clearMALoopMap() {
  for (auto &SAIIt : MALoopMap)
    for (auto &It : SAIIt.second)
      isl_map_free(It.second.second);
  MALoopMap.clear();
}


Value *BlockGenerator::trySynthesizeNewValue(ScopStmt &Stmt, Value *Old,
                                             ValueMapT &BBMap,
                                             LoopToScevMapT &LTS,
                                             Loop *L) const {
  if (!SE.isSCEVable(Old->getType()))
    return nullptr;

  const SCEV *Scev = SE.getSCEVAtScope(Old, L);
  if (!Scev)
    return nullptr;

  if (isa<SCEVCouldNotCompute>(Scev))
    return nullptr;

  const SCEV *NewScev = SCEVLoopAddRecRewriter::rewrite(Scev, LTS, SE);
  ValueMapT VTV;
  VTV.insert(BBMap.begin(), BBMap.end());
  VTV.insert(GlobalMap.begin(), GlobalMap.end());

  Scop &S = *Stmt.getParent();
  const DataLayout &DL = S.getFunction().getParent()->getDataLayout();
  auto IP = Builder.GetInsertPoint();

  assert(IP != Builder.GetInsertBlock()->end() &&
         "Only instructions can be insert points for SCEVExpander");
  Value *Expanded =
      expandCodeFor(S, SE, DL, "polly", NewScev, Old->getType(), &*IP, &VTV,
                    StartBlock->getSinglePredecessor());

  BBMap[Old] = Expanded;
  return Expanded;
}

Value *BlockGenerator::getNewValue(ScopStmt &Stmt, Value *Old, ValueMapT &BBMap,
                                   LoopToScevMapT &LTS, Loop *L) const {
  // Constants that do not reference any named value can always remain
  // unchanged. Handle them early to avoid expensive map lookups. We do not take
  // the fast-path for external constants which are referenced through globals
  // as these may need to be rewritten when distributing code accross different
  // LLVM modules.
  if (isa<Constant>(Old) && !isa<GlobalValue>(Old))
    return Old;

  // Inline asm is like a constant to us.
  if (isa<InlineAsm>(Old))
    return Old;

  if (Value *New = GlobalMap.lookup(Old)) {
    if (Value *NewRemapped = GlobalMap.lookup(New))
      New = NewRemapped;
    if (Old->getType()->getScalarSizeInBits() <
        New->getType()->getScalarSizeInBits())
      New = Builder.CreateTruncOrBitCast(New, Old->getType());

    return New;
  }

  if (Value *New = BBMap.lookup(Old))
    return New;

  if (Value *New = trySynthesizeNewValue(Stmt, Old, BBMap, LTS, L))
    return New;

  // A scop-constant value defined by a global or a function parameter.
  if (isa<GlobalValue>(Old) || isa<Argument>(Old))
    return Old;

  // A scop-constant value defined by an instruction executed outside the scop.
  if (const Instruction *Inst = dyn_cast<Instruction>(Old))
    if (!Stmt.getParent()->contains(Inst->getParent()))
      return Old;

  // The scalar dependence is neither available nor SCEVCodegenable.
  llvm_unreachable("Unexpected scalar dependence in region!");
  return nullptr;
}

void BlockGenerator::copyInstScalar(ScopStmt &Stmt, Instruction *Inst,
                                    ValueMapT &BBMap, LoopToScevMapT &LTS) {
  // We do not generate debug intrinsics as we did not investigate how to
  // copy them correctly. At the current state, they just crash the code
  // generation as the meta-data operands are not correctly copied.
  if (isa<DbgInfoIntrinsic>(Inst))
    return;

  Instruction *NewInst = Inst->clone();

  // Replace old operands with the new ones.
  for (Value *OldOperand : Inst->operands()) {
    Value *NewOperand =
        getNewValue(Stmt, OldOperand, BBMap, LTS, getLoopForStmt(Stmt));

    if (!NewOperand) {
      assert(!isa<StoreInst>(NewInst) &&
             "Store instructions are always needed!");
      NewInst->deleteValue();
      return;
    }

    NewInst->replaceUsesOfWith(OldOperand, NewOperand);
  }

  Builder.Insert(NewInst);
  BBMap[Inst] = NewInst;

  if (!NewInst->getType()->isVoidTy())
    NewInst->setName("p_" + Inst->getName());
}

Value *
BlockGenerator::generateLocationAccessed(ScopStmt &Stmt, MemAccInst Inst,
                                         ValueMapT &BBMap, LoopToScevMapT &LTS,
                                         isl_id_to_ast_expr *NewAccesses) {
  const MemoryAccess &MA = Stmt.getArrayAccessFor(Inst);
  return generateLocationAccessed(
      Stmt, getLoopForStmt(Stmt),
      Inst.isNull() ? nullptr : Inst.getPointerOperand(), BBMap, LTS,
      NewAccesses, MA.getId(), MA.getAccessValue()->getType());
}

Value *BlockGenerator::generateLocationAccessed(
    ScopStmt &Stmt, Loop *L, Value *Pointer, ValueMapT &BBMap,
    LoopToScevMapT &LTS, isl_id_to_ast_expr *NewAccesses, __isl_take isl_id *Id,
    Type *ExpectedType) {
  isl_ast_expr *AccessExpr = isl_id_to_ast_expr_get(NewAccesses, Id);

  if (AccessExpr) {
    AccessExpr = isl_ast_expr_address_of(AccessExpr);
    auto Address = ExprBuilder->create(AccessExpr);

    // Cast the address of this memory access to a pointer type that has the
    // same element type as the original access, but uses the address space of
    // the newly generated pointer.
    auto OldPtrTy = ExpectedType->getPointerTo();
    auto NewPtrTy = Address->getType();
    OldPtrTy = PointerType::get(OldPtrTy->getElementType(),
                                NewPtrTy->getPointerAddressSpace());

    if (OldPtrTy != NewPtrTy)
      Address = Builder.CreateBitOrPointerCast(Address, OldPtrTy);
    return Address;
  }
  assert(
      Pointer &&
      "If expression was not generated, must use the original pointer value");
  return getNewValue(Stmt, Pointer, BBMap, LTS, L);
}

Value *
BlockGenerator::getImplicitAddress(MemoryAccess &Access, Loop *L,
                                   LoopToScevMapT &LTS, ValueMapT &BBMap,
                                   __isl_keep isl_id_to_ast_expr *NewAccesses) {
  if (Access.isLatestArrayKind())
    return generateLocationAccessed(*Access.getStatement(), L, nullptr, BBMap,
                                    LTS, NewAccesses, Access.getId(),
                                    Access.getAccessValue()->getType());

  return getOrCreateAlloca(Access);
}

Loop *BlockGenerator::getLoopForStmt(const ScopStmt &Stmt) const {
  auto *StmtBB = Stmt.getEntryBlock();
  return LI.getLoopFor(StmtBB);
}

Value *BlockGenerator::generateArrayLoad(ScopStmt &Stmt, LoadInst *Load,
                                         ValueMapT &BBMap, LoopToScevMapT &LTS,
                                         isl_id_to_ast_expr *NewAccesses) {
  if (Value *PreloadLoad = GlobalMap.lookup(Load))
    return PreloadLoad;

  Value *NewPointer =
      generateLocationAccessed(Stmt, Load, BBMap, LTS, NewAccesses);
  auto *ScalarLoad = Builder.CreateAlignedLoad(
      NewPointer, Load->getAlignment(), Load->getName() + "_p_scalar_");
  if (Aligned)
    ScalarLoad->setAlignment(16);

  if (PollyDebugPrinting)
    RuntimeDebugBuilder::createCPUPrinter(Builder, "Load from ", NewPointer,
                                          ": ", ScalarLoad, "\n");

  MemoryAccess &MA = Stmt.getArrayAccessFor(Load);
  MABBMap[&MA] = ScalarLoad;

  if (MA.isAffine()) {
    auto *AccRel = MA.getAddressFunction();
    if (!isl_map_is_injective(AccRel)) {
      isl_map_free(AccRel);
      return ScalarLoad;
    }
    isl_union_map *ScheduleUMap =
        isl_ast_build_get_schedule(Stmt.getAstBuild());
    isl_map *Schedule = isl_map_from_union_map(ScheduleUMap);
    AccRel = isl_map_apply_domain(AccRel, Schedule);
    //errs() << "-- AccRel: " << AccRel << "\n";
    assert(!MALoopMap[MA.getScopArrayInfo()].count(ScalarLoad));
    MALoopMap[MA.getScopArrayInfo()][ScalarLoad] = {&Stmt, AccRel};
  }

  return ScalarLoad;
}

Value *BlockGenerator::generateArrayLoad2(ScopStmt &Stmt, MemoryAccess &MA,
                                          ValueMapT &BBMap, LoopToScevMapT &LTS,
                                          isl_id_to_ast_expr *NewAccesses,
                                          Type *Ty) {
  LoadInst *Load = cast<LoadInst>(MA.getAccessInstruction());
  Value *NewPointer = generateLocationAccessed(
      Stmt, getLoopForStmt(Stmt), Load->getPointerOperand(), BBMap, LTS,
      NewAccesses, MA.getId(), Ty);
  auto *ScalarLoad = Builder.CreateAlignedLoad(
      NewPointer, Load->getAlignment(), Load->getName() + "_p_scalar_");
  if (Aligned)
    ScalarLoad->setAlignment(16);

  if (PollyDebugPrinting)
    RuntimeDebugBuilder::createCPUPrinter(Builder, "Load from ", NewPointer,
                                          ": ", ScalarLoad, "\n");

  return ScalarLoad;
}


StoreInst* BlockGenerator::generateArrayStore(ScopStmt &Stmt, StoreInst *Store,
                                        ValueMapT &BBMap, LoopToScevMapT &LTS,
                                        isl_id_to_ast_expr *NewAccesses) {
  Value *NewPointer =
      generateLocationAccessed(Stmt, Store, BBMap, LTS, NewAccesses);
  Value *ValueOperand = getNewValue(Stmt, Store->getValueOperand(), BBMap, LTS,
                                    getLoopForStmt(Stmt));

  if (PollyDebugPrinting)
    RuntimeDebugBuilder::createCPUPrinter(Builder, "Store to  ", NewPointer,
                                          ": ", ValueOperand, " [",
                                          Stmt.getBaseName(), "]\n");

  assert(
      NewPointer->getType()->getPointerElementType()->getScalarSizeInBits() ==
      ValueOperand->getType()->getScalarSizeInBits());
  if (NewPointer->getType()->getPointerElementType() !=
      ValueOperand->getType())
    NewPointer = Builder.CreateBitOrPointerCast(
        NewPointer, ValueOperand->getType()->getPointerTo());

  auto *StoreCopy = Builder.CreateAlignedStore(ValueOperand, NewPointer,
                                               Store->getAlignment());
  if (Aligned)
    StoreCopy->setAlignment(16);
  (void)StoreCopy;
  //MDNode *Kind = MDNode::get(Builder.getContext(),
                              //{ConstantAsMetadata::get(Builder.getInt32(1))});
  //StoreCopy->setMetadata(LLVMContext::MD_nontemporal, Kind);
  return StoreCopy;
}

bool BlockGenerator::canSyntheziseInStmt(ScopStmt &Stmt, Instruction *Inst) {
  Loop *L = getLoopForStmt(Stmt);
  return (Stmt.isBlockStmt() || !Stmt.getRegion()->contains(L)) &&
         canSynthesize(Inst, *Stmt.getParent(), &SE, L);
}

void BlockGenerator::copyInstruction(ScopStmt &Stmt, Instruction *Inst,
                                     ValueMapT &BBMap, LoopToScevMapT &LTS,
                                     isl_id_to_ast_expr *NewAccesses) {
  if (BBMap.count(Inst))
    return;
  // Terminator instructions control the control flow. They are explicitly
  // expressed in the clast and do not need to be copied.
  if (Inst->isTerminator())
    return;

  // Synthesizable statements will be generated on-demand.
  if (canSyntheziseInStmt(Stmt, Inst))
    return;

  if (auto *Load = dyn_cast<LoadInst>(Inst)) {
    if (BBMap.count(Load))
      return;

    Value *NewLoad = generateArrayLoad(Stmt, Load, BBMap, LTS, NewAccesses);
    // Compute NewLoad before its insertion in BBMap to make the insertion
    // deterministic.
    BBMap[Load] = NewLoad;
    return;
  }

  if (auto *Store = dyn_cast<StoreInst>(Inst)) {
    // Identified as redundant by -polly-simplify.
    if (!Stmt.getArrayAccessOrNULLFor(Store))
      return;

    //static int X = 0;
    //if (getenv("SKIPSTORES") && X++ < atoi(getenv("SKIPSTORES")))
      //return;
    //static int M = 0;
    //if (getenv("MAXSTORES") && M++ >= atoi(getenv("MAXSTORES")))
      //return;
    generateArrayStore(Stmt, Store, BBMap, LTS, NewAccesses);
    return;
  }

  if (auto *PHI = dyn_cast<PHINode>(Inst)) {
    copyPHIInstruction(Stmt, PHI, BBMap, LTS);
    return;
  }

  // Skip some special intrinsics for which we do not adjust the semantics to
  // the new schedule. All others are handled like every other instruction.
  if (isIgnoredIntrinsic(Inst))
    return;

  copyInstScalar(Stmt, Inst, BBMap, LTS);
}

void BlockGenerator::removeDeadInstructions(BasicBlock *BB, ValueMapT &BBMap) {
  SmallPtrSet<BasicBlock *, 8> BBsToCheck;
  for (auto I = BB->rbegin(); I != BB->rend(); I++) {
    Instruction *NewInst = &*I;

    if (!isInstructionTriviallyDead(NewInst))
      continue;

    //for (auto Pair : BBMap)
      //if (Pair.second == NewInst) {
        //BBMap.erase(Pair.first);
      //}

    for (auto &Op : NewInst->operands())
      if (auto *OpI = dyn_cast<Instruction>(Op))
        BBsToCheck.insert(OpI->getParent());
    NewInst->eraseFromParent();
    I = BB->rbegin();
  }
  for (auto *BB: BBsToCheck)
    removeDeadInstructions(BB, BBMap);

  #if 0
  SmallVector<BinaryOperator *, 8> FSubUsers;
  for (auto &I : *BB) {
    if (!I.getType()->isFloatingPointTy())
      continue;
    auto *BinOp = dyn_cast<BinaryOperator>(&I);
    if (!BinOp || BinOp->getOpcode() != BinaryOperator::FMul)
      continue;
    for (auto *User : I.users()) {
      auto *BinOpUser = dyn_cast<BinaryOperator>(User);
      if (!BinOpUser || BinOpUser->getOpcode() != BinaryOperator::FSub)
        continue;
      errs() << "Mul: " << I.getName() << " => " << BinOpUser->getName() << "\n";
      FSubUsers.push_back(BinOpUser);
    }
  }
  for (auto *FSub : FSubUsers) {
    auto *Op = FSub->getOperand(1);
    auto *NegOp = BinaryOperator::CreateFNeg(Op, Op->getName() + "_neg", FSub);
    auto *FAdd = BinaryOperator::CreateFAdd(
        FSub->llvm::User::getOperand(0), NegOp, FSub->getName() + "_add", FSub);
    FSub->replaceAllUsesWith(FAdd);
    errs() << "Replace " << FSub->getName() << " by " << FAdd->getName() << "\n";
    //FSub->eraseFromParent();
  }
  #endif
}

void BlockGenerator::copyStmt(ScopStmt &Stmt, LoopToScevMapT &LTS,
                              isl_id_to_ast_expr *NewAccesses) {
  assert(Stmt.isBlockStmt() &&
         "Only block statements can be copied by the block generator");
  //if (!Stmt.CopyStmts.empty())
    //return;

  ValueMapT BBMap;

  BasicBlock *BB = Stmt.getBasicBlock();
  copyBB(Stmt, BB, BBMap, LTS, NewAccesses);
  //removeDeadInstructions(BB, BBMap);
}

BasicBlock *BlockGenerator::splitBB(BasicBlock *BB) {
  BasicBlock *CopyBB = SplitBlock(Builder.GetInsertBlock(),
                                  &*Builder.GetInsertPoint(), &DT, &LI);
  CopyBB->setName("polly.stmt." + BB->getName());
  return CopyBB;
}

BasicBlock *BlockGenerator::copyBB(ScopStmt &Stmt, BasicBlock *BB,
                                   ValueMapT &BBMap, LoopToScevMapT &LTS,
                                   isl_id_to_ast_expr *NewAccesses) {
  BasicBlock *CopyBB = splitBB(BB);
  Builder.SetInsertPoint(&CopyBB->front());
  generateScalarLoads(Stmt, LTS, BBMap, NewAccesses);

  copyBB(Stmt, BB, CopyBB, BBMap, LTS, NewAccesses);

  // After a basic block was copied store all scalars that escape this block in
  // their alloca.
  generateScalarStores(Stmt, LTS, BBMap, NewAccesses);

  //if (auto *L = Annotator.peekLoop())
    //for (auto &I : *CopyBB)
      //if (auto *SI = dyn_cast<StoreInst>(&I)) {
        ////createExpressionTree(*CopyBB, L);
        //sortBB(Stmt, SI, L, LI, DT);
      //}

  return CopyBB;
}

static void movePrior(Instruction *I, Loop *L) {
  //errs() << "Move: " << *I << "\n";
  if (isa<PHINode>(I) || !L->contains(I))
    return;

  if (I->getNumOperands() == 1)
    if (auto *IOpI = dyn_cast<Instruction>(I->getOperand(0)))
      if (!isa<PHINode>(IOpI))
        I->moveBefore(IOpI->getNextNode());

  if (!L->contains(I))
    return;

  SmallPtrSet<Instruction *, 8> InLoopOps;
  for (auto &Op : I->operands())
    if (auto *OpI = dyn_cast<Instruction>(Op)) {
      movePrior(OpI, L);

      if (L->contains(OpI) && !isa<PHINode>(OpI))
        InLoopOps.insert(OpI);
    }

  if (InLoopOps.empty()) {
    I->moveBefore(&*L->getHeader()->getFirstInsertionPt());
    return;
  }

  if (InLoopOps.size() == 1) {
    auto *InLoopOp = *InLoopOps.begin();
    I->moveBefore(InLoopOp->getNextNode());
    return;
  }

  if (std::all_of(InLoopOps.begin(), InLoopOps.end(), [&](Instruction *OpI) {
        return OpI->getParent() != I->getParent();
      })) {
    I->moveBefore(&*I->getParent()->getFirstInsertionPt());
  }
}

static Instruction *getUniqueUserInLoop(Instruction *I, Loop *L) {
  if (I->getNumUses() == 0)
    return nullptr;

  auto *User = I->user_back();
  if (!L->contains(User))
    return nullptr;
  for (auto *U : I->users())
    if (U != User)
      return nullptr;

  return User;
}

static void moveToUser(Instruction *I, Loop *L) {
  auto *User = getUniqueUserInLoop(I, L);
  if (User)
    I->moveBefore(User);

  for (auto &Op : I->operands())
    if (auto *OpI = dyn_cast<Instruction>(Op))
      if (L->contains(OpI) && !isa<PHINode>(OpI))
        moveToUser(OpI, L);
}

void BlockGenerator::copyBB(ScopStmt &Stmt, BasicBlock *BB, BasicBlock *CopyBB,
                            ValueMapT &BBMap, LoopToScevMapT &LTS,
                            isl_id_to_ast_expr *NewAccesses) {
  EntryBB = &CopyBB->getParent()->getEntryBlock();

  for (Instruction &Inst : *BB)
    copyInstruction(Stmt, &Inst, BBMap, LTS, NewAccesses);
}

Value *BlockGenerator::getOrCreateAlloca(const MemoryAccess &Access) {
  assert(!Access.isLatestArrayKind() && "Trying to get alloca for array kind");

  return getOrCreateAlloca(Access.getLatestScopArrayInfo());
}

Value *BlockGenerator::getOrCreateAlloca(const ScopArrayInfo *Array) {
  assert(!Array->isArrayKind() && "Trying to get alloca for array kind");

  auto &Addr = ScalarMap[Array];

  if (Addr) {
    // Allow allocas to be (temporarily) redirected once by adding a new
    // old-alloca-addr to new-addr mapping to GlobalMap. This funcitionality
    // is used for example by the OpenMP code generation where a first use
    // of a scalar while still in the host code allocates a normal alloca with
    // getOrCreateAlloca. When the values of this scalar are accessed during
    // the generation of the parallel subfunction, these values are copied over
    // to the parallel subfunction and each request for a scalar alloca slot
    // must be forwared to the temporary in-subfunction slot. This mapping is
    // removed when the subfunction has been generated and again normal host
    // code is generated. Due to the following reasons it is not possible to
    // perform the GlobalMap lookup right after creating the alloca below, but
    // instead we need to check GlobalMap at each call to getOrCreateAlloca:
    //
    //   1) GlobalMap may be changed multiple times (for each parallel loop),
    //   2) The temporary mapping is commonly only known after the initial
    //      alloca has already been generated, and
    //   3) The original alloca value must be restored after leaving the
    //      sub-function.
    if (Value *NewAddr = GlobalMap.lookup(&*Addr))
      return NewAddr;
    return Addr;
  }

  Type *Ty = Array->getElementType();
  Value *ScalarBase = Array->getBasePtr();
  std::string NameExt;
  if (Array->isPHIKind())
    NameExt = ".phiops";
  else
    NameExt = ".s2a";

  const DataLayout &DL = Builder.GetInsertBlock()->getModule()->getDataLayout();

  Addr = new AllocaInst(Ty, DL.getAllocaAddrSpace(),
                        ScalarBase->getName() + NameExt);
  EntryBB = &Builder.GetInsertBlock()->getParent()->getEntryBlock();
  Addr->insertBefore(&*EntryBB->getFirstInsertionPt());

  return Addr;
}

void BlockGenerator::handleOutsideUsers(const Scop &S, ScopArrayInfo *Array) {
  Instruction *Inst = cast<Instruction>(Array->getBasePtr());

  // If there are escape users we get the alloca for this instruction and put it
  // in the EscapeMap for later finalization. Lastly, if the instruction was
  // copied multiple times we already did this and can exit.
  if (EscapeMap.count(Inst))
    return;

  EscapeUserVectorTy EscapeUsers;
  for (User *U : Inst->users()) {

    // Non-instruction user will never escape.
    Instruction *UI = dyn_cast<Instruction>(U);
    if (!UI)
      continue;

    if (S.contains(UI))
      continue;

    EscapeUsers.push_back(UI);
  }

  // Exit if no escape uses were found.
  if (EscapeUsers.empty())
    return;

  // Get or create an escape alloca for this instruction.
  auto *ScalarAddr = getOrCreateAlloca(Array);

  // Remember that this instruction has escape uses and the escape alloca.
  EscapeMap[Inst] = std::make_pair(ScalarAddr, std::move(EscapeUsers));
}

Value *BlockGenerator::recomputeLoopPHI(ScopStmt &Stmt, PHINode *PHI,
                                        RecomputeInfo &RI) {
  DEBUG(errs() << "PHI: " << *PHI << "\n");
  DEBUG(errs() << "Dep: " << RI.Dependence << "\n");
  assert(PHI->getType()->isIntegerTy());
  assert(LI.isLoopHeader(PHI->getParent()));

  Loop *L = LI.getLoopFor(PHI->getParent());
  assert(SE.isSCEVable(PHI->getType()));
  const SCEV *PHIScev = SE.getSCEVAtScope(PHI, L);
  assert(PHIScev && !isa<SCEVCouldNotCompute>(PHIScev));

  DEBUG(errs() << "PHISCev: " << *PHIScev << "\n");

  Scop &S = *Stmt.getParent();

  isl_pw_aff *PHIPWA = S.getPwAffOnly(PHIScev, PHI->getParent());
  DEBUG(errs() << "PHIPWA: " << PHIPWA << " : " << PHI->getParent()->getName() << "\n");

  isl_map *Dep = isl_map_copy(RI.Dependence);
  auto *PRI = RI.ParentRI;

  auto *BB = RI.RecomputeMA->getStatement()->getBasicBlock();
  //errs() << "Dep: " << Dep << "\nBB: " << BB->getName() << "\n";
  //errs() << "Stmt: " << Stmt.getBaseName() << "\n";
  while (PRI) {
    //errs() << "PRI: " << PRI << " : " << PRI->Dependence << "\n";
    auto *PRIDefStmt =PRI->DefiningMA->getStatement();
    if (PRIDefStmt->getBasicBlock() == BB) {
      Dep = isl_map_set_tuple_id(Dep, isl_dim_out, PRIDefStmt->getDomainId());
      //errs() << "Dep: " << Dep << "\nBB: " << BB->getName() << "\n";
      Dep = isl_map_apply_range(Dep, isl_map_copy(PRI->Dependence));
      BB = PRI->RecomputeMA->getStatement()->getBasicBlock();
      //errs() << "Dep: " << Dep << "\nBB: " << BB->getName() << "\n";
    }
    PRI = PRI->ParentRI;
  }
  DEBUG(errs() << "Dep: " << Dep << "\n");

  int Depth = S.getRelativeLoopDepth(L);
  int Dims = isl_map_dim(Dep, isl_dim_in);
  if (Dims > Depth + 1)
    Dep = isl_map_project_out(Dep, isl_dim_in, Depth+1, Dims - Depth - 1);
  DEBUG(errs() << "Dep: " << Dep << "\n");

  Dep = isl_map_reset_tuple_id(Dep, isl_dim_in);
  Dep = isl_map_reset_tuple_id(Dep, isl_dim_out);
  Dep = isl_map_gist_params(Dep, S.getAssumedContext());
  Dep = isl_map_set_tuple_id(Dep, isl_dim_out, Stmt.getDomainId());
  DEBUG(errs() << "Dep: " << Dep << "\n");

  isl_union_map *Schedule = isl_ast_build_get_schedule(Stmt.getAstBuild());
  //Schedule = isl_union_map_intersect_domain(
      //Schedule, isl_union_set_from_set(Stmt.getDomain()));
  isl_map *ScheduleMap = isl_map_from_union_map(Schedule);
  DEBUG(errs() << "ScheduleMap: " << ScheduleMap << "\n");

  Dep = isl_map_apply_range(Dep, ScheduleMap);
  DEBUG(errs() << "Dep: " << Dep << "\n");

  isl_map *PHIAccMap = isl_map_from_pw_aff(PHIPWA);
  DEBUG(errs() << "PHIAccMap: " << PHIAccMap << "\n");
  PHIAccMap = isl_map_apply_domain(PHIAccMap, Dep);
  DEBUG(errs() << "PHIAccMap: " << PHIAccMap << "\n");
  PHIAccMap = isl_map_lexmin(PHIAccMap);
  DEBUG(errs() << "PHIAccMap: " << PHIAccMap << "\n");
  //if (isl_map_is_empty(PHIAccMap))
    //return UndefValue::get(PHI->getType());
  assert(!isl_map_is_empty(PHIAccMap));

  isl_pw_multi_aff *PHIPWmA = isl_pw_multi_aff_from_map(PHIAccMap);

  auto *PHIExpr = isl_ast_build_expr_from_pw_aff(
      Stmt.getAstBuild(), isl_pw_multi_aff_get_pw_aff(PHIPWmA, 0));

  isl_pw_multi_aff_free(PHIPWmA);

  auto *NewVal = ExprBuilder->create(PHIExpr);
  DEBUG(errs() << "NewVal: " << *NewVal << "\n");

  assert(!NewVal->getType()->isFloatingPointTy());
  if (NewVal->getType() != PHI->getType())
    NewVal = Builder.CreateSExtOrTrunc(NewVal, PHI->getType());
  DEBUG(errs() << "NewVal: " << *NewVal << "\n");

  return NewVal;
}

void BlockGenerator::setLoopMemAcc(ScopStmt &Stmt, RecomputeInfo *RI,
                                   Instruction *Val, Instruction *NewVal) {
  assert(RI && Val && NewVal);
  auto *Inst = dyn_cast<Instruction>(Val);
  assert(Inst);
  auto &DefStmt = *RI->DefiningMA->getStatement();
  auto *CurMA = DefStmt.getArrayAccessOrNULLFor(Inst);
  assert(CurMA);
  if (!CurMA->isAffine())
    return;
  auto *SAI = CurMA->getScopArrayInfo();
  if (MALoopMap[SAI].count(NewVal))
    return;

  //errs() << "Original MA in [" << Stmt.getBaseName() << "]:\n";
  //CurMA->dump();

  isl_map *AccRel = CurMA->getAddressFunction();
  if (!isl_map_is_injective(AccRel)) {
    isl_map_free(AccRel);
    return;
  }
  while (RI) {
    //errs() << "RI: " << RI << " CurMA: " << CurMA << " AccRel: " << AccRel
           //<< "\n   Dep: " << RI->Dependence << "\n";
    auto *RIDefStmt = RI->DefiningMA->getStatement();
    if (auto *LocalMA = RIDefStmt->MemAccMap.lookup(CurMA)) {
      //errs() << "2 LocalMA: " << LocalMA << "\n";
      AccRel =
          isl_map_set_tuple_id(AccRel, isl_dim_in, RIDefStmt->getDomainId());
      CurMA = LocalMA;
      continue;
    }
    if (auto *LocalMA = Stmt.MemAccMap.lookup(CurMA)) {
      //errs() << "0 LocalMA: " << LocalMA << "\n";
      assert(!RI->NewAccs.count(CurMA));
      CurMA = LocalMA;
      continue;
    }
    if (auto *LocalMA = RI->NewAccs.lookup(CurMA)) {
      //errs() << "1 LocalMA: " << LocalMA << "\n";
      assert(!Stmt.MemAccMap.count(CurMA));
      isl_map *Dep = isl_map_copy(RI->Dependence);
      AccRel = isl_map_apply_domain(AccRel, Dep);
      CurMA = LocalMA;
    }
    if (CurMA->getStatement()->getBasicBlock() == RIDefStmt->getBasicBlock()) {
      //errs() << "3 LocalMA: " << RI->RecomputeMA << "\n";
      assert(Stmt.getParent()->RecomputeMAs.count(CurMA));
      isl_map *Dep = isl_map_copy(RI->Dependence);
      AccRel =
          isl_map_set_tuple_id(AccRel, isl_dim_in, RIDefStmt->getDomainId());
      AccRel = isl_map_apply_domain(AccRel, Dep);
      CurMA = RI->RecomputeMA;
    }

    RI = RI->ParentRI;
  }

  AccRel = isl_map_set_tuple_id(AccRel, isl_dim_in, Stmt.getDomainId());
  AccRel = isl_map_gist_domain(AccRel, Stmt.getDomain());
  AccRel = isl_map_gist_params(AccRel, Stmt.getParent()->getAssumedContext());

  //errs() << "Final AccRel: " << AccRel << "\n";
  isl_union_map *ScheduleUMap = isl_ast_build_get_schedule(Stmt.getAstBuild());
  isl_map *Schedule = isl_map_from_union_map(ScheduleUMap);
  AccRel = isl_map_apply_domain(AccRel, Schedule);
  //errs() << "Sched AccRel: " << AccRel << "\n";

  MALoopMap[SAI][NewVal] = {&Stmt, AccRel};
}

MemoryAccess *BlockGenerator::getLocalMA(ScopStmt &Stmt,
    RecomputeInfo &RI, MemoryAccess *MA) {
  //errs() << "Stmt: " << Stmt.getBaseName() << " RI " << &RI << " MA: " << MA
         //<< "\n";
  if (auto *LocalMA = RI.DefiningMA->getStatement()->MemAccMap.lookup(MA)) {
    assert(!RI.NewAccs.count(MA));
    return getLocalMA(Stmt, RI, LocalMA);
  }
  if (auto *LocalMA = Stmt.MemAccMap.lookup(MA)) {
    assert(!RI.NewAccs.count(MA));
    return getLocalMA(Stmt, RI, LocalMA);
  }
  if (auto *LocalMA = RI.NewAccs.lookup(MA)) {
    assert(!Stmt.MemAccMap.count(MA));
    return getLocalMA(Stmt, RI, LocalMA);
  }
  if (RI.ParentRI)
    return getLocalMA(Stmt, *RI.ParentRI, MA);
  return MA;
}

MemoryAccess *BlockGenerator::getLocalMemAcc(ScopStmt &Stmt, RecomputeInfo &RI,
                                             Value *Val) {
  auto *Inst = dyn_cast<Instruction>(Val);
  if (!Inst)
    return nullptr;

  auto &DefStmt = *RI.DefiningMA->getStatement();
  auto *InstMA = DefStmt.getArrayAccessOrNULLFor(Inst);
  if (!InstMA)
    return nullptr;

  //errs() << "Inst: " << Inst << " DefStmt: " << DefStmt.getBaseName()
         //<< " InstMA: " << InstMA << "\n";
  //errs() << "OldVal is mapped to MA: [" << InstMA << "]\n";
  //InstMA->dump();
  InstMA = getLocalMA(Stmt, RI, InstMA);
  //errs() << "LocalMA: [" << InstMA << "]\n";
  //InstMA->dump();
  return InstMA;
}

Value *BlockGenerator::recomputeInstruction(
    ScopStmt &Stmt, LoopToScevMapT &LTS, ValueMapT &BBMap, Value *Val,
    RecomputeInfo &RI, __isl_keep isl_id_to_ast_expr *NewAccesses) {
  DEBUG(errs() << "Recompute Val: " << *Val << "\n");
  if (Value *NewVal = GlobalMap.lookup(Val)) {
      if (Value *NewVal2 = GlobalMap.lookup(NewVal))
        return NewVal2;
    return NewVal;
  }

  Value *NewVal = RI.BBMap.lookup(Val);
  auto *LocalInstMA = getLocalMemAcc(Stmt, RI, Val);

  if (LocalInstMA && !NewVal) {
    //errs() << "LocalInstMA:\n";
    //LocalInstMA->dump();
    //if (LocalInstMA->getStatement() == &Stmt) {
      //errs() << "LocalInstMA is in Stmt!\n";
      NewVal = MABBMap.lookup(LocalInstMA);
      //if (NewVal)
        //errs() << "  Mapped to => " << *NewVal << "\n";
    //}
  }

  if (LocalInstMA && !NewVal) {
    Scop &S = *Stmt.getParent();
    if (auto *ChainedRI = S.RecomputeMAs.lookup(LocalInstMA)) {
      //errs() << "MA is chained to " << ChainedRI << " : "
             //<< ChainedRI->Dependence << "\n";
      assert(!ChainedRI->ParentRI);
      ChainedRI->ParentRI = &RI;
      NewVal = recomputeInstructionTree(Stmt, LTS, BBMap, *ChainedRI, NewAccesses);
      ChainedRI->ParentRI = nullptr;
    } else {
      bool Hit =
          std::any_of(Stmt.begin(), Stmt.end(),
                      [&](const MemoryAccess *MA) { return MA == LocalInstMA; });
      assert(Hit);
    }
  }

  if (!NewVal) {
    //if (LocalInstMA)
      //NewVal = generateArrayLoad2(Stmt, *LocalInstMA, BBMap, LTS, NewAccesses,
                                  //Val->getType());
    //else
    NewVal = recomputeInstruction2(Stmt, LTS, BBMap, Val, RI, NewAccesses);
  }

  assert(NewVal);
  DEBUG(errs() << "       NewVal: " << *NewVal << " for Val: " << *Val << "\n");
  assert(!RI.BBMap.count(Val) || RI.BBMap[Val] == NewVal);
  RI.BBMap[Val] = NewVal;

  if (LocalInstMA && isa<Instruction>(NewVal))
    setLoopMemAcc(Stmt, &RI, cast<Instruction>(Val), cast<Instruction>(NewVal));

  if (LocalInstMA && LocalInstMA->getStatement() == &Stmt) {
    //errs() << "LocalInstMA [" << LocalInstMA << "] is in current Stmt!\nMap "
           //<< LocalInstMA << " in MABBMAp to " << *NewVal << "\n";
    assert(!MABBMap.count(LocalInstMA) ||
           MABBMap[LocalInstMA] == NewVal);
    MABBMap[LocalInstMA] = NewVal;
  }

  if (auto *LoadI = dyn_cast<LoadInst>(NewVal)) {

    //MDNode *Kind = MDNode::get(Builder.getContext(),
                                //{ConstantAsMetadata::get(Builder.getInt32(1))});
    //LoadI->setMetadata(LLVMContext::MD_nontemporal, Kind);

    //LoadI->setVolatile(true);
    if (PollyDebugPrinting) {
      if (!LoadI->hasName())
        LoadI->setName("l");
      auto IP = Builder.GetInsertPoint();
      Builder.SetInsertPoint(LoadI);
      RuntimeDebugBuilder::createCPUPrinter(
          Builder, "Load from  ", LoadI->getPointerOperand(), " in ",
          Stmt.getBaseName(), " [", LoadI->getName(), "]\n");
      Builder.SetInsertPoint(&*IP);
    }
  }

  return NewVal;
}

Value *BlockGenerator::recomputeInstruction2(
    ScopStmt &Stmt, LoopToScevMapT &LTS, ValueMapT &BBMap, Value *Val,
    RecomputeInfo &RI, __isl_keep isl_id_to_ast_expr *NewAccesses) {

  if (Value *NewVal = RI.BBMap.lookup(Val)) {
    return NewVal;
  }

  if (Value *NewVal = GlobalMap.lookup(Val)) {
      if (Value *NewVal2 = GlobalMap.lookup(NewVal))
        return NewVal2;
    return NewVal;
  }

  Loop *L = getLoopForStmt(Stmt);
  auto *Inst = dyn_cast<Instruction>(Val);
  if (!Inst || !Stmt.getParent()->contains(Inst)) {
    if (Value *New = trySynthesizeNewValue(Stmt, Val, RI.BBMap, LTS, L)) {
      return New;
    }
    return Val;
  }

  if (auto *PHI = dyn_cast<PHINode>(Inst)) {
    auto *New = recomputeLoopPHI(Stmt, PHI, RI);
    return New;
  }

  Instruction *NewInst = Inst->clone();

  SmallVector<Value *, 4> Ops;
  for (auto &Op : Inst->operands()) {
    auto *NewOp = recomputeInstruction(Stmt, LTS, BBMap, Op, RI, NewAccesses);
    assert(NewOp);
    Ops.push_back(NewOp);
  }

  unsigned u = 0;
  for (auto &Op : Inst->operands())
    NewInst->replaceUsesOfWith(Op, Ops[u++]);

  Builder.Insert(NewInst);

  if (!NewInst->getType()->isVoidTy())
    NewInst->setName("p_" + Inst->getName());

  return NewInst;
}

void BlockGenerator::createNextItPHI(ScopStmt &Stmt, NextItMapTy &NextItRevMap,
                                     MemoryAccess *PrevMA, MemoryAccess *NextMA,
                                     ValueMapT &BBMap, BasicBlock *HeaderBB) {
  if (MABBMap.count(NextMA))
    return;

  if (auto *PrevPrevMA = NextItRevMap.lookup(PrevMA))
    createNextItPHI(Stmt, NextItRevMap, PrevPrevMA, PrevMA, BBMap, HeaderBB);

  //if (!PropagateLoads && !S.RecomputeMAs.count(PrevMA))
    //continue;

  DEBUG(errs() << "Prev MA: [" << PrevMA << "]\n"
               << "Next MA: [" << NextMA << "]\n");

  auto *Val = NextMA->getAccessValue();
  auto *PHI = PHINode::Create(Val->getType(), 2, Val->getName() + "_rec",
                              HeaderBB->getFirstNonPHI());

  DEBUG(errs() << "Set new PHI for " << *Val << " : " << *PHI << "\n");
  assert(!MABBMap.count(NextMA));
  MABBMap[NextMA] = PHI;
  if (Stmt.contains(NextMA->getAccessInstruction()->getParent()))
    BBMap[NextMA->getAccessValue()] = PHI;
  PHIMap[PHI] = NextMA;
}

Value *BlockGenerator::recomputeInstructionTree(
    ScopStmt &Stmt, LoopToScevMapT &LTS, ValueMapT &BBMap, RecomputeInfo &RI,
    __isl_keep isl_id_to_ast_expr *NewAccesses) {
  Value *OriginalInst = RI.RecomputeMA->getAccessValue();
  DEBUG({
    errs() << "\nRecompute Inst Tree for : " << *OriginalInst;
    auto *CRI = &RI;
    while (CRI) {
      errs() << " [" << CRI << "]";
      CRI = CRI->ParentRI;
    }
    errs() << "\n  Dep: " << RI.Dependence << " in " << Stmt.getBaseName()
           << "\n";
  });

  RI.BBMap.clear();
  Value *NewVal = RI.DefiningMA->getAccessValue();
  NewVal = recomputeInstruction(Stmt, LTS, BBMap, NewVal, RI, NewAccesses);

  bool NewValIsFP = NewVal->getType()->isFloatingPointTy();
  bool OldValIsFP = OriginalInst->getType()->isFloatingPointTy();
  if (NewValIsFP != OldValIsFP) {
    auto *LoadI = dyn_cast<LoadInst>(NewVal);
    assert(LoadI);
    auto *Ptr = LoadI->getPointerOperand();
    Ptr = Builder.CreatePointerCast(Ptr, OriginalInst->getType()->getPointerTo());
    auto *AdjustedLoadI = Builder.CreateLoad(Ptr, LoadI->getName());
    LoadI->eraseFromParent();
    NewVal = AdjustedLoadI;
    DEBUG(errs() << "RELOAD with adjusted type: " << *NewVal << "\n");
  }

  DEBUG(errs() << "\nDone Recompute Inst Tree for : "
               << *RI.RecomputeMA->getAccessValue()
               << " [#BBMap: " << RI.BBMap.size() << "] [" << &RI << "]\n");

  assert(!RI.BBMap.count(OriginalInst) || RI.BBMap[OriginalInst] == NewVal);
  RI.BBMap[OriginalInst] = NewVal;

  assert(OriginalInst->getType() == NewVal->getType());
  return NewVal;
}

void BlockGenerator::recomputeValuesInOf(
    ScopStmt &Stmt, LoopToScevMapT &LTS, ValueMapT &BBMap,
    __isl_keep isl_id_to_ast_expr *NewAccesses) {
  DEBUG(errs() << "Recompute values [#LocRecV" << Stmt.NumLocalRecomputeValues
               << "] [#RecV" << Stmt.NumRecomputeValues << "] in "
               << Stmt.getBaseName() << "\n");

  Scop &S = *Stmt.getParent();
  SmallVector<MemoryAccess *, 8> Vec;
  Stmt.getOriginalMemAccesses(Vec);
  for (auto *MA : Vec) {
    assert(Stmt.contains(MA->getAccessInstruction()->getParent()));
    auto *AccVal = MA->getAccessValue();
    auto *RI = S.RecomputeMAs.lookup(MA);
    if (!RI)
      continue;
    assert(RI->Dependence);
    DEBUG(errs() << "Start RI for " << *AccVal << " [MA: " << MA
                 << "] [RI: " << RI << "]\n");
    Value *NewVal = MABBMap.lookup(MA);
    if (!NewVal)
      NewVal = recomputeInstructionTree(Stmt, LTS, BBMap, *RI, NewAccesses);
    DEBUG(errs() << "RI Done: " << *AccVal << " => " << *NewVal << " in "
                 << Stmt.getBaseName() << "\n");
    assert(!BBMap.count(AccVal) || BBMap[AccVal] == NewVal);
    BBMap[AccVal] = NewVal;
  }
}

void BlockGenerator::generateScalarLoads(
    ScopStmt &Stmt, LoopToScevMapT &LTS, ValueMapT &BBMap,
    __isl_keep isl_id_to_ast_expr *NewAccesses) {

#if 0
  for (MemoryAccess *MA : Stmt) {
    if (MA->isOriginalArrayKind() || MA->isWrite())
      continue;

#ifndef NDEBUG
    auto *StmtDom = Stmt.getDomain();
    auto *AccDom = isl_map_domain(MA->getAccessRelation());
    assert(isl_set_is_subset(StmtDom, AccDom) &&
           "Scalar must be loaded in all statement instances");
    isl_set_free(StmtDom);
    isl_set_free(AccDom);
#endif

    auto *Address =
        getImplicitAddress(*MA, getLoopForStmt(Stmt), LTS, BBMap, NewAccesses);
    assert((!isa<Instruction>(Address) ||
            DT.dominates(cast<Instruction>(Address)->getParent(),
                         Builder.GetInsertBlock())) &&
           "Domination violation");
    BBMap[MA->getAccessValue()] =
        Builder.CreateLoad(Address, Address->getName() + ".reload");
  }
#endif

  recomputeValuesInOf(Stmt, LTS, BBMap, NewAccesses);
}

void BlockGenerator::generateScalarStores(
    ScopStmt &Stmt, LoopToScevMapT &LTS, ValueMapT &BBMap,
    __isl_keep isl_id_to_ast_expr *NewAccesses) {
  Loop *L = LI.getLoopFor(Stmt.getBasicBlock());

  assert(Stmt.isBlockStmt() &&
         "Region statements need to use the generateScalarStores() function in "
         "the RegionGenerator");

  for (MemoryAccess *MA : Stmt) {
    if (MA->isOriginalArrayKind() || MA->isRead())
      continue;

#ifndef NDEBUG
    auto *StmtDom = Stmt.getDomain();
    auto *AccDom = isl_map_domain(MA->getAccessRelation());
    assert(isl_set_is_subset(StmtDom, AccDom) &&
           "Scalar must be stored in all statement instances");
    isl_set_free(StmtDom);
    isl_set_free(AccDom);
#endif

    Value *Val = MA->getAccessValue();
    if (MA->isAnyPHIKind()) {
      assert(MA->getIncoming().size() >= 1 &&
             "Block statements have exactly one exiting block, or multiple but "
             "with same incoming block and value");
      assert(std::all_of(MA->getIncoming().begin(), MA->getIncoming().end(),
                         [&](std::pair<BasicBlock *, Value *> p) -> bool {
                           return p.first == Stmt.getBasicBlock();
                         }) &&
             "Incoming block must be statement's block");
      Val = MA->getIncoming()[0].second;
    }
    auto Address =
        getImplicitAddress(*MA, getLoopForStmt(Stmt), LTS, BBMap, NewAccesses);

    Val = getNewValue(Stmt, Val, BBMap, LTS, L);
    assert((!isa<Instruction>(Val) ||
            DT.dominates(cast<Instruction>(Val)->getParent(),
                         Builder.GetInsertBlock())) &&
           "Domination violation");
    assert((!isa<Instruction>(Address) ||
            DT.dominates(cast<Instruction>(Address)->getParent(),
                         Builder.GetInsertBlock())) &&
           "Domination violation");
    Builder.CreateStore(Val, Address);
  }

  MABBMap.clear();
}

void BlockGenerator::createScalarInitialization(Scop &S) {
  BasicBlock *ExitBB = S.getExit();
  BasicBlock *PreEntryBB = S.getEnteringBlock();

  Builder.SetInsertPoint(&*StartBlock->begin());

  for (auto &Array : S.arrays()) {
    if (Array->getNumberOfDimensions() != 0)
      continue;
    if (Array->isPHIKind()) {
      // For PHI nodes, the only values we need to store are the ones that
      // reach the PHI node from outside the region. In general there should
      // only be one such incoming edge and this edge should enter through
      // 'PreEntryBB'.
      auto PHI = cast<PHINode>(Array->getBasePtr());

      for (auto BI = PHI->block_begin(), BE = PHI->block_end(); BI != BE; BI++)
        if (!S.contains(*BI) && *BI != PreEntryBB)
          llvm_unreachable("Incoming edges from outside the scop should always "
                           "come from PreEntryBB");

      int Idx = PHI->getBasicBlockIndex(PreEntryBB);
      if (Idx < 0)
        continue;

      Value *ScalarValue = PHI->getIncomingValue(Idx);

      Builder.CreateStore(ScalarValue, getOrCreateAlloca(Array));
      continue;
    }

    auto *Inst = dyn_cast<Instruction>(Array->getBasePtr());

    if (Inst && S.contains(Inst))
      continue;

    // PHI nodes that are not marked as such in their SAI object are either exit
    // PHI nodes we model as common scalars but without initialization, or
    // incoming phi nodes that need to be initialized. Check if the first is the
    // case for Inst and do not create and initialize memory if so.
    if (auto *PHI = dyn_cast_or_null<PHINode>(Inst))
      if (!S.hasSingleExitEdge() && PHI->getBasicBlockIndex(ExitBB) >= 0)
        continue;

    Builder.CreateStore(Array->getBasePtr(), getOrCreateAlloca(Array));
  }
}

void BlockGenerator::createScalarFinalization(Scop &S) {
  // The exit block of the __unoptimized__ region.
  BasicBlock *ExitBB = S.getExitingBlock();
  // The merge block __just after__ the region and the optimized region.
  BasicBlock *MergeBB = S.getExit();

  // The exit block of the __optimized__ region.
  BasicBlock *OptExitBB = *(pred_begin(MergeBB));
  if (OptExitBB == ExitBB)
    OptExitBB = *(++pred_begin(MergeBB));

  Builder.SetInsertPoint(OptExitBB->getTerminator());
  for (const auto &EscapeMapping : EscapeMap) {
    // Extract the escaping instruction and the escaping users as well as the
    // alloca the instruction was demoted to.
    Instruction *EscapeInst = EscapeMapping.first;
    const auto &EscapeMappingValue = EscapeMapping.second;
    const EscapeUserVectorTy &EscapeUsers = EscapeMappingValue.second;
    Value *ScalarAddr = EscapeMappingValue.first;

    // Reload the demoted instruction in the optimized version of the SCoP.
    Value *EscapeInstReload =
        Builder.CreateLoad(ScalarAddr, EscapeInst->getName() + ".final_reload");
    EscapeInstReload =
        Builder.CreateBitOrPointerCast(EscapeInstReload, EscapeInst->getType());

    // Create the merge PHI that merges the optimized and unoptimized version.
    PHINode *MergePHI = PHINode::Create(EscapeInst->getType(), 2,
                                        EscapeInst->getName() + ".merge");
    MergePHI->insertBefore(&*MergeBB->getFirstInsertionPt());

    // Add the respective values to the merge PHI.
    MergePHI->addIncoming(EscapeInstReload, OptExitBB);
    MergePHI->addIncoming(EscapeInst, ExitBB);

    // The information of scalar evolution about the escaping instruction needs
    // to be revoked so the new merged instruction will be used.
    if (SE.isSCEVable(EscapeInst->getType()))
      SE.forgetValue(EscapeInst);

    // Replace all uses of the demoted instruction with the merge PHI.
    for (Instruction *EUser : EscapeUsers)
      EUser->replaceUsesOfWith(EscapeInst, MergePHI);
  }
}

void BlockGenerator::findOutsideUsers(Scop &S) {
  for (auto &Array : S.arrays()) {

    if (Array->getNumberOfDimensions() != 0)
      continue;

    if (Array->isPHIKind())
      continue;

    auto *Inst = dyn_cast<Instruction>(Array->getBasePtr());

    if (!Inst)
      continue;

    // Scop invariant hoisting moves some of the base pointers out of the scop.
    // We can ignore these, as the invariant load hoisting already registers the
    // relevant outside users.
    if (!S.contains(Inst))
      continue;

    handleOutsideUsers(S, Array);
  }
}

void BlockGenerator::createExitPHINodeMerges(Scop &S) {
  if (S.hasSingleExitEdge())
    return;

  auto *ExitBB = S.getExitingBlock();
  auto *MergeBB = S.getExit();
  auto *AfterMergeBB = MergeBB->getSingleSuccessor();
  BasicBlock *OptExitBB = *(pred_begin(MergeBB));
  if (OptExitBB == ExitBB)
    OptExitBB = *(++pred_begin(MergeBB));

  Builder.SetInsertPoint(OptExitBB->getTerminator());

  for (auto &SAI : S.arrays()) {
    auto *Val = SAI->getBasePtr();

    // Only Value-like scalars need a merge PHI. Exit block PHIs receive either
    // the original PHI's value or the reloaded incoming values from the
    // generated code. An llvm::Value is merged between the original code's
    // value or the generated one.
    if (!SAI->isExitPHIKind())
      continue;

    PHINode *PHI = dyn_cast<PHINode>(Val);
    if (!PHI)
      continue;

    if (PHI->getParent() != AfterMergeBB)
      continue;

    std::string Name = PHI->getName();
    Value *ScalarAddr = getOrCreateAlloca(SAI);
    Value *Reload = Builder.CreateLoad(ScalarAddr, Name + ".ph.final_reload");
    Reload = Builder.CreateBitOrPointerCast(Reload, PHI->getType());
    Value *OriginalValue = PHI->getIncomingValueForBlock(MergeBB);
    assert((!isa<Instruction>(OriginalValue) ||
            cast<Instruction>(OriginalValue)->getParent() != MergeBB) &&
           "Original value must no be one we just generated.");
    auto *MergePHI = PHINode::Create(PHI->getType(), 2, Name + ".ph.merge");
    MergePHI->insertBefore(&*MergeBB->getFirstInsertionPt());
    MergePHI->addIncoming(Reload, OptExitBB);
    MergePHI->addIncoming(OriginalValue, ExitBB);
    int Idx = PHI->getBasicBlockIndex(MergeBB);
    PHI->setIncomingValue(Idx, MergePHI);
  }
}

void BlockGenerator::invalidateScalarEvolution(Scop &S) {
  for (auto &Stmt : S)
    if (Stmt.isCopyStmt())
      continue;
    else if (Stmt.isBlockStmt())
      for (auto &Inst : *Stmt.getBasicBlock())
        SE.forgetValue(&Inst);
    else if (Stmt.isRegionStmt())
      for (auto *BB : Stmt.getRegion()->blocks())
        for (auto &Inst : *BB)
          SE.forgetValue(&Inst);
    else
      llvm_unreachable("Unexpected statement type found");
}

void BlockGenerator::finalizeSCoP(Scop &S) {
  findOutsideUsers(S);
  createScalarInitialization(S);
  createExitPHINodeMerges(S);
  createScalarFinalization(S);
  invalidateScalarEvolution(S);
}

VectorBlockGenerator::VectorBlockGenerator(BlockGenerator &BlockGen,
                                           std::vector<LoopToScevMapT> &VLTS,
                                           isl_map *Schedule)
    : BlockGenerator(BlockGen), VLTS(VLTS), Schedule(Schedule) {
  assert(Schedule && "No statement domain provided");
}

Value *VectorBlockGenerator::getVectorValue(ScopStmt &Stmt, Value *Old,
                                            ValueMapT &VectorMap,
                                            VectorValueMapT &ScalarMaps,
                                            Loop *L) {
  if (Value *NewValue = VectorMap.lookup(Old))
    return NewValue;

  int Width = getVectorWidth();

  Value *Vector = UndefValue::get(VectorType::get(Old->getType(), Width));

  for (int Lane = 0; Lane < Width; Lane++)
    Vector = Builder.CreateInsertElement(
        Vector, getNewValue(Stmt, Old, ScalarMaps[Lane], VLTS[Lane], L),
        Builder.getInt32(Lane));

  VectorMap[Old] = Vector;

  return Vector;
}

Type *VectorBlockGenerator::getVectorPtrTy(const Value *Val, int Width) {
  PointerType *PointerTy = dyn_cast<PointerType>(Val->getType());
  assert(PointerTy && "PointerType expected");

  Type *ScalarType = PointerTy->getElementType();
  VectorType *VectorType = VectorType::get(ScalarType, Width);

  return PointerType::getUnqual(VectorType);
}

Value *VectorBlockGenerator::generateStrideOneLoad(
    ScopStmt &Stmt, LoadInst *Load, VectorValueMapT &ScalarMaps,
    __isl_keep isl_id_to_ast_expr *NewAccesses, bool NegativeStride = false) {
  unsigned VectorWidth = getVectorWidth();
  auto *Pointer = Load->getPointerOperand();
  Type *VectorPtrType = getVectorPtrTy(Pointer, VectorWidth);
  unsigned Offset = NegativeStride ? VectorWidth - 1 : 0;

  Value *NewPointer = generateLocationAccessed(Stmt, Load, ScalarMaps[Offset],
                                               VLTS[Offset], NewAccesses);
  Value *VectorPtr =
      Builder.CreateBitCast(NewPointer, VectorPtrType, "vector_ptr");
  LoadInst *VecLoad =
      Builder.CreateLoad(VectorPtr, Load->getName() + "_p_vec_full");
  if (!Aligned)
    VecLoad->setAlignment(8);

  if (NegativeStride) {
    SmallVector<Constant *, 16> Indices;
    for (int i = VectorWidth - 1; i >= 0; i--)
      Indices.push_back(ConstantInt::get(Builder.getInt32Ty(), i));
    Constant *SV = llvm::ConstantVector::get(Indices);
    Value *RevVecLoad = Builder.CreateShuffleVector(
        VecLoad, VecLoad, SV, Load->getName() + "_reverse");
    return RevVecLoad;
  }

  return VecLoad;
}

Value *VectorBlockGenerator::generateStrideZeroLoad(
    ScopStmt &Stmt, LoadInst *Load, ValueMapT &BBMap,
    __isl_keep isl_id_to_ast_expr *NewAccesses) {
  auto *Pointer = Load->getPointerOperand();
  Type *VectorPtrType = getVectorPtrTy(Pointer, 1);
  Value *NewPointer =
      generateLocationAccessed(Stmt, Load, BBMap, VLTS[0], NewAccesses);
  Value *VectorPtr = Builder.CreateBitCast(NewPointer, VectorPtrType,
                                           Load->getName() + "_p_vec_p");
  LoadInst *ScalarLoad =
      Builder.CreateLoad(VectorPtr, Load->getName() + "_p_splat_one");

  if (!Aligned)
    ScalarLoad->setAlignment(8);

  Constant *SplatVector = Constant::getNullValue(
      VectorType::get(Builder.getInt32Ty(), getVectorWidth()));

  Value *VectorLoad = Builder.CreateShuffleVector(
      ScalarLoad, ScalarLoad, SplatVector, Load->getName() + "_p_splat");
  return VectorLoad;
}

Value *VectorBlockGenerator::generateUnknownStrideLoad(
    ScopStmt &Stmt, LoadInst *Load, VectorValueMapT &ScalarMaps,
    __isl_keep isl_id_to_ast_expr *NewAccesses) {
  int VectorWidth = getVectorWidth();
  auto *Pointer = Load->getPointerOperand();
  VectorType *VectorType = VectorType::get(
      dyn_cast<PointerType>(Pointer->getType())->getElementType(), VectorWidth);

  Value *Vector = UndefValue::get(VectorType);

  for (int i = 0; i < VectorWidth; i++) {
    Value *NewPointer = generateLocationAccessed(Stmt, Load, ScalarMaps[i],
                                                 VLTS[i], NewAccesses);
    Value *ScalarLoad =
        Builder.CreateLoad(NewPointer, Load->getName() + "_p_scalar_");
    Vector = Builder.CreateInsertElement(
        Vector, ScalarLoad, Builder.getInt32(i), Load->getName() + "_p_vec_");
  }

  return Vector;
}

void VectorBlockGenerator::generateLoad(
    ScopStmt &Stmt, LoadInst *Load, ValueMapT &VectorMap,
    VectorValueMapT &ScalarMaps, __isl_keep isl_id_to_ast_expr *NewAccesses) {
  if (Value *PreloadLoad = GlobalMap.lookup(Load)) {
    VectorMap[Load] = Builder.CreateVectorSplat(getVectorWidth(), PreloadLoad,
                                                Load->getName() + "_p");
    return;
  }

  if (!VectorType::isValidElementType(Load->getType())) {
    for (int i = 0; i < getVectorWidth(); i++)
      ScalarMaps[i][Load] =
          generateArrayLoad(Stmt, Load, ScalarMaps[i], VLTS[i], NewAccesses);
    return;
  }

  const MemoryAccess &Access = Stmt.getArrayAccessFor(Load);

  // Make sure we have scalar values available to access the pointer to
  // the data location.
  extractScalarValues(Load, VectorMap, ScalarMaps);

  Value *NewLoad;
  if (Access.isStrideZero(isl_map_copy(Schedule)))
    NewLoad = generateStrideZeroLoad(Stmt, Load, ScalarMaps[0], NewAccesses);
  else if (Access.isStrideOne(isl_map_copy(Schedule)))
    NewLoad = generateStrideOneLoad(Stmt, Load, ScalarMaps, NewAccesses);
  else if (Access.isStrideX(isl_map_copy(Schedule), -1))
    NewLoad = generateStrideOneLoad(Stmt, Load, ScalarMaps, NewAccesses, true);
  else
    NewLoad = generateUnknownStrideLoad(Stmt, Load, ScalarMaps, NewAccesses);

  VectorMap[Load] = NewLoad;
}

void VectorBlockGenerator::copyUnaryInst(ScopStmt &Stmt, UnaryInstruction *Inst,
                                         ValueMapT &VectorMap,
                                         VectorValueMapT &ScalarMaps) {
  int VectorWidth = getVectorWidth();
  Value *NewOperand = getVectorValue(Stmt, Inst->getOperand(0), VectorMap,
                                     ScalarMaps, getLoopForStmt(Stmt));

  assert(isa<CastInst>(Inst) && "Can not generate vector code for instruction");

  const CastInst *Cast = dyn_cast<CastInst>(Inst);
  VectorType *DestType = VectorType::get(Inst->getType(), VectorWidth);
  VectorMap[Inst] = Builder.CreateCast(Cast->getOpcode(), NewOperand, DestType);
}

void VectorBlockGenerator::copyBinaryInst(ScopStmt &Stmt, BinaryOperator *Inst,
                                          ValueMapT &VectorMap,
                                          VectorValueMapT &ScalarMaps) {
  Loop *L = getLoopForStmt(Stmt);
  Value *OpZero = Inst->getOperand(0);
  Value *OpOne = Inst->getOperand(1);

  Value *NewOpZero, *NewOpOne;
  NewOpZero = getVectorValue(Stmt, OpZero, VectorMap, ScalarMaps, L);
  NewOpOne = getVectorValue(Stmt, OpOne, VectorMap, ScalarMaps, L);

  Value *NewInst = Builder.CreateBinOp(Inst->getOpcode(), NewOpZero, NewOpOne,
                                       Inst->getName() + "p_vec");
  VectorMap[Inst] = NewInst;
}

void VectorBlockGenerator::copyStore(
    ScopStmt &Stmt, StoreInst *Store, ValueMapT &VectorMap,
    VectorValueMapT &ScalarMaps, __isl_keep isl_id_to_ast_expr *NewAccesses) {
  const MemoryAccess &Access = Stmt.getArrayAccessFor(Store);

  auto *Pointer = Store->getPointerOperand();
  Value *Vector = getVectorValue(Stmt, Store->getValueOperand(), VectorMap,
                                 ScalarMaps, getLoopForStmt(Stmt));

  // Make sure we have scalar values available to access the pointer to
  // the data location.
  extractScalarValues(Store, VectorMap, ScalarMaps);

  if (Access.isStrideOne(isl_map_copy(Schedule))) {
    Type *VectorPtrType = getVectorPtrTy(Pointer, getVectorWidth());
    Value *NewPointer = generateLocationAccessed(Stmt, Store, ScalarMaps[0],
                                                 VLTS[0], NewAccesses);

    Value *VectorPtr =
        Builder.CreateBitCast(NewPointer, VectorPtrType, "vector_ptr");
    StoreInst *Store = Builder.CreateStore(Vector, VectorPtr);

    if (!Aligned)
      Store->setAlignment(8);
  } else {
    for (unsigned i = 0; i < ScalarMaps.size(); i++) {
      Value *Scalar = Builder.CreateExtractElement(Vector, Builder.getInt32(i));
      Value *NewPointer = generateLocationAccessed(Stmt, Store, ScalarMaps[i],
                                                   VLTS[i], NewAccesses);
      Builder.CreateStore(Scalar, NewPointer);
    }
  }
}

bool VectorBlockGenerator::hasVectorOperands(const Instruction *Inst,
                                             ValueMapT &VectorMap) {
  for (Value *Operand : Inst->operands())
    if (VectorMap.count(Operand))
      return true;
  return false;
}

bool VectorBlockGenerator::extractScalarValues(const Instruction *Inst,
                                               ValueMapT &VectorMap,
                                               VectorValueMapT &ScalarMaps) {
  bool HasVectorOperand = false;
  int VectorWidth = getVectorWidth();

  for (Value *Operand : Inst->operands()) {
    ValueMapT::iterator VecOp = VectorMap.find(Operand);

    if (VecOp == VectorMap.end())
      continue;

    HasVectorOperand = true;
    Value *NewVector = VecOp->second;

    for (int i = 0; i < VectorWidth; ++i) {
      ValueMapT &SM = ScalarMaps[i];

      // If there is one scalar extracted, all scalar elements should have
      // already been extracted by the code here. So no need to check for the
      // existence of all of them.
      if (SM.count(Operand))
        break;

      SM[Operand] =
          Builder.CreateExtractElement(NewVector, Builder.getInt32(i));
    }
  }

  return HasVectorOperand;
}

void VectorBlockGenerator::copyInstScalarized(
    ScopStmt &Stmt, Instruction *Inst, ValueMapT &VectorMap,
    VectorValueMapT &ScalarMaps, __isl_keep isl_id_to_ast_expr *NewAccesses) {
  bool HasVectorOperand;
  int VectorWidth = getVectorWidth();

  HasVectorOperand = extractScalarValues(Inst, VectorMap, ScalarMaps);

  for (int VectorLane = 0; VectorLane < getVectorWidth(); VectorLane++)
    BlockGenerator::copyInstruction(Stmt, Inst, ScalarMaps[VectorLane],
                                    VLTS[VectorLane], NewAccesses);

  if (!VectorType::isValidElementType(Inst->getType()) || !HasVectorOperand)
    return;

  // Make the result available as vector value.
  VectorType *VectorType = VectorType::get(Inst->getType(), VectorWidth);
  Value *Vector = UndefValue::get(VectorType);

  for (int i = 0; i < VectorWidth; i++)
    Vector = Builder.CreateInsertElement(Vector, ScalarMaps[i][Inst],
                                         Builder.getInt32(i));

  VectorMap[Inst] = Vector;
}

int VectorBlockGenerator::getVectorWidth() { return VLTS.size(); }

void VectorBlockGenerator::copyInstruction(
    ScopStmt &Stmt, Instruction *Inst, ValueMapT &VectorMap,
    VectorValueMapT &ScalarMaps, __isl_keep isl_id_to_ast_expr *NewAccesses) {
  // Terminator instructions control the control flow. They are explicitly
  // expressed in the clast and do not need to be copied.
  if (Inst->isTerminator())
    return;

  if (canSyntheziseInStmt(Stmt, Inst))
    return;

  if (auto *Load = dyn_cast<LoadInst>(Inst)) {
    generateLoad(Stmt, Load, VectorMap, ScalarMaps, NewAccesses);
    return;
  }

  if (hasVectorOperands(Inst, VectorMap)) {
    if (auto *Store = dyn_cast<StoreInst>(Inst)) {
      // Identified as redundant by -polly-simplify.
      if (!Stmt.getArrayAccessOrNULLFor(Store))
        return;

      copyStore(Stmt, Store, VectorMap, ScalarMaps, NewAccesses);
      return;
    }

    if (auto *Unary = dyn_cast<UnaryInstruction>(Inst)) {
      copyUnaryInst(Stmt, Unary, VectorMap, ScalarMaps);
      return;
    }

    if (auto *Binary = dyn_cast<BinaryOperator>(Inst)) {
      copyBinaryInst(Stmt, Binary, VectorMap, ScalarMaps);
      return;
    }

    // Falltrough: We generate scalar instructions, if we don't know how to
    // generate vector code.
  }

  copyInstScalarized(Stmt, Inst, VectorMap, ScalarMaps, NewAccesses);
}

void VectorBlockGenerator::generateScalarVectorLoads(
    ScopStmt &Stmt, ValueMapT &VectorBlockMap) {
  for (MemoryAccess *MA : Stmt) {
    if (MA->isArrayKind() || MA->isWrite())
      continue;

    auto *Address = getOrCreateAlloca(*MA);
    Type *VectorPtrType = getVectorPtrTy(Address, 1);
    Value *VectorPtr = Builder.CreateBitCast(Address, VectorPtrType,
                                             Address->getName() + "_p_vec_p");
    auto *Val = Builder.CreateLoad(VectorPtr, Address->getName() + ".reload");
    Constant *SplatVector = Constant::getNullValue(
        VectorType::get(Builder.getInt32Ty(), getVectorWidth()));

    Value *VectorVal = Builder.CreateShuffleVector(
        Val, Val, SplatVector, Address->getName() + "_p_splat");
    VectorBlockMap[MA->getAccessValue()] = VectorVal;
  }
}

void VectorBlockGenerator::verifyNoScalarStores(ScopStmt &Stmt) {
  for (MemoryAccess *MA : Stmt) {
    if (MA->isArrayKind() || MA->isRead())
      continue;

    llvm_unreachable("Scalar stores not expected in vector loop");
  }
}

void VectorBlockGenerator::copyStmt(
    ScopStmt &Stmt, __isl_keep isl_id_to_ast_expr *NewAccesses) {
  assert(Stmt.isBlockStmt() &&
         "TODO: Only block statements can be copied by the vector block "
         "generator");

  BasicBlock *BB = Stmt.getBasicBlock();
  BasicBlock *CopyBB = SplitBlock(Builder.GetInsertBlock(),
                                  &*Builder.GetInsertPoint(), &DT, &LI);
  CopyBB->setName("polly.stmt." + BB->getName());
  Builder.SetInsertPoint(&CopyBB->front());

  // Create two maps that store the mapping from the original instructions of
  // the old basic block to their copies in the new basic block. Those maps
  // are basic block local.
  //
  // As vector code generation is supported there is one map for scalar values
  // and one for vector values.
  //
  // In case we just do scalar code generation, the vectorMap is not used and
  // the scalarMap has just one dimension, which contains the mapping.
  //
  // In case vector code generation is done, an instruction may either appear
  // in the vector map once (as it is calculating >vectorwidth< values at a
  // time. Or (if the values are calculated using scalar operations), it
  // appears once in every dimension of the scalarMap.
  VectorValueMapT ScalarBlockMap(getVectorWidth());
  ValueMapT VectorBlockMap;

  generateScalarVectorLoads(Stmt, VectorBlockMap);

  for (Instruction &Inst : *BB)
    copyInstruction(Stmt, &Inst, VectorBlockMap, ScalarBlockMap, NewAccesses);

  verifyNoScalarStores(Stmt);
}

BasicBlock *RegionGenerator::repairDominance(BasicBlock *BB,
                                             BasicBlock *BBCopy) {

  BasicBlock *BBIDom = DT.getNode(BB)->getIDom()->getBlock();
  BasicBlock *BBCopyIDom = BlockMap.lookup(BBIDom);

  if (BBCopyIDom)
    DT.changeImmediateDominator(BBCopy, BBCopyIDom);

  return BBCopyIDom;
}

// This is to determine whether an llvm::Value (defined in @p BB) is usable when
// leaving a subregion. The straight-forward DT.dominates(BB, R->getExitBlock())
// does not work in cases where the exit block has edges from outside the
// region. In that case the llvm::Value would never be usable in in the exit
// block. The RegionGenerator however creates an new exit block ('ExitBBCopy')
// for the subregion's exiting edges only. We need to determine whether an
// llvm::Value is usable in there. We do this by checking whether it dominates
// all exiting blocks individually.
static bool isDominatingSubregionExit(const DominatorTree &DT, Region *R,
                                      BasicBlock *BB) {
  for (auto ExitingBB : predecessors(R->getExit())) {
    // Check for non-subregion incoming edges.
    if (!R->contains(ExitingBB))
      continue;

    if (!DT.dominates(BB, ExitingBB))
      return false;
  }

  return true;
}

// Find the direct dominator of the subregion's exit block if the subregion was
// simplified.
static BasicBlock *findExitDominator(DominatorTree &DT, Region *R) {
  BasicBlock *Common = nullptr;
  for (auto ExitingBB : predecessors(R->getExit())) {
    // Check for non-subregion incoming edges.
    if (!R->contains(ExitingBB))
      continue;

    // First exiting edge.
    if (!Common) {
      Common = ExitingBB;
      continue;
    }

    Common = DT.findNearestCommonDominator(Common, ExitingBB);
  }

  assert(Common && R->contains(Common));
  return Common;
}

void RegionGenerator::copyStmt(ScopStmt &Stmt, LoopToScevMapT &LTS,
                               isl_id_to_ast_expr *IdToAstExp) {
  assert(Stmt.isRegionStmt() &&
         "Only region statements can be copied by the region generator");

  // Forget all old mappings.
  BlockMap.clear();
  RegionMaps.clear();
  IncompletePHINodeMap.clear();

  // Collection of all values related to this subregion.
  ValueMapT ValueMap;

  // The region represented by the statement.
  Region *R = Stmt.getRegion();

  // Create a dedicated entry for the region where we can reload all demoted
  // inputs.
  BasicBlock *EntryBB = R->getEntry();
  BasicBlock *EntryBBCopy = SplitBlock(Builder.GetInsertBlock(),
                                       &*Builder.GetInsertPoint(), &DT, &LI);
  EntryBBCopy->setName("polly.stmt." + EntryBB->getName() + ".entry");
  Builder.SetInsertPoint(&EntryBBCopy->front());

  ValueMapT &EntryBBMap = RegionMaps[EntryBBCopy];
  generateScalarLoads(Stmt, LTS, EntryBBMap, IdToAstExp);

  for (auto PI = pred_begin(EntryBB), PE = pred_end(EntryBB); PI != PE; ++PI)
    if (!R->contains(*PI))
      BlockMap[*PI] = EntryBBCopy;

  // Iterate over all blocks in the region in a breadth-first search.
  std::deque<BasicBlock *> Blocks;
  SmallSetVector<BasicBlock *, 8> SeenBlocks;
  Blocks.push_back(EntryBB);
  SeenBlocks.insert(EntryBB);

  while (!Blocks.empty()) {
    BasicBlock *BB = Blocks.front();
    Blocks.pop_front();

    // First split the block and update dominance information.
    BasicBlock *BBCopy = splitBB(BB);
    BasicBlock *BBCopyIDom = repairDominance(BB, BBCopy);

    // Get the mapping for this block and initialize it with either the scalar
    // loads from the generated entering block (which dominates all blocks of
    // this subregion) or the maps of the immediate dominator, if part of the
    // subregion. The latter necessarily includes the former.
    ValueMapT *InitBBMap;
    if (BBCopyIDom) {
      assert(RegionMaps.count(BBCopyIDom));
      InitBBMap = &RegionMaps[BBCopyIDom];
    } else
      InitBBMap = &EntryBBMap;
    auto Inserted = RegionMaps.insert(std::make_pair(BBCopy, *InitBBMap));
    ValueMapT &RegionMap = Inserted.first->second;

    // Copy the block with the BlockGenerator.
    Builder.SetInsertPoint(&BBCopy->front());
    copyBB(Stmt, BB, BBCopy, RegionMap, LTS, IdToAstExp);

    // In order to remap PHI nodes we store also basic block mappings.
    BlockMap[BB] = BBCopy;

    // Add values to incomplete PHI nodes waiting for this block to be copied.
    for (const PHINodePairTy &PHINodePair : IncompletePHINodeMap[BB])
      addOperandToPHI(Stmt, PHINodePair.first, PHINodePair.second, BB, LTS);
    IncompletePHINodeMap[BB].clear();

    // And continue with new successors inside the region.
    for (auto SI = succ_begin(BB), SE = succ_end(BB); SI != SE; SI++)
      if (R->contains(*SI) && SeenBlocks.insert(*SI))
        Blocks.push_back(*SI);

    // Remember value in case it is visible after this subregion.
    if (isDominatingSubregionExit(DT, R, BB))
      ValueMap.insert(RegionMap.begin(), RegionMap.end());
  }

  // Now create a new dedicated region exit block and add it to the region map.
  BasicBlock *ExitBBCopy = SplitBlock(Builder.GetInsertBlock(),
                                      &*Builder.GetInsertPoint(), &DT, &LI);
  ExitBBCopy->setName("polly.stmt." + R->getExit()->getName() + ".exit");
  BlockMap[R->getExit()] = ExitBBCopy;

  BasicBlock *ExitDomBBCopy = BlockMap.lookup(findExitDominator(DT, R));
  assert(ExitDomBBCopy &&
         "Common exit dominator must be within region; at least the entry node "
         "must match");
  DT.changeImmediateDominator(ExitBBCopy, ExitDomBBCopy);

  // As the block generator doesn't handle control flow we need to add the
  // region control flow by hand after all blocks have been copied.
  for (BasicBlock *BB : SeenBlocks) {

    BasicBlock *BBCopy = BlockMap[BB];
    TerminatorInst *TI = BB->getTerminator();
    if (isa<UnreachableInst>(TI)) {
      while (!BBCopy->empty())
        BBCopy->begin()->eraseFromParent();
      new UnreachableInst(BBCopy->getContext(), BBCopy);
      continue;
    }

    Instruction *BICopy = BBCopy->getTerminator();

    ValueMapT &RegionMap = RegionMaps[BBCopy];
    RegionMap.insert(BlockMap.begin(), BlockMap.end());

    Builder.SetInsertPoint(BICopy);
    copyInstScalar(Stmt, TI, RegionMap, LTS);
    BICopy->eraseFromParent();
  }

  // Add counting PHI nodes to all loops in the region that can be used as
  // replacement for SCEVs refering to the old loop.
  for (BasicBlock *BB : SeenBlocks) {
    Loop *L = LI.getLoopFor(BB);
    if (L == nullptr || L->getHeader() != BB || !R->contains(L))
      continue;

    BasicBlock *BBCopy = BlockMap[BB];
    Value *NullVal = Builder.getInt32(0);
    PHINode *LoopPHI =
        PHINode::Create(Builder.getInt32Ty(), 2, "polly.subregion.iv");
    Instruction *LoopPHIInc = BinaryOperator::CreateAdd(
        LoopPHI, Builder.getInt32(1), "polly.subregion.iv.inc");
    LoopPHI->insertBefore(&BBCopy->front());
    LoopPHIInc->insertBefore(BBCopy->getTerminator());

    for (auto *PredBB : make_range(pred_begin(BB), pred_end(BB))) {
      if (!R->contains(PredBB))
        continue;
      if (L->contains(PredBB))
        LoopPHI->addIncoming(LoopPHIInc, BlockMap[PredBB]);
      else
        LoopPHI->addIncoming(NullVal, BlockMap[PredBB]);
    }

    for (auto *PredBBCopy : make_range(pred_begin(BBCopy), pred_end(BBCopy)))
      if (LoopPHI->getBasicBlockIndex(PredBBCopy) < 0)
        LoopPHI->addIncoming(NullVal, PredBBCopy);

    LTS[L] = SE.getUnknown(LoopPHI);
  }

  // Continue generating code in the exit block.
  Builder.SetInsertPoint(&*ExitBBCopy->getFirstInsertionPt());

  // Write values visible to other statements.
  generateScalarStores(Stmt, LTS, ValueMap, IdToAstExp);
  BlockMap.clear();
  RegionMaps.clear();
  IncompletePHINodeMap.clear();
}

PHINode *RegionGenerator::buildExitPHI(MemoryAccess *MA, LoopToScevMapT &LTS,
                                       ValueMapT &BBMap, Loop *L) {
  ScopStmt *Stmt = MA->getStatement();
  Region *SubR = Stmt->getRegion();
  auto Incoming = MA->getIncoming();

  PollyIRBuilder::InsertPointGuard IPGuard(Builder);
  PHINode *OrigPHI = cast<PHINode>(MA->getAccessInstruction());
  BasicBlock *NewSubregionExit = Builder.GetInsertBlock();

  // This can happen if the subregion is simplified after the ScopStmts
  // have been created; simplification happens as part of CodeGeneration.
  if (OrigPHI->getParent() != SubR->getExit()) {
    BasicBlock *FormerExit = SubR->getExitingBlock();
    if (FormerExit)
      NewSubregionExit = BlockMap.lookup(FormerExit);
  }

  PHINode *NewPHI = PHINode::Create(OrigPHI->getType(), Incoming.size(),
                                    "polly." + OrigPHI->getName(),
                                    NewSubregionExit->getFirstNonPHI());

  // Add the incoming values to the PHI.
  for (auto &Pair : Incoming) {
    BasicBlock *OrigIncomingBlock = Pair.first;
    BasicBlock *NewIncomingBlock = BlockMap.lookup(OrigIncomingBlock);
    Builder.SetInsertPoint(NewIncomingBlock->getTerminator());
    assert(RegionMaps.count(NewIncomingBlock));
    ValueMapT *LocalBBMap = &RegionMaps[NewIncomingBlock];

    Value *OrigIncomingValue = Pair.second;
    Value *NewIncomingValue =
        getNewValue(*Stmt, OrigIncomingValue, *LocalBBMap, LTS, L);
    NewPHI->addIncoming(NewIncomingValue, NewIncomingBlock);
  }

  return NewPHI;
}

Value *RegionGenerator::getExitScalar(MemoryAccess *MA, LoopToScevMapT &LTS,
                                      ValueMapT &BBMap) {
  ScopStmt *Stmt = MA->getStatement();

  // TODO: Add some test cases that ensure this is really the right choice.
  Loop *L = LI.getLoopFor(Stmt->getRegion()->getExit());

  if (MA->isAnyPHIKind()) {
    auto Incoming = MA->getIncoming();
    assert(!Incoming.empty() &&
           "PHI WRITEs must have originate from at least one incoming block");

    // If there is only one incoming value, we do not need to create a PHI.
    if (Incoming.size() == 1) {
      Value *OldVal = Incoming[0].second;
      return getNewValue(*Stmt, OldVal, BBMap, LTS, L);
    }

    return buildExitPHI(MA, LTS, BBMap, L);
  }

  // MemoryKind::Value accesses leaving the subregion must dominate the exit
  // block; just pass the copied value.
  Value *OldVal = MA->getAccessValue();
  return getNewValue(*Stmt, OldVal, BBMap, LTS, L);
}

void RegionGenerator::generateScalarStores(
    ScopStmt &Stmt, LoopToScevMapT &LTS, ValueMapT &BBMap,
    __isl_keep isl_id_to_ast_expr *NewAccesses) {
  assert(Stmt.getRegion() &&
         "Block statements need to use the generateScalarStores() "
         "function in the BlockGenerator");

  for (MemoryAccess *MA : Stmt) {
    if (MA->isOriginalArrayKind() || MA->isRead())
      continue;

    Value *NewVal = getExitScalar(MA, LTS, BBMap);
    Value *Address =
        getImplicitAddress(*MA, getLoopForStmt(Stmt), LTS, BBMap, NewAccesses);
    assert((!isa<Instruction>(NewVal) ||
            DT.dominates(cast<Instruction>(NewVal)->getParent(),
                         Builder.GetInsertBlock())) &&
           "Domination violation");
    assert((!isa<Instruction>(Address) ||
            DT.dominates(cast<Instruction>(Address)->getParent(),
                         Builder.GetInsertBlock())) &&
           "Domination violation");
    Builder.CreateStore(NewVal, Address);
  }
}

void RegionGenerator::addOperandToPHI(ScopStmt &Stmt, PHINode *PHI,
                                      PHINode *PHICopy, BasicBlock *IncomingBB,
                                      LoopToScevMapT &LTS) {
  // If the incoming block was not yet copied mark this PHI as incomplete.
  // Once the block will be copied the incoming value will be added.
  BasicBlock *BBCopy = BlockMap[IncomingBB];
  if (!BBCopy) {
    assert(Stmt.contains(IncomingBB) &&
           "Bad incoming block for PHI in non-affine region");
    IncompletePHINodeMap[IncomingBB].push_back(std::make_pair(PHI, PHICopy));
    return;
  }

  assert(RegionMaps.count(BBCopy) && "Incoming PHI block did not have a BBMap");
  ValueMapT &BBCopyMap = RegionMaps[BBCopy];

  Value *OpCopy = nullptr;

  if (Stmt.contains(IncomingBB)) {
    Value *Op = PHI->getIncomingValueForBlock(IncomingBB);

    // If the current insert block is different from the PHIs incoming block
    // change it, otherwise do not.
    auto IP = Builder.GetInsertPoint();
    if (IP->getParent() != BBCopy)
      Builder.SetInsertPoint(BBCopy->getTerminator());
    OpCopy = getNewValue(Stmt, Op, BBCopyMap, LTS, getLoopForStmt(Stmt));
    if (IP->getParent() != BBCopy)
      Builder.SetInsertPoint(&*IP);
  } else {
    // All edges from outside the non-affine region become a single edge
    // in the new copy of the non-affine region. Make sure to only add the
    // corresponding edge the first time we encounter a basic block from
    // outside the non-affine region.
    if (PHICopy->getBasicBlockIndex(BBCopy) >= 0)
      return;

    // Get the reloaded value.
    OpCopy = getNewValue(Stmt, PHI, BBCopyMap, LTS, getLoopForStmt(Stmt));
  }

  assert(OpCopy && "Incoming PHI value was not copied properly");
  assert(BBCopy && "Incoming PHI block was not copied properly");
  PHICopy->addIncoming(OpCopy, BBCopy);
}

void RegionGenerator::copyPHIInstruction(ScopStmt &Stmt, PHINode *PHI,
                                         ValueMapT &BBMap,
                                         LoopToScevMapT &LTS) {
  unsigned NumIncoming = PHI->getNumIncomingValues();
  PHINode *PHICopy =
      Builder.CreatePHI(PHI->getType(), NumIncoming, "polly." + PHI->getName());
  PHICopy->moveBefore(PHICopy->getParent()->getFirstNonPHI());
  BBMap[PHI] = PHICopy;

  for (BasicBlock *IncomingBB : PHI->blocks())
    addOperandToPHI(Stmt, PHI, PHICopy, IncomingBB, LTS);
}
