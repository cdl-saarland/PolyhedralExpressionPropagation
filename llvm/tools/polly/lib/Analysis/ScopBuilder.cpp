//===- ScopBuilder.cpp ---------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create a polyhedral description for a static control flow region.
//
// The pass creates a polyhedral description of the Scops detected by the SCoP
// detection derived from their LLVM-IR code.
//
//===----------------------------------------------------------------------===//

#include "polly/ScopBuilder.h"
#include "polly/Options.h"
#include "polly/Support/GICHelper.h"
#include "polly/Support/SCEVValidator.h"
#include "polly/Support/VirtualInstruction.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;
using namespace polly;

#define DEBUG_TYPE "polly-scops"

STATISTIC(ScopFound, "Number of valid Scops");
STATISTIC(RichScopFound, "Number of Scops containing a loop");
STATISTIC(InfeasibleScops,
          "Number of SCoPs with statically infeasible context.");

static cl::opt<bool> OneWrite("polly-one-write", cl::desc(""), cl::Hidden,
                              cl::ZeroOrMore, cl::init(false),
                              cl::cat(PollyCategory));

static cl::opt<bool> ModelReadOnlyScalars(
    "polly-analyze-read-only-scalars",
    cl::desc("Model read-only scalar values in the scop description"),
    cl::Hidden, cl::ZeroOrMore, cl::init(true), cl::cat(PollyCategory));

void ScopBuilder::buildPHIAccesses(PHINode *PHI, Region *NonAffineSubRegion,
                                   bool IsExitBlock) {

  // PHI nodes that are in the exit block of the region, hence if IsExitBlock is
  // true, are not modeled as ordinary PHI nodes as they are not part of the
  // region. However, we model the operands in the predecessor blocks that are
  // part of the region as regular scalar accesses.

  // If we can synthesize a PHI we can skip it, however only if it is in
  // the region. If it is not it can only be in the exit block of the region.
  // In this case we model the operands but not the PHI itself.
  auto *Scope = LI.getLoopFor(PHI->getParent());
  if (!IsExitBlock && canSynthesize(PHI, *scop, &SE, Scope))
    return;

  // PHI nodes are modeled as if they had been demoted prior to the SCoP
  // detection. Hence, the PHI is a load of a new memory location in which the
  // incoming value was written at the end of the incoming basic block.
  bool OnlyNonAffineSubRegionOperands = true;
  for (unsigned u = 0; u < PHI->getNumIncomingValues(); u++) {
    Value *Op = PHI->getIncomingValue(u);
    BasicBlock *OpBB = PHI->getIncomingBlock(u);

    // Do not build PHI dependences inside a non-affine subregion, but make
    // sure that the necessary scalar values are still made available.
    if (NonAffineSubRegion && NonAffineSubRegion->contains(OpBB)) {
      auto *OpInst = dyn_cast<Instruction>(Op);
      if (!OpInst || !NonAffineSubRegion->contains(OpInst))
        ensureValueRead(PHI, Op, OpBB);
      continue;
    }

    OnlyNonAffineSubRegionOperands = false;
    ensurePHIWrite(PHI, OpBB, Op, IsExitBlock);
  }

  if (!OnlyNonAffineSubRegionOperands && !IsExitBlock) {
    addPHIReadAccess(PHI);
  }
}

void ScopBuilder::buildScalarDependences(Instruction *Inst) {
  assert(!isa<PHINode>(Inst));

  // Pull-in required operands.
  for (Use &Op : Inst->operands())
    ensureValueRead(Inst, Op.get(), Inst->getParent());
}

void ScopBuilder::buildEscapingDependences(Instruction *Inst) {
  // Check for uses of this instruction outside the scop. Because we do not
  // iterate over such instructions and therefore did not "ensure" the existence
  // of a write, we must determine such use here.
  for (Use &U : Inst->uses()) {
    Instruction *UI = dyn_cast<Instruction>(U.getUser());
    if (!UI)
      continue;

    BasicBlock *UseParent = getUseBlock(U);
    BasicBlock *UserParent = UI->getParent();

    // An escaping value is either used by an instruction not within the scop,
    // or (when the scop region's exit needs to be simplified) by a PHI in the
    // scop's exit block. This is because region simplification before code
    // generation inserts new basic blocks before the PHI such that its incoming
    // blocks are not in the scop anymore.
    if (!scop->contains(UseParent) ||
        (isa<PHINode>(UI) && scop->isExit(UserParent) &&
         scop->hasSingleExitEdge())) {
      // At least one escaping use found.
      ensureValueWrite(Inst);
      break;
    }
  }
}

bool ScopBuilder::buildAccessMultiDimFixed(MemAccInst Inst, ScopStmt *Stmt) {
  Value *Val = Inst.getValueOperand();
  Type *ElementType = Val->getType();
  Value *Address = Inst.getPointerOperand();
  const SCEV *AccessFunction =
      SE.getSCEVAtScope(Address, LI.getLoopFor(Inst->getParent()));
  const SCEVUnknown *BasePointer =
      dyn_cast<SCEVUnknown>(SE.getPointerBase(AccessFunction));
  enum MemoryAccess::AccessType AccType =
      isa<LoadInst>(Inst) ? MemoryAccess::READ : MemoryAccess::MUST_WRITE;

  if (auto *BitCast = dyn_cast<BitCastInst>(Address)) {
    auto *Src = BitCast->getOperand(0);
    auto *SrcTy = Src->getType();
    auto *DstTy = BitCast->getType();
    // Do not try to delinearize non-sized (opaque) pointers.
    if ((SrcTy->isPointerTy() && !SrcTy->getPointerElementType()->isSized()) ||
        (DstTy->isPointerTy() && !DstTy->getPointerElementType()->isSized())) {
      return false;
    }
    if (SrcTy->isPointerTy() && DstTy->isPointerTy() &&
        DL.getTypeAllocSize(SrcTy->getPointerElementType()) ==
            DL.getTypeAllocSize(DstTy->getPointerElementType()))
      Address = Src;
  }

  auto *GEP = dyn_cast<GetElementPtrInst>(Address);
  if (!GEP)
    return false;

  std::vector<const SCEV *> Subscripts;
  std::vector<int> Sizes;
  std::tie(Subscripts, Sizes) = getIndexExpressionsFromGEP(GEP, SE);
  auto *BasePtr = GEP->getOperand(0);

  if (auto *BasePtrCast = dyn_cast<BitCastInst>(BasePtr))
    BasePtr = BasePtrCast->getOperand(0);

  // Check for identical base pointers to ensure that we do not miss index
  // offsets that have been added before this GEP is applied.
  if (BasePtr != BasePointer->getValue())
    return false;

  std::vector<const SCEV *> SizesSCEV;

  const InvariantLoadsSetTy &ScopRIL = scop->getRequiredInvariantLoads();

  Loop *SurroundingLoop = Stmt->getSurroundingLoop();
  for (auto *Subscript : Subscripts) {
    InvariantLoadsSetTy AccessILS;
    if (!isAffineExpr(&scop->getRegion(), SurroundingLoop, Subscript, SE,
                      &AccessILS))
      return false;

    for (LoadInst *LInst : AccessILS)
      if (!ScopRIL.count(LInst))
        return false;
  }

  if (Sizes.empty())
    return false;

  SizesSCEV.push_back(nullptr);

  for (auto V : Sizes)
    SizesSCEV.push_back(SE.getSCEV(
        ConstantInt::get(IntegerType::getInt64Ty(BasePtr->getContext()), V)));

  addArrayAccess(Inst, AccType, BasePointer->getValue(), ElementType, true,
                 Subscripts, SizesSCEV, Val);
  return true;
}

bool ScopBuilder::buildAccessMultiDimParam(MemAccInst Inst, ScopStmt *Stmt) {
  if (!PollyDelinearize)
    return false;

  Value *Address = Inst.getPointerOperand();
  Value *Val = Inst.getValueOperand();
  Type *ElementType = Val->getType();
  unsigned ElementSize = DL.getTypeAllocSize(ElementType);
  enum MemoryAccess::AccessType AccType =
      isa<LoadInst>(Inst) ? MemoryAccess::READ : MemoryAccess::MUST_WRITE;

  const SCEV *AccessFunction =
      SE.getSCEVAtScope(Address, LI.getLoopFor(Inst->getParent()));
  const SCEVUnknown *BasePointer =
      dyn_cast<SCEVUnknown>(SE.getPointerBase(AccessFunction));

  assert(BasePointer && "Could not find base pointer");

  auto &InsnToMemAcc = scop->getInsnToMemAccMap();
  auto AccItr = InsnToMemAcc.find(Inst);
  if (AccItr == InsnToMemAcc.end())
    return false;

  std::vector<const SCEV *> Sizes = {nullptr};

  Sizes.insert(Sizes.end(), AccItr->second.Shape->DelinearizedSizes.begin(),
               AccItr->second.Shape->DelinearizedSizes.end());
  if (Sizes.size() == 1)
    return false;
  // Remove the element size. This information is already provided by the
  // ElementSize parameter. In case the element size of this access and the
  // element size used for delinearization differs the delinearization is
  // incorrect. Hence, we invalidate the scop.
  //
  // TODO: Handle delinearization with differing element sizes.
  auto DelinearizedSize =
      cast<SCEVConstant>(Sizes.back())->getAPInt().getSExtValue();
  Sizes.pop_back();
  if (ElementSize != DelinearizedSize)
    scop->invalidate(DELINEARIZATION, Inst->getDebugLoc());

  bool IsAffine = !AccItr->second.DelinearizedSubscripts.empty();
  if (!IsAffine && AccType == MemoryAccess::MUST_WRITE)
    AccType = MemoryAccess::MAY_WRITE;
  addArrayAccess(Inst, AccType, BasePointer->getValue(), ElementType, IsAffine,
                 AccItr->second.DelinearizedSubscripts, Sizes, Val);
  return true;
}

bool ScopBuilder::buildAccessMemIntrinsic(MemAccInst Inst, ScopStmt *Stmt) {
  auto *MemIntr = dyn_cast_or_null<MemIntrinsic>(Inst);

  if (MemIntr == nullptr)
    return false;

  auto *L = LI.getLoopFor(Inst->getParent());
  auto *LengthVal = SE.getSCEVAtScope(MemIntr->getLength(), L);
  assert(LengthVal);

  // Check if the length val is actually affine or if we overapproximate it
  InvariantLoadsSetTy AccessILS;
  const InvariantLoadsSetTy &ScopRIL = scop->getRequiredInvariantLoads();

  Loop *SurroundingLoop = Stmt->getSurroundingLoop();
  bool LengthIsAffine = isAffineExpr(&scop->getRegion(), SurroundingLoop,
                                     LengthVal, SE, &AccessILS);
  for (LoadInst *LInst : AccessILS)
    if (!ScopRIL.count(LInst))
      LengthIsAffine = false;
  if (!LengthIsAffine)
    LengthVal = nullptr;

  auto *DestPtrVal = MemIntr->getDest();
  assert(DestPtrVal);

  auto *DestAccFunc = SE.getSCEVAtScope(DestPtrVal, L);
  assert(DestAccFunc);
  // Ignore accesses to "NULL".
  // TODO: We could use this to optimize the region further, e.g., intersect
  //       the context with
  //          isl_set_complement(isl_set_params(getDomain()))
  //       as we know it would be undefined to execute this instruction anyway.
  if (DestAccFunc->isZero())
    return true;

  auto *DestPtrSCEV = dyn_cast<SCEVUnknown>(SE.getPointerBase(DestAccFunc));
  assert(DestPtrSCEV);
  DestAccFunc = SE.getMinusSCEV(DestAccFunc, DestPtrSCEV);
  addArrayAccess(Inst, MemoryAccess::MUST_WRITE, DestPtrSCEV->getValue(),
                 IntegerType::getInt8Ty(DestPtrVal->getContext()),
                 LengthIsAffine, {DestAccFunc, LengthVal}, {nullptr},
                 Inst.getValueOperand());

  auto *MemTrans = dyn_cast<MemTransferInst>(MemIntr);
  if (!MemTrans)
    return true;

  auto *SrcPtrVal = MemTrans->getSource();
  assert(SrcPtrVal);

  auto *SrcAccFunc = SE.getSCEVAtScope(SrcPtrVal, L);
  assert(SrcAccFunc);
  // Ignore accesses to "NULL".
  // TODO: See above TODO
  if (SrcAccFunc->isZero())
    return true;

  auto *SrcPtrSCEV = dyn_cast<SCEVUnknown>(SE.getPointerBase(SrcAccFunc));
  assert(SrcPtrSCEV);
  SrcAccFunc = SE.getMinusSCEV(SrcAccFunc, SrcPtrSCEV);
  addArrayAccess(Inst, MemoryAccess::READ, SrcPtrSCEV->getValue(),
                 IntegerType::getInt8Ty(SrcPtrVal->getContext()),
                 LengthIsAffine, {SrcAccFunc, LengthVal}, {nullptr},
                 Inst.getValueOperand());

  return true;
}

bool ScopBuilder::buildAccessCallInst(MemAccInst Inst, ScopStmt *Stmt) {
  auto *CI = dyn_cast_or_null<CallInst>(Inst);

  if (CI == nullptr)
    return false;

  if (CI->doesNotAccessMemory() || isIgnoredIntrinsic(CI))
    return true;

  auto *CalledFunction = CI->getCalledFunction();
  if (CalledFunction->getName().equals("_ZSt3expf"))
    return true;

  auto *AF = SE.getConstant(IntegerType::getInt64Ty(CI->getContext()), 0);
  if (CalledFunction->getName().equals("fprintf") ||
      CalledFunction->getName().equals("fwrite") ||
      CalledFunction->getName().equals("fputc")) {
    Loop *L = LI.getLoopFor(CI->getParent());
    unsigned Idx = CalledFunction->getName().equals("fprintf")
                       ? 0
                       : CI->getNumArgOperands() - 1;
    auto *ArgSCEV = SE.getSCEVAtScope(CI->getArgOperand(Idx), L);
    ArgSCEV = scop->getRepresentingInvariantLoadSCEV(ArgSCEV);
    addArrayAccess(Inst, MemoryAccess::MUST_WRITE,
                   cast<SCEVUnknown>(ArgSCEV)->getValue(), ArgSCEV->getType(),
                   false, {AF}, {nullptr}, CI);
    return true;
  }

  bool ReadOnly = false;
  switch (AA.getModRefBehavior(CalledFunction)) {
  case FMRB_UnknownModRefBehavior:
    llvm_unreachable("Unknown mod ref behaviour cannot be represented.");
  case FMRB_DoesNotAccessMemory:
    return true;
  case FMRB_DoesNotReadMemory:
  case FMRB_OnlyAccessesInaccessibleMem:
  case FMRB_OnlyAccessesInaccessibleOrArgMem:
    return false;
  case FMRB_OnlyReadsMemory:
    GlobalReads.push_back(CI);
    return true;
  case FMRB_OnlyReadsArgumentPointees:
    ReadOnly = true;
  // Fall through
  case FMRB_OnlyAccessesArgumentPointees:
    auto AccType = ReadOnly ? MemoryAccess::READ : MemoryAccess::MAY_WRITE;
    Loop *L = LI.getLoopFor(Inst->getParent());
    for (const auto &Arg : CI->arg_operands()) {
      if (!Arg->getType()->isPointerTy())
        continue;

      auto *ArgSCEV = SE.getSCEVAtScope(Arg, L);
      if (ArgSCEV->isZero())
        continue;

      auto *ArgBasePtr = cast<SCEVUnknown>(SE.getPointerBase(ArgSCEV));
      addArrayAccess(Inst, AccType, ArgBasePtr->getValue(),
                     ArgBasePtr->getType(), false, {AF}, {nullptr}, CI);
    }
    return true;
  }

  return true;
}

void ScopBuilder::buildAccessSingleDim(MemAccInst Inst, ScopStmt *Stmt) {
  Value *Address = Inst.getPointerOperand();
  Value *Val = Inst.getValueOperand();
  Type *ElementType = Val->getType();
  enum MemoryAccess::AccessType AccType =
      isa<LoadInst>(Inst) ? MemoryAccess::READ : MemoryAccess::MUST_WRITE;

  const SCEV *AccessFunction =
      SE.getSCEVAtScope(Address, LI.getLoopFor(Inst->getParent()));
  const SCEVUnknown *BasePointer =
      dyn_cast<SCEVUnknown>(SE.getPointerBase(AccessFunction));

  assert(BasePointer && "Could not find base pointer");
  AccessFunction = SE.getMinusSCEV(AccessFunction, BasePointer);

  // Check if the access depends on a loop contained in a non-affine subregion.
  bool isVariantInNonAffineLoop = false;
  SetVector<const Loop *> Loops;
  findLoops(AccessFunction, Loops);
  for (const Loop *L : Loops)
    if (Stmt->contains(L)) {
      isVariantInNonAffineLoop = true;
      break;
    }

  InvariantLoadsSetTy AccessILS;

  Loop *SurroundingLoop = Stmt->getSurroundingLoop();
  bool IsAffine = !isVariantInNonAffineLoop &&
                  isAffineExpr(&scop->getRegion(), SurroundingLoop,
                               AccessFunction, SE, &AccessILS);

  const InvariantLoadsSetTy &ScopRIL = scop->getRequiredInvariantLoads();
  for (LoadInst *LInst : AccessILS)
    if (!ScopRIL.count(LInst))
      IsAffine = false;

  if (!IsAffine && AccType == MemoryAccess::MUST_WRITE)
    AccType = MemoryAccess::MAY_WRITE;

  addArrayAccess(Inst, AccType, BasePointer->getValue(), ElementType, IsAffine,
                 {AccessFunction}, {nullptr}, Val);
}

void ScopBuilder::buildMemoryAccess(MemAccInst Inst, ScopStmt *Stmt) {

  if (buildAccessMemIntrinsic(Inst, Stmt))
    return;

  if (buildAccessCallInst(Inst, Stmt))
    return;

  if (buildAccessMultiDimFixed(Inst, Stmt))
    return;

  if (buildAccessMultiDimParam(Inst, Stmt))
    return;

  buildAccessSingleDim(Inst, Stmt);
}

void ScopBuilder::buildAccessFunctions(Region &SR) {

  if (scop->isNonAffineSubRegion(&SR)) {
    for (BasicBlock *BB : SR.blocks())
      buildAccessFunctions(*BB, &SR);
    return;
  }

  for (auto I = SR.element_begin(), E = SR.element_end(); I != E; ++I)
    if (I->isSubRegion())
      buildAccessFunctions(*I->getNodeAs<Region>());
    else
      buildAccessFunctions(*I->getNodeAs<BasicBlock>());
}

void ScopBuilder::buildStmts(Region &SR) {

  if (scop->isNonAffineSubRegion(&SR)) {
    Loop *SurroundingLoop = LI.getLoopFor(SR.getEntry());
    auto &BoxedLoops = scop->getBoxedLoops();
    while (BoxedLoops.count(SurroundingLoop))
      SurroundingLoop = SurroundingLoop->getParentLoop();
    scop->addScopStmt(&SR, SurroundingLoop);
    return;
  }

  for (auto I = SR.element_begin(), E = SR.element_end(); I != E; ++I)
    if (I->isSubRegion())
      buildStmts(*I->getNodeAs<Region>());
    else {
      Loop *SurroundingLoop = LI.getLoopFor(I->getNodeAs<BasicBlock>());
      scop->addScopStmt(I->getNodeAs<BasicBlock>(), SurroundingLoop);
    }
}

void ScopBuilder::buildAccessFunctions(BasicBlock &BB,
                                       Region *NonAffineSubRegion,
                                       bool IsExitBlock) {
  // We do not build access functions for error blocks, as they may contain
  // instructions we can not model.
  if (isErrorBlock(BB, scop->getRegion(), LI, DT) && !IsExitBlock)
    return;

  ScopStmt *Stmt = scop->getStmtFor(&BB);

  for (Instruction &Inst : BB) {
    PHINode *PHI = dyn_cast<PHINode>(&Inst);
    if (PHI)
      buildPHIAccesses(PHI, NonAffineSubRegion, IsExitBlock);

    // For the exit block we stop modeling after the last PHI node.
    if (!PHI && IsExitBlock)
      break;

    if (auto MemInst = MemAccInst::dyn_cast(Inst)) {
      assert(Stmt && "Cannot build access function in non-existing statement");
      buildMemoryAccess(MemInst, Stmt);
    }

    if (isIgnoredIntrinsic(&Inst))
      continue;

    // PHI nodes have already been modeled above and TerminatorInsts that are
    // not part of a non-affine subregion are fully modeled and regenerated
    // from the polyhedral domains. Hence, they do not need to be modeled as
    // explicit data dependences.
    if (!PHI && (!isa<TerminatorInst>(&Inst) || NonAffineSubRegion))
      buildScalarDependences(&Inst);

    if (!IsExitBlock)
      buildEscapingDependences(&Inst);
  }
}

MemoryAccess *ScopBuilder::addMemoryAccess(
    BasicBlock *BB, Instruction *Inst, MemoryAccess::AccessType AccType,
    Value *BaseAddress, Type *ElementType, bool Affine, Value *AccessValue,
    ArrayRef<const SCEV *> Subscripts, ArrayRef<const SCEV *> Sizes,
    MemoryKind Kind) {
  ScopStmt *Stmt = scop->getStmtFor(BB);

  // Do not create a memory access for anything not in the SCoP. It would be
  // ignored anyway.
  if (!Stmt)
    return nullptr;

  Value *BaseAddr = BaseAddress;
  std::string BaseName = getIslCompatibleName("MemRef_", BaseAddr, 0, "", true);

  bool isKnownMustAccess = false;

  // Accesses in single-basic block statements are always excuted.
  if (Stmt->isBlockStmt())
    isKnownMustAccess = true;

  if (Stmt->isRegionStmt()) {
    // Accesses that dominate the exit block of a non-affine region are always
    // executed. In non-affine regions there may exist MemoryKind::Values that
    // do not dominate the exit. MemoryKind::Values will always dominate the
    // exit and MemoryKind::PHIs only if there is at most one PHI_WRITE in the
    // non-affine region.
    if (DT.dominates(BB, Stmt->getRegion()->getExit()))
      isKnownMustAccess = true;
  }

  // Non-affine PHI writes do not "happen" at a particular instruction, but
  // after exiting the statement. Therefore they are guaranteed to execute and
  // overwrite the old value.
  if (Kind == MemoryKind::PHI || Kind == MemoryKind::ExitPHI)
    isKnownMustAccess = true;

  if (!isKnownMustAccess && AccType == MemoryAccess::MUST_WRITE)
    AccType = MemoryAccess::MAY_WRITE;

  auto *Access =
      new MemoryAccess(Stmt, Inst, AccType, BaseAddress, ElementType, Affine,
                       Subscripts, Sizes, AccessValue, Kind, BaseName);

  scop->addAccessFunction(Access);
  Stmt->addAccess(Access);
  return Access;
}

void ScopBuilder::addArrayAccess(
    MemAccInst MemAccInst, MemoryAccess::AccessType AccType, Value *BaseAddress,
    Type *ElementType, bool IsAffine, ArrayRef<const SCEV *> Subscripts,
    ArrayRef<const SCEV *> Sizes, Value *AccessValue) {
  ArrayBasePointers.insert(BaseAddress);
  addMemoryAccess(MemAccInst->getParent(), MemAccInst, AccType, BaseAddress,
                  ElementType, IsAffine, AccessValue, Subscripts, Sizes,
                  MemoryKind::Array);
}

void ScopBuilder::ensureValueWrite(Instruction *Inst) {
  ScopStmt *Stmt = scop->getStmtFor(Inst);

  // Inst not defined within this SCoP.
  if (!Stmt)
    return;

  // Do not process further if the instruction is already written.
  if (Stmt->lookupValueWriteOf(Inst))
    return;

  addMemoryAccess(Inst->getParent(), Inst, MemoryAccess::MUST_WRITE, Inst,
                  Inst->getType(), true, Inst, ArrayRef<const SCEV *>(),
                  ArrayRef<const SCEV *>(), MemoryKind::Value);
}

void ScopBuilder::ensureValueRead(Instruction *Inst, Value *V,
                                  BasicBlock *UserBB) {

  // There cannot be an "access" for literal constants. BasicBlock references
  // (jump destinations) also never change.
  if ((isa<Constant>(V) && !isa<GlobalVariable>(V)) || isa<BasicBlock>(V))
    return;

  // If the instruction can be synthesized and the user is in the region we do
  // not need to add a value dependences.
  auto *Scope = LI.getLoopFor(UserBB);
  if (canSynthesize(V, *scop, &SE, Scope))
    return;

  // TODO HACK
  if (V->getType()->isIntegerTy(1))
    return;

  // Do not build scalar dependences for required invariant loads as we will
  // hoist them later on anyway or drop the SCoP if we cannot.
  auto &ScopRIL = scop->getRequiredInvariantLoads();
  if (ScopRIL.count(dyn_cast<LoadInst>(V)))
    return;

  // Determine the ScopStmt containing the value's definition and use. There is
  // no defining ScopStmt if the value is a function argument, a global value,
  // or defined outside the SCoP.
  Instruction *ValueInst = dyn_cast<Instruction>(V);
  ScopStmt *ValueStmt = ValueInst ? scop->getStmtFor(ValueInst) : nullptr;

  ScopStmt *UserStmt = scop->getStmtFor(UserBB);

  // We do not model uses outside the scop.
  if (!UserStmt)
    return;

  // Add MemoryAccess for invariant values only if requested.
  if (!ModelReadOnlyScalars && !ValueStmt)
    return;

  // Ignore use-def chains within the same ScopStmt.
  if (ValueStmt == UserStmt)
    return;

  // Do not create another MemoryAccess for reloading the value if one already
  // exists.
  if (UserStmt->lookupValueReadOf(V))
    return;

  addMemoryAccess(UserBB, Inst, MemoryAccess::READ, V, V->getType(), true, V,
                  ArrayRef<const SCEV *>(), ArrayRef<const SCEV *>(),
                  MemoryKind::Value);
  if (ValueInst)
    ensureValueWrite(ValueInst);
}

void ScopBuilder::ensurePHIWrite(PHINode *PHI, BasicBlock *IncomingBlock,
                                 Value *IncomingValue, bool IsExitBlock) {
  // As the incoming block might turn out to be an error statement ensure we
  // will create an exit PHI SAI object. It is needed during code generation
  // and would be created later anyway.
  if (IsExitBlock)
    scop->getOrCreateScopArrayInfo(PHI, PHI->getType(), {},
                                   MemoryKind::ExitPHI);

  ScopStmt *IncomingStmt = scop->getStmtFor(IncomingBlock);
  if (!IncomingStmt)
    return;

  // Take care for the incoming value being available in the incoming block.
  // This must be done before the check for multiple PHI writes because multiple
  // exiting edges from subregion each can be the effective written value of the
  // subregion. As such, all of them must be made available in the subregion
  // statement.
  ensureValueRead(PHI, IncomingValue, IncomingBlock);

  // Do not add more than one MemoryAccess per PHINode and ScopStmt.
  if (MemoryAccess *Acc = IncomingStmt->lookupPHIWriteOf(PHI)) {
    assert(Acc->getAccessInstruction() == PHI);
    Acc->addIncoming(IncomingBlock, IncomingValue);
    return;
  }

  MemoryAccess *Acc = addMemoryAccess(
      IncomingStmt->getEntryBlock(), PHI, MemoryAccess::MUST_WRITE, PHI,
      PHI->getType(), true, IncomingValue, ArrayRef<const SCEV *>(),
      ArrayRef<const SCEV *>(),
      IsExitBlock ? MemoryKind::ExitPHI : MemoryKind::PHI);
  assert(Acc);
  Acc->addIncoming(IncomingBlock, IncomingValue);
}

void ScopBuilder::addPHIReadAccess(PHINode *PHI) {
  addMemoryAccess(PHI->getParent(), PHI, MemoryAccess::READ, PHI,
                  PHI->getType(), true, PHI, ArrayRef<const SCEV *>(),
                  ArrayRef<const SCEV *>(), MemoryKind::PHI);
}

void ScopBuilder::buildScop(Region &R, AssumptionCache &AC) {
  if (OneWrite || PollyExpProp) {
    DEBUG(errs() << "Before one write stmt creation:\n";
          R.getEntry()->getParent()->dump(); R.dump());

    ScopDetection::RegionSet &NASSet =
        SD.getDetectionContext(&R)->NonAffineSubRegionSet;
    DEBUG({
      errs() << "Non affine regions:\n";
      for (auto *R : NASSet)
        R->dump();
    });

    RegionInfo *RI = R.getRegionInfo();
    SmallVector<PHINode *, 4> PHIsToDemote;
    for (auto *BB : R.blocks()) {
      auto *BBR = RI->getRegionFor(BB);
      if (NASSet.count(BBR)) {
        continue;
      }

      auto *Scope = LI.getLoopFor(BB);
      for (auto &I : *BB) {
        if (!isa<PHINode>(I))
          break;
        if (canSynthesize(&I, R, &SE, Scope)) {
          DEBUG(errs() << "Can synthesize PHI: " << I << "\n");
          continue;
        }
        auto *PHI = cast<PHINode>(&I);
        DEBUG(errs() << "Will demote PHI: " << I << "\n");
        PHIsToDemote.push_back(PHI);
      }
    }
    for (auto &I : *R.getExit()) {
      if (!isa<PHINode>(I))
        break;
      auto *PHI = cast<PHINode>(&I);
      DEBUG(errs() << "Will demote PHI: " << I << "\n");
      PHIsToDemote.push_back(PHI);
    }
    for (auto *PHI : PHIsToDemote) {
      if (PHI->getNumIncomingValues() == 1) {
        PHI->replaceAllUsesWith(PHI->getIncomingValue(0));
        PHI->eraseFromParent();
      } else {
        DEBUG(errs() << "Demote PHI: " << *PHI << "\n");
        DemotePHIToStack(PHI);
      }
    }
    DEBUG(errs() << "After PHI demotation:\n";
          R.getEntry()->getParent()->dump());

    SmallVector<Instruction *, 4> InstToDemote;
    for (auto *BB : R.blocks()) {
      auto *BBR = RI->getRegionFor(BB);
      if (NASSet.count(BBR))
        continue;

      auto *Scope = LI.getLoopFor(BB);
      SmallVector<Instruction *, 32> Insts;
      for (auto &I : *BB) {
        Insts.push_back(&I);
      }
      bool First = true;
      for (auto It = Insts.rbegin() + 1, End = Insts.rend(); It != End; It++) {
        Instruction *I = *It;
        if (I->mayHaveSideEffects()) {
          if (First) {
            First = false;
          } else {
            splitBlock(I->getParent(), *(It - 1), &DT, &LI, RI);
            continue;
          }
        }

        if (canSynthesize(I, R, &SE, Scope))
          continue;

        // TODO HACK
        if (I->getType()->isIntegerTy(1))
          continue;

        for (auto *User : I->users()) {
          auto *UserI = dyn_cast<Instruction>(User);
          if (!UserI || UserI->getParent() == BB)
            continue;
          if (First) {
            First = false;
          } else {
            splitBlock(I->getParent(), *(It - 1), &DT, &LI, RI);
          }
          DEBUG(errs() << "Demote I: " << *I << " due to " << *UserI << "\n");
          AllocaInst *AI = DemoteRegToStack(*I);
          for (auto *User : AI->users()) {
            auto *UserI = cast<Instruction>(User);
            if (isa<StoreInst>(UserI))
              continue;
            assert(isa<LoadInst>(User));
            if (UserI->getParent() != I->getParent())
              continue;
            assert(UserI->getNumUses() == 1);
            auto *UserIUser = cast<Instruction>(UserI->user_back());
            if (UserIUser->getParent() == I->getParent()) {
              UserIUser->replaceUsesOfWith(UserI, I);
              UserI->eraseFromParent();
              continue;
            }

            assert(isa<PHINode>(UserIUser));
            auto *PHI = cast<PHINode>(UserIUser);
            if (canSynthesize(I, R, &SE, Scope))
              continue;

            if (PHI->getNumIncomingValues() == 1) {
              PHI->replaceAllUsesWith(I);
              PHI->eraseFromParent();
            } else {
              assert(PHI->getNumIncomingValues() == 1);
              UserI->moveBefore(&*PHI->getParent()->getFirstInsertionPt());
              PHI->replaceAllUsesWith(UserI);
              PHI->eraseFromParent();
            }
          }
          break;
        }
      }
    }
    for (BasicBlock &BB : *R.getEntry()->getParent())
      for (Instruction &I : BB)
        if (!I.hasName() && !I.getType()->isVoidTy())
          I.setName("n");

    for (auto *BB : R.blocks()) {
      auto *L = LI.getLoopFor(BB);
      StoreInst *SI = nullptr;
      for (auto &I : *BB)
        if (auto *S = dyn_cast<StoreInst>(&I)) {
          if (SI) {
            SI = nullptr;
            break;
          } else {
            SI = S;
          }
        }
      if (SI && L) {
        //createExpressionTree(*BB, L);
        //sortBB(SI, L);
      }
    }
    DEBUG(errs() << "After one write stmt creation:\n";
          R.getEntry()->getModule()->dump());
    assert(!verifyFunction(*R.getEntry()->getParent(), &errs()));
  }

  scop.reset(new Scop(R, SE, LI, *SD.getDetectionContext(&R)));
  scop->buildInvariantEquivalenceClasses();

  buildStmts(R);
  buildAccessFunctions(R);

  // In case the region does not have an exiting block we will later (during
  // code generation) split the exit block. This will move potential PHI nodes
  // from the current exit block into the new region exiting block. Hence, PHI
  // nodes that are at this point not part of the region will be.
  // To handle these PHI nodes later we will now model their operands as scalar
  // accesses. Note that we do not model anything in the exit block if we have
  // an exiting block in the region, as there will not be any splitting later.
  if (!scop->hasSingleExitEdge())
    buildAccessFunctions(*R.getExit(), nullptr,
                         /* IsExitBlock */ true);

  // Create memory accesses for global reads since all arrays are now known.
  auto *AF = SE.getConstant(IntegerType::getInt64Ty(SE.getContext()), 0);
  for (auto *GlobalRead : GlobalReads)
    for (auto *BP : ArrayBasePointers)
      addArrayAccess(MemAccInst(GlobalRead), MemoryAccess::READ, BP,
                     BP->getType(), false, {AF}, {nullptr}, GlobalRead);

  scop->init(AA, AC, DT, LI);
}

ScopBuilder::ScopBuilder(Region *R, AssumptionCache &AC, AliasAnalysis &AA,
                         const DataLayout &DL, DominatorTree &DT, LoopInfo &LI,
                         ScopDetection &SD, ScalarEvolution &SE)
    : AA(AA), DL(DL), DT(DT), LI(LI), SD(SD), SE(SE) {

  Function *F = R->getEntry()->getParent();

  DebugLoc Beg, End;
  getDebugLocations(getBBPairForRegion(R), Beg, End);
  std::string Msg = "SCoP begins here.";
  emitOptimizationRemarkAnalysis(F->getContext(), DEBUG_TYPE, *F, Beg, Msg);

  buildScop(*R, AC);

  DEBUG(scop->print(dbgs()));

  if (!scop->hasFeasibleRuntimeContext()) {
    InfeasibleScops++;
    Msg = "SCoP ends here but was dismissed.";
    scop.reset();
  } else {
    Msg = "SCoP ends here.";
    ++ScopFound;
    if (scop->getMaxLoopDepth() > 0)
      ++RichScopFound;
  }

  emitOptimizationRemarkAnalysis(F->getContext(), DEBUG_TYPE, *F, End, Msg);
}
