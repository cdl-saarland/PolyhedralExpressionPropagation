//===--------- ExpressionPropagation.cpp ----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "polly/ExpressionPropagation.h"

#include "polly/Options.h"
#include "polly/Support/GICHelper.h"
#include "polly/Support/LiveOutAnalyser.h"
#include "polly/Support/SCEVValidator.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"

#include "isl/map.h"
#include "isl/schedule.h"
#include "isl/set.h"

#define DEBUG_TYPE "polly-exp"

static cl::opt<unsigned>
    NumStreamTreshold("polly-prop-streams",
                      cl::desc("The maximal number of streams"), cl::Hidden,
                      cl::ZeroOrMore, cl::init(32), cl::cat(PollyCategory));

static cl::opt<unsigned> VectorWidth("polly-prop-vector-width",
                                     cl::desc("The vector width"), cl::Hidden,
                                     cl::ZeroOrMore, cl::init(8),
                                     cl::cat(PollyCategory));

static cl::opt<bool>
    ScheduleAll("polly-prop-schedule-all",
                      cl::desc(""), cl::Hidden,
                      cl::ZeroOrMore, cl::init(true), cl::cat(PollyCategory));

static cl::opt<bool>
    AllButProp("polly-prop-all-but-prop",
                      cl::desc(""), cl::Hidden,
                      cl::ZeroOrMore, cl::init(false), cl::cat(PollyCategory));

static cl::opt<int>
    MAX_PROP("polly-max-prop",
             cl::desc("The maximal number of propagated expressions."),
             cl::Hidden, cl::ZeroOrMore, cl::init(-1), cl::cat(PollyCategory));

static cl::opt<int>
    MAX_RESTRICT("polly-max-restrict",
                 cl::desc("The maximal number of propagated expressions."),
                 cl::Hidden, cl::ZeroOrMore, cl::init(-1),
                 cl::cat(PollyCategory));
static cl::opt<int>
    MIN_PROP("polly-min-prop",
             cl::desc("The maximal number of propagated expressions."),
             cl::Hidden, cl::ZeroOrMore, cl::init(-1), cl::cat(PollyCategory));

static cl::opt<int>
    NUM_PROP("polly-prop-times",
             cl::desc("The maximal number of propagated expressions."),
             cl::Hidden, cl::ZeroOrMore, cl::init(8), cl::cat(PollyCategory));

STATISTIC(REMOVED_SCALAR_READS, "Number of scalar reads removed.");
STATISTIC(REMOVED_MEMORY_READS, "Number of memory reads removed.");
STATISTIC(RESTRICTED_STATEMENTS, "Number of statement domains restricted.");
// STATISTIC(REMOVED_SCALAR_WRITES, "Number of scalar writes removed.");
STATISTIC(STATEMENT_SPLITS, "Number of statement splits.");
STATISTIC(STATEMENTS_SPLIT, "Number of statements split.");
STATISTIC(STATEMENTS_REMOVED, "Number of statement removed.");
STATISTIC(EDGES_PROPAGATED, "Number of RAW edges propagated.");

using namespace llvm;
using namespace polly;

static bool isSmallerInDim(isl_set *S, unsigned Dim, int V) {
  isl_pw_aff *Min = isl_set_dim_min(isl_set_copy(S), Dim);
  isl_pw_aff *Max = isl_set_dim_max(S, Dim);
  isl_pw_aff *Diff = isl_pw_aff_sub(Max, Min);
  isl_ctx *Ctx = isl_pw_aff_get_ctx(Diff);
  isl_local_space *LS = isl_local_space_from_space(isl_pw_aff_get_domain_space(Diff));
  isl_pw_aff *Limit = isl_pw_aff_from_aff(
      isl_aff_val_on_domain(LS, isl_val_int_from_si(Ctx, V - 1)));
  isl_set *UnrollGESet = isl_pw_aff_ge_set(Diff, Limit);
  bool Smaller = isl_set_is_empty(UnrollGESet);
  isl_set_free(UnrollGESet);
  return Smaller;
}

struct RegisterInfo {

  struct ResultTy {
    unsigned VectorUsage;
    unsigned ScalarUsage;

    ResultTy max1() {
      return {VectorUsage ? std::max((unsigned)1, VectorUsage) : 0,
              VectorUsage ? ScalarUsage : std::max((unsigned)1, ScalarUsage)};
    }

    ResultTy &max(ResultTy &Other) {
      VectorUsage = std::max(VectorUsage, Other.VectorUsage);
      ScalarUsage = std::max(ScalarUsage, Other.ScalarUsage);
      return *this;
    }

    ResultTy &min(ResultTy &Other) {
      VectorUsage = std::min(VectorUsage, Other.VectorUsage);
      ScalarUsage = std::min(ScalarUsage, Other.ScalarUsage);
      return *this;
    }

    bool operator==(const ResultTy &Other) const {
      return Other.VectorUsage == VectorUsage &&
             Other.ScalarUsage == ScalarUsage;
    }
    ResultTy operator+(unsigned U) const {
      return {VectorUsage ? VectorUsage + U : VectorUsage,
              VectorUsage ? ScalarUsage : ScalarUsage + U};
    }
  };

  using OpInfoTy = std::pair<Value *, ResultTy>;
  using OpInfoVecTy = SmallVector<OpInfoTy, 8>;

  ScopStmt &Stmt;
  Loop *L;

  LoopInfo &LI;
  const DominatorTree &DT;
  TargetTransformInfo *TTI;

  RecomputeInfo *RI = nullptr;

  DenseMap<Value *, ResultTy> RegCountMap;
  SmallPtrSet<Value *, 8> OutsideValues;

  RegisterInfo(ScopStmt &Stmt, Loop *L, LoopInfo &LI, const DominatorTree &DT,
               TargetTransformInfo *TTI)
      : Stmt(Stmt), L(L), LI(LI), DT(DT), TTI(TTI) {}

  static MemoryAccess *getLocalMA(ScopStmt &Stmt, RecomputeInfo &RI,
                                  MemoryAccess *MA) {
    if (auto *LocalMA = RI.DefiningMA->getStatement()->MemAccMap.lookup(MA)) {
      assert(!RI.NewAccs.count(MA));
      assert(LocalMA != MA);
      return getLocalMA(Stmt, RI, LocalMA);
    }
    if (auto *LocalMA = Stmt.MemAccMap.lookup(MA)) {
      assert(!RI.NewAccs.count(MA));
      assert(LocalMA != MA);
      return getLocalMA(Stmt, RI, LocalMA);
    }
    if (auto *LocalMA = RI.NewAccs.lookup(MA)) {
      assert(!Stmt.MemAccMap.count(MA));
      assert(LocalMA != MA);
      return getLocalMA(Stmt, RI, LocalMA);
    }
    if (RI.ParentRI)
      return getLocalMA(Stmt, *RI.ParentRI, MA);
    return MA;
  }

  MemoryAccess *getLocalMemAcc(Instruction *Inst, ScopStmt &DefStmt) {
    // errs() << "Inst: " << *Inst << " DefStmt: " << DefStmt.getBaseName()
    // <<"\n";
    if (!DefStmt.contains(Inst->getParent()))
      return nullptr;
    auto *InstMA = DefStmt.getArrayAccessOrNULLFor(Inst);
    if (!InstMA)
      return nullptr;

    // errs() << "Inst is mapped to MA: [" << InstMA << "]\n";
    // InstMA->dump();
    if (RI) {
      InstMA = getLocalMA(Stmt, *RI, InstMA);
      // errs() << "LocalMA: [" << InstMA << "]\n";
      // InstMA->dump();
    }
    return InstMA;
  }

  static ResultTy computeNumOfRegsNeeded(ScopStmt &Stmt, Value *V, Loop *L,
                                         LoopInfo &LI, const DominatorTree &DT,
                                         TargetTransformInfo *TTI) {
    if (!L)
      return {0, 0};

    RegisterInfo RegInf(Stmt, L, LI, DT, TTI);
    auto RegCost = RegInf.computeNumOfRegsNeeded(V);
    unsigned IVs = Stmt.getNumIterators();
    DEBUG({
      for (auto *V : RegInf.OutsideValues)
        errs() << "     OV: " << *V << " \n";
      errs() << *V << " in " << L->getName() << " RC: " << RegCost.VectorUsage
             << "/" << RegCost.ScalarUsage << " + " << IVs << " (IVs) + "
             << RegInf.OutsideValues.size()
             << " (OV) = " << RegCost.VectorUsage + 1 << "/"
             << RegCost.ScalarUsage + IVs - 1 + RegInf.OutsideValues.size()
             << "\n";
    });

    RegCost.VectorUsage += 1;
    RegCost.ScalarUsage += IVs - 1 + RegInf.OutsideValues.size();

    return RegCost;
  }

  ResultTy computeNumOfRegsNeeded(Value *V) {
    if (RegCountMap.count(V))
      return RegCountMap[V];

    ResultTy RegCost;
    if (auto *CI = dyn_cast<ConstantInt>(V)) {
      if (TTI && TTI->isLegalAddImmediate(CI->getSExtValue()) &&
          TTI->isLegalICmpImmediate(CI->getSExtValue())) {
        DEBUG_WITH_TYPE("polly-exp-reg", errs() << "RC 0\tCnst: " << *V
                                                << " is legal immediate.\n");
        RegCost = {0, 0};
      } else {
        DEBUG_WITH_TYPE("polly-exp-reg", errs()
                                             << "RC 1\tCnst: " << *V
                                             << " is not a legal immediate.\n");
        RegCost = {0, 1};
      }
    } else if (isa<ConstantFP>(V)) {
      DEBUG_WITH_TYPE("polly-exp-reg", errs() << "RC 1\tCnst: " << *V
                                              << " is floating point.\n");
      RegCost = {0, 1};
    } else if (auto *I = dyn_cast<Instruction>(V)) {
      RegCost = computeNumOfRegsNeeded(I);
    } else {
      // OutsideValues.insert(V);
      DEBUG_WITH_TYPE("polly-exp-reg",
                      errs() << "RC 0\tGVal: " << *V
                             << " is not a constant nor an instruction.\n");
      RegCost = {0, 1};
    }

    RegCountMap[V] = RegCost;
    return RegCost;
  }

  ResultTy computeNumOfRegsNeeded(Instruction *I) {
    if (!L)
      return {0, 1};

    if (isa<PHINode>(I)) {
      // OutsideValues.insert(I);
      DEBUG_WITH_TYPE("polly-exp-reg",
                      errs() << "RC 0\t PHI << " << *I
                             << " is a induction variable [special].\n");
      if (I->getParent() == L->getHeader())
        return {1, 0};
      return {0, 1};
    }

    if (!L->contains(I)) {
      auto *NewI = I;
      do {
        I = NewI;
        if (isa<PHINode>(I))
          break;
        SmallPtrSet<Value *, 4> OpNonConstants;
        for (auto &Op : I->operands())
          if (!isa<Constant>(Op))
            OpNonConstants.insert(Op);
        if (OpNonConstants.size() > 1)
          break;
        if (OpNonConstants.empty())
          return {0, 1};
        NewI = dyn_cast<Instruction>(*OpNonConstants.begin());
      } while (NewI);
      OutsideValues.insert(I);
      DEBUG_WITH_TYPE("polly-exp-reg", errs() << "RC 0\tInst: " << *I
                                              << " is outside the loop.\n");
      return {0, 1};
    }

    auto &DefStmt = RI ? *RI->DefiningMA->getStatement() : Stmt;
    auto *LocalInstMA = getLocalMemAcc(I, DefStmt);
    if (LocalInstMA) {
      DEBUG_WITH_TYPE("polly-exp-reg", errs() << "I: " << *I << " [DefStmt: "
                                              << DefStmt.getBaseName() << "]\n";
                      errs()
                      << "LocalInstMA: " << LocalInstMA->getAccessRelationStr()
                      << " [" << LocalInstMA << "]\n";);
      Scop &S = *Stmt.getParent();
      if (auto *ChainedRI = S.RecomputeMAs.lookup(LocalInstMA)) {
        DEBUG_WITH_TYPE("polly-exp-reg",
                        errs() << "MA is chained to " << ChainedRI << " : "
                               << ChainedRI->Dependence << "\n");
        assert(!ChainedRI->ParentRI);
        ChainedRI->ParentRI = RI;
        RI = ChainedRI;
        auto *OldL = L;
        auto *SrcStore =
            dyn_cast<StoreInst>(ChainedRI->DefiningMA->getAccessInstruction());
        DEBUG_WITH_TYPE("polly-exp-reg",
                        errs() << "ChainedVal: " << *SrcStore->getValueOperand()
                               << "\n");
        L = LI.getLoopFor(SrcStore->getParent());
        auto RegCost = computeNumOfRegsNeeded(SrcStore->getValueOperand());
        L = OldL;
        RI = ChainedRI->ParentRI;
        ChainedRI->ParentRI = nullptr;
        DEBUG_WITH_TYPE("polly-exp-reg", errs() << "RC " << RegCost.VectorUsage
                                                << "/" << RegCost.ScalarUsage
                                                << "\tInst: " << *I
                                                << " [PROP]\n");
        return RegCost;
      } else if (!LocalInstMA->isAffine()) {
        DEBUG_WITH_TYPE("polly-exp-reg", errs() << "non affine access! assume scalarization!\n");
        return {1, 1};
      } else if (LocalInstMA->isStrideOne(LocalInstMA->getStatement()->getSchedule())) {
        DEBUG_WITH_TYPE("polly-exp-reg", errs() << "stride one access!\n");
        return {1, 0};
      } else if (LocalInstMA->isStrideZero(LocalInstMA->getStatement()->getSchedule())) {
        DEBUG_WITH_TYPE("polly-exp-reg", errs() << "stride zero access!\n");
        return {0, 1};
      } else {
        DEBUG_WITH_TYPE("polly-exp-reg", errs() << "unknown stride access!\n");
        return {2, 1};
      }
    }

    if (I->getNumOperands() == 1) {
      auto *Op = I->getOperand(0);
      auto OpCost = computeNumOfRegsNeeded(Op);
      auto RegCost = OpCost.max1();
      DEBUG_WITH_TYPE("polly-exp-reg",
                      errs() << "RC " << RegCost.VectorUsage << "/"
                             << RegCost.ScalarUsage << "\tInst: " << *I
                             << " is unary. OpCost: " << OpCost.VectorUsage
                             << "/" << OpCost.ScalarUsage << ".\t\n");
      return RegCost;
    }

    // DEBUG_WITH_TYPE("polly-exp-reg",errs() << "\t\tInst: "<< *I << " has
    // multiple ops.\n");

    SmallPtrSet<Value *, 4> Ops;
    OpInfoVecTy OpInfos;
    OpInfos.reserve(I->getNumOperands());

    for (auto &Op : I->operands()) {
      Ops.insert(Op);
      auto OpCost = computeNumOfRegsNeeded(Op);
      OpInfos.emplace_back(Op, OpCost);
    }

    ResultTy MaxOpCost = {0, 0}, MinOpCost = {UINT_MAX, UINT_MAX};
    for (auto &OpInfoPair : OpInfos) {
      MaxOpCost.max(OpInfoPair.second);
      MinOpCost.min(OpInfoPair.second);
    }

    ResultTy RegCost;
    if (Ops.size() == 1)
      RegCost = MaxOpCost;
    else if (MaxOpCost == MinOpCost)
      RegCost = MaxOpCost + 1;
    else
      RegCost = MaxOpCost.max1();

    DEBUG_WITH_TYPE("polly-exp-reg", errs() << "RC " << RegCost.VectorUsage
                                            << "/" << RegCost.ScalarUsage
                                            << "\tInst: " << *I
                                            << ". Operands need min/maximal ("
                                            << MinOpCost.VectorUsage << "/"
                                            << MinOpCost.ScalarUsage << ") | ("
                                            << MaxOpCost.VectorUsage << "/"
                                            << MaxOpCost.ScalarUsage << ").\n");
    return RegCost;
  }
};

void polly::sortBB(ScopStmt &Stmt, StoreInst *SI, Loop *L, LoopInfo &LI,
                   const DominatorTree &DT) {
}

unsigned ExpressionPropagation::computeNumAccessedStreams(
    DefNode *Trg, SmallVectorImpl<RAWEdge *> &Edges, unsigned Limit) {
  SmallVector<DefNode *, 8> Nodes;
  SmallVector<isl_map *, 8> NodeDeps;
  for (auto *Node : Trg->Group->Nodes)
    if (!Node->isEmpty())
      Nodes.push_back(Node);
  NodeDeps.resize(Nodes.size());

  SmallPtrSet<MemoryAccess *, 8> TrgMAs;
  for (auto *Edge : Edges) {
    TrgMAs.insert(Edge->Trg->MA);
    Nodes.push_back(&Edge->Src->getDefNode());
    isl_map *Dep = Edge->getDep();
    Dep = isl_map_gist_domain(Dep, Edge->Src->getDomain());
    Dep = isl_map_gist_range(Dep, Edge->Trg->getDomain());
    Dep = isl_map_gist_params(Dep, getAssumedContext());
    NodeDeps.push_back(Dep);
  }

  return computeNumAccessedStreams(Nodes, NodeDeps, TrgMAs, Limit);
}

static isl_map *removeFixedInputDims(isl_map *M) {
  unsigned NumDims = isl_map_dim(M, isl_dim_in);
  isl_set *S = isl_map_domain(isl_map_copy(M));
  for (unsigned d = 0; d < NumDims; d++) {
    auto *Val = isl_set_plain_get_val_if_fixed(S, isl_dim_set, d);
    if (Val && !isl_val_is_nan(Val))
      M = isl_map_eliminate(M, isl_dim_in, d, 1);
    isl_val_free(Val);
  }

  isl_set_free(S);
  return M;
}

isl_stat countStreams(isl_basic_map *BM, void *User) {
  isl_basic_map_dump(BM);
  auto *BMBM = isl_basic_map_apply_domain(BM, isl_basic_map_copy(BM));
  isl_basic_map_dump(BMBM);
  auto *Diff = isl_basic_set_lexmax(isl_basic_map_deltas(BMBM));
  errs() << "Diff: " << Diff << "\n";
  unsigned Streams = 1;
  for (unsigned d = 0, e = isl_set_dim(Diff, isl_dim_set); d < e; d++) {
    isl_val *V = isl_set_plain_get_val_if_fixed(Diff, isl_dim_out, d);
    //errs() << "V: "; isl_val_dump(V);
    assert(V && !isl_val_is_nan(V));
    auto N = isl_val_get_num_si(V);
    auto D = isl_val_get_den_si(V);
    //errs() << "N: " << N << " D: "<< D << "\n";
    assert(N >= 0 && D == 1);
    Streams *= (N + 1);
    isl_val_free(V);
  }
  isl_set_free(Diff);

  errs() << "Streams: " << Streams << "\n";
  unsigned &NumStreams = *static_cast<unsigned *>(User);
  NumStreams += Streams;

  return isl_stat_ok;
}

static isl_map *getEqualAndLarger(__isl_take isl_space *setDomain) {
  isl_space *Space = isl_space_map_from_set(setDomain);
  isl_map *Map = isl_map_universe(Space);
  unsigned lastDimension = isl_map_dim(Map, isl_dim_in) - 1;

  // Set all but the last dimension to be equal for the input and output
  //
  //   input[i0, i1, ..., iX] -> output[o0, o1, ..., oX]
  //     : i0 = o0, i1 = o1, ..., i(X-1) = o(X-1)
  for (unsigned i = 0; i < lastDimension; ++i)
    Map = isl_map_equate(Map, isl_dim_in, i, isl_dim_out, i);

  // Set the last dimension of the input to be strict smaller than the
  // last dimension of the output.
  //
  //   input[?,?,?,...,iX] -> output[?,?,?,...,oX] : iX < oX
  Map = isl_map_order_lt(Map, isl_dim_in, lastDimension, isl_dim_out,
                         lastDimension);
  return Map;
}

__isl_give isl_set *getStride(isl_map *AccRel) {
  isl_map *S = isl_map_identity(
      isl_space_map_from_set(isl_space_domain(isl_map_get_space(AccRel))));
  isl_space *Space = isl_space_range(isl_map_get_space(S));
  isl_map *NextScatt = getEqualAndLarger(Space);

  S = isl_map_reverse(S);

  //errs() << " S: " << S << "\nNS: " << NextScatt << "\nAR: " << AccRel << "\n";
  NextScatt = isl_map_apply_range(NextScatt, isl_map_copy(S));
  //errs() << "NS: " << NextScatt << "\n";
  NextScatt = isl_map_apply_range(NextScatt, isl_map_copy(AccRel));
  //errs() << "NS: " << NextScatt << "\n";
  NextScatt = isl_map_apply_domain(NextScatt, S);
  //errs() << "NS: " << NextScatt << "\n";
  NextScatt = isl_map_apply_domain(NextScatt, isl_map_copy(AccRel));
  //errs() << "NS: " << NextScatt << "\n";

  isl_set *Deltas = isl_map_deltas(NextScatt);
  //errs() << " D: " << Deltas << "\n";
  Deltas = isl_set_lexmin(Deltas);
  //errs() << " D: " << Deltas << "\n";
  return Deltas;
}
bool isGreaterStride(isl_map *AccRel, unsigned StrideWidth) {
  auto *Stride = getStride(AccRel);
  auto *StrideX = isl_set_universe(isl_set_get_space(Stride));
  for (unsigned i = 0; i < isl_set_dim(StrideX, isl_dim_set) - 1; i++)
    StrideX = isl_set_fix_si(StrideX, isl_dim_set, i, 0);
  StrideX = isl_set_lower_bound_si(
      StrideX, isl_dim_set, isl_set_dim(StrideX, isl_dim_set) - 1, StrideWidth);
  bool IsStrideX = isl_set_is_subset(Stride, StrideX);
  isl_set_free(Stride);
  isl_set_free(StrideX);
  return IsStrideX;
}
unsigned ExpressionPropagation::computeNumAccessedStreams(
    ArrayRef<DefNode *> Nodes, SmallVectorImpl<isl_map *> &NodeDeps,
    SmallPtrSetImpl<MemoryAccess *> &ExcludedMAs, unsigned Limit) {
  assert(NodeDeps.empty() || NodeDeps.size() == Nodes.size());

  std::map<const ScopArrayInfo *, isl_map *> AccessMap;
  unsigned NonAffineAccs = 0, NonAffineEdge = 0;

  int NumAccessStreams = 0;
  for (unsigned u = 0; u < Nodes.size() && NumAccessStreams + NonAffineAccs <= Limit; u++) {
    auto *Stmt = Nodes[u]->getStmt();
    if (Nodes[u]->isEmpty())
      continue;
    isl_map *Dep = nullptr;
    if (!NodeDeps.empty())
      Dep = NodeDeps[u];

    for (auto *MA : *Stmt) {
      if (ExcludedMAs.count(MA))
        continue;
      if (MA->isScalarKind())
        continue;
      if (Dep && MA->isWrite())
        continue;
      if (!MA->isAffine()) {
        NonAffineAccs++;
        if (NumAccessStreams + NonAffineAccs > Limit)
          break;
        if (Dep)
          NonAffineEdge++;
        continue;
      }

      auto *AccRel = MA->getAccessRelation();
      //(errs() << "AccRel: " << AccRel << "\n");
      if (Dep)
        AccRel = isl_map_apply_domain(AccRel, isl_map_copy(Dep));

      auto *SAI = MA->getScopArrayInfo();
      isl_val *V = isl_map_plain_get_val_if_fixed(AccRel, isl_dim_out,
                                          SAI->getNumberOfDimensions() - 1);
      bool Fixed = V && !isl_val_is_nan(V);
      isl_val_free(V);
      if (Fixed) {
        isl_map_free(AccRel);
        continue;
      }

      DEBUG(errs() << "AccRel: " << AccRel << "\n");

      //AccRel = isl_map_gist_domain(AccRel, Nodes[u]->getDomain());
      //AccRel = isl_map_project_out(AccRel, isl_dim_out,
                                   //SAI->getNumberOfDimensions() - 1, 1);

      DEBUG(errs() << "AccRel: " << AccRel << "\n");
      AccRel = isl_map_reset_tuple_id(AccRel, isl_dim_in);
      auto *&Accesses = AccessMap[SAI];
      bool New = !Accesses || !isl_map_is_subset(AccRel, Accesses);
      if (New) {
        //(errs() << "+ AccRel: " << AccRel << "\n");
        DEBUG(errs() << "  AccRel is not contained in Accesses!\n\n");
        NumAccessStreams++;
        if (NumAccessStreams + NonAffineAccs > Limit) {
          isl_map_free(AccRel);
          break;
        }
      } else {
        //(errs() << "= AccRel: " << AccRel << "\n");
        DEBUG(errs() << "  AccRel is contained in Accesses!\n\n");
        isl_map_free(AccRel);
        continue;
      }

      isl_map *NextCellMap = isl_map_universe(isl_map_get_space(AccRel));
      unsigned NumDims = isl_map_dim(NextCellMap, isl_dim_out);
      for (unsigned d = 0; d + 1 < NumDims; d++)
        NextCellMap = isl_map_fix_si(NextCellMap, isl_dim_out, d, 0);
      NextCellMap = isl_map_lower_bound_si(NextCellMap, isl_dim_out, NumDims -1, -4);
      NextCellMap = isl_map_upper_bound_si(NextCellMap, isl_dim_out, NumDims -1, 3);
      DEBUG(errs() << "NextCellMap: " << NextCellMap << "\n");
      AccRel = isl_map_sum(AccRel, NextCellMap);
      //(errs() << "AdjAccR: " << AccRel << "\n\n");

      //isl_map *LastItMap = isl_map_lex_le(isl_space_domain(isl_map_get_space(AccRel)));
      //for (unsigned d = 0; d + 1 < NumDims; d++)
        //LastItMap = isl_map_equate(LastItMap, isl_dim_in, d, isl_dim_out, d);
      //(errs() << "LastItMap: " << LastItMap << "\n\n");
      //AccRel = isl_map_apply_domain(AccRel, LastItMap);
      //(errs() << "AdjAccR: " << AccRel << "\n\n");

      AccRel = isl_map_drop_constraints_not_involving_dims(AccRel, isl_dim_out, 0, SAI->getNumberOfDimensions());
      DEBUG(errs() << "AdjAccR: " << AccRel << "\n\n");

      if (!Accesses)
        Accesses = AccRel;
      else
        Accesses = isl_map_coalesce(isl_map_union(Accesses, AccRel));
    }
  }
  assert(NumAccessStreams >= 0);

  for (auto *Dep : NodeDeps)
    isl_map_free(Dep);
  for (auto &It : AccessMap)
    isl_map_free(It.second);

  (errs() << "GOT " << NumAccessStreams + NonAffineAccs
          << " access streams [NA: " << NonAffineAccs
          << ", Edge: " << NonAffineEdge << "]!\n");
  return NumAccessStreams + NonAffineAccs;
}

std::pair<unsigned, unsigned>
ExpressionPropagation::computeNumOfRegsNeeded(SmallVectorImpl<RAWEdge *> &Edges) {
  auto &TrgDef = Edges[0]->Trg->getDefNode();
  auto &TrgStmt = *TrgDef.getStmt();
  auto *TrgStore = dyn_cast<StoreInst>(TrgDef.MA->getAccessInstruction());
  if (!TrgStore)
    return {-1, -1};

  SmallVector<RecomputeInfo *, 8> RIs;
  for (RAWEdge *RAW : Edges) {
    assert(&TrgStmt == RAW->Trg->getStmt());

    RAW->Src->applyToEdges<RAWEdge>([&](const RAWEdge &Edge) {
      if (&Edge.Trg->getDefNode() != &TrgDef)
        return;
      RecomputeInfo *RI =
          new RecomputeInfo(Edge.Trg->MA, Edge.Src->MA, nullptr);
      RIs.push_back(RI);
      TrgStmt.getParent()->RecomputeMAs[Edge.Trg->MA] = RI;
    });
  }

  auto *L = LI->getLoopFor(TrgStore->getParent());
  auto RegCost = RegisterInfo::computeNumOfRegsNeeded(TrgStmt, TrgStore, L, *LI,
                                                      *DT, TTI);
  for (auto *RI : RIs) {
    TrgStmt.getParent()->RecomputeMAs.erase(RI->RecomputeMA);
    delete RI;
  }

  return {RegCost.VectorUsage,RegCost.ScalarUsage};
}


void Node::printEdges(raw_ostream &OS, unsigned Indent) const {
  if (getEdges<RAWEdge>().size()) {
    OS.indent(Indent) << "\tRAWEdges:\n";
    applyToEdges<RAWEdge>([&](const RAWEdge &Edge) {
      OS.indent(Indent) << "\t\t" << Edge.getDepStr() << " ["
                        << Edge.RR.toString() << "]\n";
      assert(Edge);
    });
  }
  if (getEdges<WAREdge>().size()) {
    OS.indent(Indent) << "\tWAREdges:\n";
    applyToEdges<WAREdge>([&](const WAREdge &Edge) {
      OS.indent(Indent) << "\t\t" << Edge.getDepStr() << "\n";
      assert(Edge);
    });
  }
  if (getEdges<WAWEdge>().size()) {
    OS.indent(Indent) << "\tWAWEdges:\n";
    applyToEdges<WAWEdge>([&](const WAWEdge &Edge) {
      OS.indent(Indent) << "\t\t" << Edge.getDepStr() << "\n";
      assert(Edge);
    });
  }
}

template <typename EdgeTy>
static bool splitEdge(DefNode *OldDef, DefNode *NewDef,
                      DenseMap<MemoryAccess *, MemoryAccess *> &AccMap,
                      EdgeTy &Edge, __isl_keep isl_set *SplitDomain,
                      isl_set *LeftDomain, __isl_keep isl_id *NewId) {
  assert(OldDef && NewDef && Edge && SplitDomain && LeftDomain && NewId);
  isl_map *EdgeDep = Edge.getDep();
  ScopStmt *OldStmt = OldDef->getStmt();

  if (&Edge.Src->getDefNode() == OldDef) {
    EdgeDep = isl_map_intersect_domain(EdgeDep, isl_set_copy(LeftDomain));
  }

  if (Edge.Src->MA->getStatement() == OldStmt && !Edge.isPropagated()) {
    isl_map *SplitDep =
        isl_map_intersect_domain(Edge.getDep(), isl_set_copy(SplitDomain));
    if (Edge.Trg->MA->getStatement() == OldStmt) {
      SplitDep = isl_map_intersect_range(SplitDep, isl_set_copy(LeftDomain));
    }

    if (isl_map_is_empty(SplitDep)) {
      isl_map_free(SplitDep);
    } else {
      SplitDep = isl_map_set_tuple_id(SplitDep, isl_dim_in, isl_id_copy(NewId));

      MemoryAccess *NewSrcMA = AccMap[Edge.Src->MA];
      assert(NewSrcMA);
      Node *NewSrc = OldDef->ExprProp.DefNodeMap.lookup(NewSrcMA);
      if (!NewSrc)
        NewSrc = OldDef->ExprProp.UseNodeMap.lookup(NewSrcMA);
      assert(NewSrc);

      auto *SplitEdge =
          new EdgeTy(static_cast<typename EdgeTy::SrcTy *>(NewSrc), Edge.Trg,
                     SplitDep, false);
      //SplitEdge->setRR(Edge);
      auto Key = SplitEdge->template getKey<EdgeTy>();
      auto &SrcEdges = NewSrc->getEdges<EdgeTy>();
      auto &TrgEdges = Edge.Trg->template getEdges<EdgeTy>();
      assert(!SrcEdges.count(Key) && !TrgEdges.count(Key));
      SrcEdges[Key] = SplitEdge;
      TrgEdges[Key] = SplitEdge;
    }
  }

  if (Edge.Trg->MA->getStatement() == OldStmt) {
    EdgeDep = isl_map_intersect_range(EdgeDep, isl_set_copy(LeftDomain));

    isl_map *SplitDep =
        isl_map_intersect_range(Edge.getDep(), isl_set_copy(SplitDomain));
    if (Edge.Src->MA->getStatement() == OldStmt) {
      SplitDep = isl_map_intersect_domain(SplitDep, isl_set_copy(LeftDomain));
    }

    if (isl_map_is_empty(SplitDep)) {
      isl_map_free(SplitDep);
    } else {
      SplitDep =
          isl_map_set_tuple_id(SplitDep, isl_dim_out, isl_id_copy(NewId));

      MemoryAccess *NewTrgMA = AccMap[Edge.Trg->MA];
      assert(NewTrgMA);
      Node *NewTrg = OldDef->ExprProp.DefNodeMap.lookup(NewTrgMA);
      if (!NewTrg)
        NewTrg = OldDef->ExprProp.UseNodeMap.lookup(NewTrgMA);
      assert(NewTrg);

      auto *SplitEdge =
          new EdgeTy(Edge.Src, static_cast<typename EdgeTy::TrgTy *>(NewTrg),
                     SplitDep, false);
      //SplitEdge->setRR(Edge);
      auto Key = SplitEdge->template getKey<EdgeTy>();
      auto &SrcEdges = Edge.Src->template getEdges<EdgeTy>();
      auto &TrgEdges = NewTrg->getEdges<EdgeTy>();
      assert(!SrcEdges.count(Key) && !TrgEdges.count(Key));
      SrcEdges[Key] = SplitEdge;
      TrgEdges[Key] = SplitEdge;
    }
  }

  if (&Edge.Src->getDefNode() == &Edge.Trg->getDefNode()) {
    isl_map *SplitDep =
        isl_map_intersect_range(Edge.getDep(), isl_set_copy(SplitDomain));
    SplitDep = isl_map_intersect_domain(SplitDep, isl_set_copy(SplitDomain));

    if (isl_map_is_empty(SplitDep)) {
      isl_map_free(SplitDep);
    } else {
      SplitDep = isl_map_set_tuple_id(SplitDep, isl_dim_in, isl_id_copy(NewId));
      SplitDep =
          isl_map_set_tuple_id(SplitDep, isl_dim_out, isl_id_copy(NewId));

      MemoryAccess *NewSrcMA = AccMap[Edge.Src->MA],
                   *NewTrgMA = AccMap[Edge.Trg->MA];
      assert(NewSrcMA && NewTrgMA);

      Node *NewSrc = OldDef->ExprProp.DefNodeMap.lookup(NewSrcMA);
      Node *NewTrg = OldDef->ExprProp.DefNodeMap.lookup(NewTrgMA);
      if (!NewSrc)
        NewSrc = OldDef->ExprProp.UseNodeMap.lookup(NewSrcMA);
      if (!NewTrg)
        NewTrg = OldDef->ExprProp.UseNodeMap.lookup(NewTrgMA);
      assert(NewSrc && NewTrg);

      auto *SplitEdge = new EdgeTy(
          static_cast<typename EdgeTy::SrcTy *>(NewSrc),
          static_cast<typename EdgeTy::TrgTy *>(NewTrg), SplitDep, false);
      auto Key = SplitEdge->template getKey<EdgeTy>();
      auto &SrcEdges = NewSrc->getEdges<EdgeTy>();
      auto &TrgEdges = NewTrg->template getEdges<EdgeTy>();
      assert(!SrcEdges.count(Key) && !TrgEdges.count(Key));
      SrcEdges[Key] = SplitEdge;
      TrgEdges[Key] = SplitEdge;
    }
  }

  if (!isl_map_is_empty(EdgeDep)) {
    Edge.setDep(EdgeDep);
    return true;
  } else {
    isl_map_free(EdgeDep);
    return false;
  }
}

DefNode::DefNode(MemoryAccess *DefMA, ExpressionPropagation &ExprProp,
                 isl_set *Domain)
    : Node(DefMA), IsOverwritten(false), ExprProp(ExprProp), Domain(Domain),
      Schedule(getStmt()->getSchedule()) {
  Blocks.insert(DefMA->getStatement()->getBasicBlock());
  Arrays[DefMA->getScopArrayInfo()] = 1;
}

DefNode::DefNode(DefNode &Other, MemoryAccess *DefMA,
                 DenseMap<MemoryAccess *, MemoryAccess *> &AccMap,
                 isl_set *SplitDomain, __isl_keep isl_set *LeftDomain)
    : Node(DefMA), Arrays(Other.Arrays), Blocks(Other.Blocks),
      IsOverwritten(Other.IsOverwritten), ExprProp(Other.ExprProp) {
  isl_id *NewId = getStmt()->getDomainId();
  Domain = isl_set_set_tuple_id(isl_set_copy(SplitDomain), isl_id_copy(NewId));
  Schedule =
      isl_map_set_tuple_id(Other.getSchedule(), isl_dim_in, isl_id_copy(NewId));

  Other.ExprProp.DefNodeMap[DefMA] = this;

  DenseMap<UseNode *, UseNode *> OpMap;
  for (auto *Op : Other.Operands) {
    MemoryAccess *UseMACopy = AccMap[Op->MA];
    assert(Op && UseMACopy);
    UseNode *OpUseCopy = ExprProp.copyOperandInto(*Op, *this, *UseMACopy);
    Operands.insert(OpUseCopy);
    OpMap[Op] = OpUseCopy;
  }

  Other.applyToAndCheckEdges<RAWEdge>(
      [&](RAWEdge &RAW) {
        return splitEdge<RAWEdge>(&Other, this, AccMap, RAW, SplitDomain,
                                  LeftDomain, NewId);
      },
      false);
  Other.applyToAndCheckEdges<WAREdge>(
      [&](WAREdge &WAR) {
        return splitEdge<WAREdge>(&Other, this, AccMap, WAR, SplitDomain,
                                  LeftDomain, NewId);
      },
      false);
  Other.applyToAndCheckEdges<WAWEdge>(
      [&](WAWEdge &WAW) {
        return splitEdge<WAWEdge>(&Other, this, AccMap, WAW, SplitDomain,
                                  LeftDomain, NewId);
      },
      false);

  // Split operand dependences.
  for (auto *Op : Other.Operands) {
    auto *OpUseCopy = OpMap[Op];
    assert(OpUseCopy);

    Op->applyToAndCheckEdges<RAWEdge>(
        [&](RAWEdge &RAW) {
          assert(RAW);

          if (RAW.Src == &Other)
            return true;

          isl_map *DepSplit =
              isl_map_intersect_range(RAW.getDep(), isl_set_copy(SplitDomain));

          if (!isl_map_is_empty(DepSplit)) {
            DepSplit =
                isl_map_set_tuple_id(DepSplit, isl_dim_out, isl_id_copy(NewId));

            auto *RAWSplit = new RAWEdge(RAW.Src, OpUseCopy, DepSplit, false);
            //RAWSplit->setRR(RAW);
            auto Key = RAWSplit->getKey<RAWEdge>();
            auto &SrcRAWEdges = RAW.Src->getEdges<RAWEdge>();
            auto &TrgRAWEdges = OpUseCopy->getEdges<RAWEdge>();
            assert(!SrcRAWEdges.count(Key) && !TrgRAWEdges.count(Key));
            SrcRAWEdges[Key] = RAWSplit;
            TrgRAWEdges[Key] = RAWSplit;
          } else {
            isl_map_free(DepSplit);
            return true;
          }

          isl_map *DepNew =
              isl_map_subtract_range(RAW.getDep(), isl_set_copy(SplitDomain));

          if (!isl_map_is_empty(DepNew)) {
            RAW.setDep(DepNew);
            return true;
          } else {
            isl_map_free(DepNew);
            return false;
          }
        },
        false);

    Op->applyToAndCheckEdges<WAREdge>(
        [&](WAREdge &WAR) {
          assert(WAR);

          if (WAR.Trg == &Other)
            return true;

          isl_map *DepSplit =
              isl_map_intersect_domain(WAR.getDep(), isl_set_copy(SplitDomain));
          if (!isl_map_is_empty(DepSplit)) {
            DepSplit =
                isl_map_set_tuple_id(DepSplit, isl_dim_in, isl_id_copy(NewId));

            auto *WARSplit = new WAREdge(OpUseCopy, WAR.Trg, DepSplit, false);
            auto Key = WARSplit->getKey<WAREdge>();
            auto &SrcWAREdges = OpUseCopy->getEdges<WAREdge>();
            auto &TrgWAREdges = WAR.Trg->getEdges<WAREdge>();
            assert(!SrcWAREdges.count(Key) && !TrgWAREdges.count(Key));
            SrcWAREdges[Key] = WARSplit;
            TrgWAREdges[Key] = WARSplit;
          } else {
            isl_map_free(DepSplit);
            return true;
          }

          isl_map *DepNew =
              isl_map_subtract_domain(WAR.getDep(), isl_set_copy(SplitDomain));

          if (!isl_map_is_empty(DepNew)) {
            WAR.setDep(DepNew);
            return true;
          } else {
            isl_map_free(DepNew);
            return false;
          }
        },
        false);
  }

  getStmt()->restrictDomain(isl_set_copy(Domain));
  isl_set_free(SplitDomain);
  isl_id_free(NewId);
}

bool DefNode::isLiveOut() const {
  assert(Group);
  return Group->IsLiveOut;
}

DefNode &UseNode::getDefNode() { return *Def; }
const DefNode &UseNode::getDefNode() const { return *Def; }

__isl_give isl_set *UseNode::getDomain() const {
  return getDefNode().getDomain();
}
__isl_give isl_map *UseNode::getSchedule() const {
  return getDefNode().getSchedule();
}
std::string UseNode::getDomainStr() const {
  return getDefNode().getDomainStr();
}

DefNode &DefNode::getDefNode() { return *this; }
const DefNode &DefNode::getDefNode() const { return *this; }

bool DefNode::setDomain(__isl_take isl_set *NewDomain) {
  DEBUG(errs() << "\nBefore set domain of:\n"
               << "   Domain: " << Domain << "\n"
               << "NewDomain: " << NewDomain << "\n");

  // assert(NewDomain && isl_set_is_subset(NewDomain, Domain));
  if (isl_set_is_subset(Domain, NewDomain)) {
    DEBUG(errs() << "Domain is subset!\n");
    isl_set_free(NewDomain);
    return false;
  }

  DEBUG(errs() << "NewDomain is subset!\n");

  if (UsedDomain) {
    assert(isl_set_is_subset(UsedDomain, Domain));
    UsedDomain = isl_set_intersect(UsedDomain, isl_set_copy(NewDomain));
  }

  if (Overwritten) {
    assert(isl_set_is_subset(Overwritten, Domain));
    Overwritten = isl_set_intersect(Overwritten, isl_set_copy(NewDomain));
  }

  isl_set_free(Domain);
  Domain = NewDomain;
  return true;
}

void DefNode::print(raw_ostream &OS, unsigned Indent) const {
  unsigned NumRegsNeeded = -1;
  OS.indent(Indent) << "\tMA: " << MA->getAccessRelationStr() << " [A# "
                    << MA->AccNo << "] [LO " << isLiveOut()
                    << "] [OW: " << IsOverwritten
                    << "] [#Reg: " << NumRegsNeeded << "]\n";
  OS.indent(Indent) << "\tDom: " << Domain << " Sched: " << Schedule << "\n";
  OS.indent(Indent) << "\tOperands (#" << Operands.size() << ")\n";
  for (const UseNode *Op : Operands) {
    assert(Op->Def == this);
    Op->print(OS, Indent + 8);
  }
  printEdges(OS, Indent);
  OS.indent(Indent) << "\n";
}

void DefNode::verify() const {
  //DEBUG(errs() << "Verify def:\n"; print(errs(), 4));

  isl_set *Domain = getDomain();
  assert(Domain);

  assert(!UsedDomain || isl_set_is_subset(UsedDomain, Domain));
  assert(!Overwritten || isl_set_is_subset(Overwritten, Domain));

  applyToEdges<RAWEdge>([&](const RAWEdge &Edge) {
    assert(Edge);
    isl_map *EdgeDep = Edge.getDep();
    assert(EdgeDep);
    isl_set *EdgeDom = isl_map_domain(EdgeDep);
    assert(isl_set_is_subset(EdgeDom, Domain));
    isl_set_free(EdgeDom);
    if (!(Edge.isPropagated() ||
          Edge.Trg->getDefNode().Operands.count(Edge.Trg))) {
      //errs() << "Ops:\n";
      //for (auto *Op : Edge.Trg->getDefNode().Operands)
        //Op->MA->dump();
      //errs() << "Trg:\n";
      //Edge.Trg->MA->dump();
    }
    assert(Edge.isPropagated() ||
           Edge.Trg->getDefNode().Operands.count(Edge.Trg));
    assert(Edge.Trg->getEdges<RAWEdge>()[Edge.getKey<RAWEdge>()]);
  });

  applyToEdges<WAREdge>([&](const WAREdge &Edge) {
    assert(Edge);
    isl_map *EdgeDep = Edge.getDep();
    assert(EdgeDep);
    isl_set *EdgeRng = isl_map_range(EdgeDep);
    assert(isl_set_is_subset(EdgeRng, Domain));
    isl_set_free(EdgeRng);
    assert(Edge.Src->getDefNode().Operands.count(Edge.Src));
    assert(Edge.Src->getEdges<WAREdge>()[Edge.getKey<WAREdge>()]);
  });

  applyToEdges<WAWEdge>([&](const WAWEdge &Edge) {
    assert(Edge);
    isl_map *EdgeDep = Edge.getDep();
    assert(EdgeDep);
    isl_set *EdgeDom = isl_map_domain(isl_map_copy(EdgeDep));
    isl_set *EdgeRng = isl_map_range(EdgeDep);
    isl_set *SrcDom = Edge.Src->getDefNode().getDomain();
    isl_set *TrgDom = Edge.Trg->getDefNode().getDomain();
    assert(isl_set_is_subset(EdgeDom, SrcDom));
    assert(isl_set_is_subset(EdgeRng, TrgDom));
    assert(Edge.Src->getDefNode().MA == Edge.Src->MA);
    assert(Edge.Trg->getDefNode().MA == Edge.Trg->MA);
    isl_set_free(EdgeDom);
    isl_set_free(EdgeRng);
    isl_set_free(SrcDom);
    isl_set_free(TrgDom);
  });

  isl_set_free(Domain);

  for (auto *Op : Operands)
    Op->verify();
}

void UseNode::verify() const {
  //DEBUG(errs() << "Verify use:\n"; print(errs(), 4));

  isl_set *Domain = getDomain();
  assert(Domain);

  isl_set *FullRng = nullptr;
  applyToEdges<RAWEdge>([&](const RAWEdge &Edge) {
    assert(Edge);
    isl_map *EdgeDep = Edge.getDep();
    assert(EdgeDep);
    isl_set *EdgeRng = isl_map_range(EdgeDep);
    assert(isl_set_is_subset(EdgeRng, Domain));
    if (Edge.Src->MA->isAffine() && Edge.Trg->MA->isAffine()) {
      if (FullRng) {
        isl_set *Overlap =
            isl_set_intersect(isl_set_copy(FullRng), isl_set_copy(EdgeRng));
        assert(isl_set_is_empty(Overlap));
        FullRng = isl_set_union(FullRng, EdgeRng);
      } else {
        FullRng = EdgeRng;
      }
    } else {
      isl_set_free(EdgeRng);
    }
    assert(Edge.isPropagated() ||
           Edge.Trg->getDefNode().Operands.count(Edge.Trg));
    assert(Edge.Src->getEdges<RAWEdge>()[Edge.getKey<RAWEdge>()]);
  });
  isl_set_free(FullRng);
  applyToEdges<WAREdge>([&](const WAREdge &Edge) {
    assert(Edge);
    isl_map *EdgeDep = Edge.getDep();
    assert(EdgeDep);
    isl_set *EdgeDom = isl_map_domain(EdgeDep);
    assert(isl_set_is_subset(EdgeDom, Domain));
    isl_set_free(EdgeDom);
    assert(Edge.Src->getDefNode().Operands.count(Edge.Src));
    assert(Edge.Src->getEdges<WAREdge>()[Edge.getKey<WAREdge>()]);
  });

  isl_set_free(Domain);
}

void UseNode::print(raw_ostream &OS, unsigned Indent) const {
  OS.indent(Indent) << "\tMA: " << MA->getAccessRelationStr() << " [A# "
                    << MA->AccNo << "]\n";
  if (Def)
    OS.indent(Indent) << "\tDef: " << Def->MA->getAccessRelationStr() << "\n";
  else
    OS.indent(Indent) << "\tDef: NONE\n";
  printEdges(OS, Indent);
  OS.indent(Indent) << "\n";
}

DefNode *ExpressionPropagation::getDefNode(ScopStmt &Stmt) {
  if (Stmt.isRegionStmt())
    return nullptr;

  MemoryAccess *WriteMA = nullptr;
  for (MemoryAccess *MA : Stmt) {
    if (MA->isRead())
      continue;
    if (WriteMA) {
      WriteMA->dump();
      MA->dump();
    }
    assert(!WriteMA && "Found multiple writes in Stmt!");
    WriteMA = MA;
  }
  assert(WriteMA && "Did not find a write in Stmt!");

  DefNode *&DefN = DefNodeMap[WriteMA];
  if (!DefN) {
    isl_set *Domain =
        isl_set_intersect_params(Stmt.getDomain(), getAssumedContext());
    DefN = new DefNode(WriteMA, *this, Domain);
  }
  return DefN;
}

void DefNode::collectOperands(UseNodeMapTy &UseNodeMap) {
  if (OperandsCollected)
    return;
  OperandsCollected = true;

  assert(MA && "Set MA first!");

  auto *DefStmt = getStmt();
  assert(DefStmt);
  assert(DefStmt->isBlockStmt());
  Scop &S = *DefStmt->getParent();

  auto *DefValInst = dyn_cast<Instruction>(MA->getAccessInstruction());
  if (!DefValInst)
    return;
  assert(DefValInst && S.contains(DefValInst));

  auto AddOperandMA = [&](MemoryAccess *UseMA) {
    if (UseMA == MA)
      return;
    assert(UseMA->isRead());
    if (auto *IEC = S.lookupInvariantEquivClass(UseMA->getAccessInstruction()))
      if (std::any_of(
              IEC->InvariantAccesses.begin(), IEC->InvariantAccesses.end(),
              [&](const MemoryAccess *InvMA) { return InvMA == UseMA; }))
        return;

    UseNode *&Use = UseNodeMap[UseMA];
    if (!Use)
      Use = new UseNode(UseMA, this);
    assert(Use == UseNodeMap[UseMA]);
    addOperand(Use);
  };

  SmallPtrSet<Instruction *, 32> OpVisited;
  SmallVector<Instruction *, 8> OpWorklist;
  if (isa<CallInst>(DefValInst)) {
    for (auto &Op : DefValInst->operands())
      if (auto *OpI = dyn_cast<Instruction>(Op))
        OpWorklist.push_back(OpI);
  } else {
    OpWorklist.push_back(DefValInst);
  }

  while (!OpWorklist.empty()) {
    Instruction *Inst = OpWorklist.pop_back_val();
    if (!S.contains(Inst))
      continue;

    if (!OpVisited.insert(Inst).second)
      continue;

    auto *ArrayReadMA = DefStmt->getArrayAccessOrNULLFor(Inst);
    if (ArrayReadMA)
      AddOperandMA(ArrayReadMA);

    auto *ValueReadMA = DefStmt->lookupValueReadOf(Inst);
    if (ValueReadMA)
      AddOperandMA(ValueReadMA);

    for (auto &InstOp : Inst->operands()) {
      auto *ValueReadMA = DefStmt->lookupValueReadOf(InstOp);
      if (ValueReadMA)
        AddOperandMA(ValueReadMA);

      auto *InstOpInst = dyn_cast<Instruction>(InstOp);
      if (!InstOpInst)
        continue;

      OpWorklist.push_back(InstOpInst);
    }
  }
}

isl_stat ExpressionPropagation::createWAREdges(__isl_take isl_map *D,
                                               void *User) {
  isl_space *DomainSpace = isl_space_domain(isl_map_get_space(D));
  if (!isl_space_is_wrapping(DomainSpace)) {
    isl_space_free(DomainSpace);
    isl_map_free(D);
    return isl_stat_ok;
  }

  DomainSpace = isl_space_unwrap(DomainSpace);
  isl_id *Id = isl_space_get_tuple_id(DomainSpace, isl_dim_out);
  auto *ReadMA = static_cast<MemoryAccess *>(isl_id_get_user(Id));
  isl_space_free(DomainSpace);
  isl_id_free(Id);

  isl_space *RangeSpace = isl_space_range(isl_map_get_space(D));
  RangeSpace = isl_space_unwrap(RangeSpace);
  Id = isl_space_get_tuple_id(RangeSpace, isl_dim_out);
  auto *WriteMA = static_cast<MemoryAccess *>(isl_id_get_user(Id));
  isl_space_free(RangeSpace);
  isl_id_free(Id);

  assert(ReadMA && WriteMA && ReadMA->isRead() && WriteMA->isWrite());

  auto &ExprProp = *static_cast<ExpressionPropagation *>(User);
  auto *&Src = ExprProp.UseNodeMap[ReadMA];
  if (!Src) {
    DefNode *Def = ExprProp.getDefNode(*ReadMA->getStatement());
    Src = new UseNode(ReadMA, Def);
  }
  assert(Src == ExprProp.UseNodeMap[ReadMA]);

  auto *&Trg = ExprProp.DefNodeMap[WriteMA];
  if (!Trg) {
    isl_set *Domain = isl_set_intersect_params(
        WriteMA->getStatement()->getDomain(), ExprProp.getAssumedContext());
    Trg = new DefNode(WriteMA, ExprProp, Domain);
  }

  assert(Src == ExprProp.UseNodeMap[ReadMA]);
  assert(Trg == ExprProp.DefNodeMap[WriteMA]);

  D = isl_set_unwrap(isl_map_domain(isl_map_zip(D)));
  DEBUG(errs() << "WAR: " << D << "\n");

  Src->addEdge<WAREdge>(*Trg, D);

  return isl_stat_ok;
}

isl_stat ExpressionPropagation::createWAWEdges(__isl_take isl_map *D,
                                               void *User) {
  isl_space *DomainSpace = isl_space_domain(isl_map_get_space(D));
  if (!isl_space_is_wrapping(DomainSpace)) {
    isl_space_free(DomainSpace);
    isl_map_free(D);
    return isl_stat_ok;
  }

  DomainSpace = isl_space_unwrap(DomainSpace);
  isl_id *Id = isl_space_get_tuple_id(DomainSpace, isl_dim_out);
  auto *FirstMA = static_cast<MemoryAccess *>(isl_id_get_user(Id));
  isl_space_free(DomainSpace);
  isl_id_free(Id);

  isl_space *RangeSpace = isl_space_range(isl_map_get_space(D));
  RangeSpace = isl_space_unwrap(RangeSpace);
  Id = isl_space_get_tuple_id(RangeSpace, isl_dim_out);
  auto *SecondMA = static_cast<MemoryAccess *>(isl_id_get_user(Id));
  isl_space_free(RangeSpace);
  isl_id_free(Id);

  auto &ExprProp = *static_cast<ExpressionPropagation *>(User);
  ExprProp.DefNodeMap[SecondMA];

  DefNode *&Src = ExprProp.DefNodeMap[FirstMA];
  if (!Src) {
    isl_set *Domain = isl_set_intersect_params(
        FirstMA->getStatement()->getDomain(), ExprProp.getAssumedContext());
    Src = new DefNode(FirstMA, ExprProp, Domain);
  }

  auto *&Trg = ExprProp.DefNodeMap[SecondMA];
  if (!Trg) {
    isl_set *Domain = isl_set_intersect_params(
        SecondMA->getStatement()->getDomain(), ExprProp.getAssumedContext());
    Trg = new DefNode(SecondMA, ExprProp, Domain);
  }

  assert(FirstMA && SecondMA && FirstMA->isWrite() && SecondMA->isWrite());
  assert(Src == ExprProp.DefNodeMap[FirstMA]);
  assert(Trg == ExprProp.DefNodeMap[SecondMA]);

  D = isl_set_unwrap(isl_map_domain(isl_map_zip(D)));

  Src->addEdge<WAWEdge>(*Trg, D);

  return isl_stat_ok;
}

isl_stat ExpressionPropagation::createRAWEdges(__isl_take isl_map *D,
                                               void *User) {
  isl_space *DomainSpace = isl_space_domain(isl_map_get_space(D));
  if (!isl_space_is_wrapping(DomainSpace)) {
    isl_space_free(DomainSpace);
    isl_map_free(D);
    return isl_stat_ok;
  }

  DomainSpace = isl_space_unwrap(DomainSpace);
  isl_id *Id = isl_space_get_tuple_id(DomainSpace, isl_dim_out);
  auto *WriteMA = static_cast<MemoryAccess *>(isl_id_get_user(Id));
  isl_space_free(DomainSpace);
  isl_id_free(Id);

  isl_space *RangeSpace = isl_space_range(isl_map_get_space(D));
  RangeSpace = isl_space_unwrap(RangeSpace);
  Id = isl_space_get_tuple_id(RangeSpace, isl_dim_out);
  auto *ReadMA = static_cast<MemoryAccess *>(isl_id_get_user(Id));
  isl_space_free(RangeSpace);
  isl_id_free(Id);

  if (!(WriteMA && ReadMA && WriteMA->isWrite() && ReadMA->isRead())) {
    errs() << "D: " << D << "\n";
    errs() << "WMA:" << WriteMA << " RMA: " << ReadMA << "\n";
    WriteMA->dump();
    ReadMA->dump();
  }
  assert(WriteMA && ReadMA && WriteMA->isWrite() && ReadMA->isRead());

  D = isl_set_unwrap(isl_map_domain(isl_map_zip(D)));

  auto &ExprProp = *static_cast<ExpressionPropagation *>(User);
  auto *&Trg = ExprProp.UseNodeMap[ReadMA];
  if (!Trg) {
    DefNode *Def = ExprProp.getDefNode(*ReadMA->getStatement());
    assert(Def);
    Trg = new UseNode(ReadMA, Def);
  }

  auto *&Src = ExprProp.DefNodeMap[WriteMA];
  if (!Src) {
    isl_set *Domain = isl_set_intersect_params(
        WriteMA->getStatement()->getDomain(), ExprProp.getAssumedContext());
    Src = new DefNode(WriteMA, ExprProp, Domain);
  }

  assert(Src == ExprProp.DefNodeMap[WriteMA]);
  assert(Trg == ExprProp.UseNodeMap[ReadMA]);

  Src->addEdge<RAWEdge>(*Trg, D);

  return isl_stat_ok;
}

void ExpressionPropagation::createDependenceGraph(Scop &S) {

  const Dependences &D = DI->getDependences(Dependences::AL_Access);
  if (!D.hasValidDependences()) {
    DEBUG(errs() << "No valid dependences!\n");
    return;
  }

  isl_union_map *RAW = D.getDependences(Dependences::TYPE_RAW);
  DEBUG(errs() << "Access-wise RAW dependences:\n" << RAW << "\n\n");
  isl_stat Success = isl_union_map_foreach_map(RAW, createRAWEdges, this);
  isl_union_map_free(RAW);
  assert(Success == isl_stat_ok);
  (void)Success;

  isl_union_map *RED = D.getDependences(Dependences::TYPE_RED);
  assert(!RED || isl_union_map_is_empty(RED));
  isl_union_map_free(RED);

  isl_union_map *WAR = D.getDependences(Dependences::TYPE_WAR);
  DEBUG(errs() << "Access-wise WAR (and RED) dependences:\n" << WAR << "\n\n");
  Success = isl_union_map_foreach_map(WAR, createWAREdges, this);
  isl_union_map_free(WAR);
  assert(Success == isl_stat_ok);
  (void)Success;

  isl_union_map *WAW = D.getDependences(Dependences::TYPE_WAW);
  DEBUG(errs() << "Access-wise WAW dependences:\n" << WAW << "\n\n");
  Success = isl_union_map_foreach_map(WAW, createWAWEdges, this);
  isl_union_map_free(WAW);
  assert(Success == isl_stat_ok);
  (void)Success;

  for (auto &Stmt : S) {
    for (auto *MA : Stmt) {
      if (MA->isRead()) {
        auto *&Use = UseNodeMap[MA];
        if (Use)
          continue;
        DefNode *Def = getDefNode(Stmt);
        assert(Def);
        Use = new UseNode(MA, Def);
      } else {
        auto *&Def = DefNodeMap[MA];
        if (Def)
          continue;
        isl_set *Domain =
            isl_set_intersect_params(Stmt.getDomain(), getAssumedContext());
        Def = new DefNode(MA, *this, Domain);
      }
    }
  }

  for (auto &It : DefNodeMap) {
    DefNode &Def = *It.second;
    Def.collectOperands(UseNodeMap);
    assert(Def.OperandsCollected);
    isl_set *Domain = Def.getDomain();
    isl_set *Overwritten = Def.getOverwrittenSet<false>();
    assert(isl_set_is_subset(Overwritten, Domain));
    DEBUG(errs() << "\tDom: " << Domain << "\n\tOverwritten: " << Overwritten
                 << "\n");
    Def.IsOverwritten = isl_set_is_subset(Domain, Overwritten);
    isl_set_free(Overwritten);
    isl_set_free(Domain);
  }

  DEBUG(errs() << "Create use stmt self barrier:\n");
  for (auto &It : UseNodeMap) {
    UseNode *Use = It.second;
    assert(Use);
    assert(Use->MA);
    assert(Use->Def && Use->Def->MA);
    if (Use->MA->getScopArrayInfo() != Use->Def->MA->getScopArrayInfo())
      continue;

    isl_map *UseAccRel = Use->MA->getAccessRelation();
    UseAccRel = isl_map_intersect_domain(UseAccRel, Use->getDomain());
    isl_map *DefAccRel = Use->Def->MA->getAccessRelation();
    DefAccRel = isl_map_intersect_domain(DefAccRel, Use->getDomain());

    isl_map *Schedule = Use->getSchedule();
    isl_map *SameAccRel =
        isl_map_apply_range(UseAccRel, isl_map_reverse(DefAccRel));
    SameAccRel = isl_map_apply_domain(SameAccRel, isl_map_copy(Schedule));
    SameAccRel = isl_map_apply_range(SameAccRel, Schedule);
    isl_map *IdMap = isl_map_identity(isl_map_get_space(SameAccRel));
    SameAccRel = isl_map_intersect(SameAccRel, IdMap);

    if (isl_map_is_empty(SameAccRel)) {
      isl_map_free(SameAccRel);
      continue;
    }

    DEBUG(errs() << "SameAccRel: " << SameAccRel << "\n");
    Schedule = isl_map_reverse(Use->getSchedule());
    SameAccRel = isl_map_apply_domain(SameAccRel, isl_map_copy(Schedule));
    SameAccRel = isl_map_apply_range(SameAccRel, Schedule);
    DEBUG(errs() << "SameAccRel: " << SameAccRel << "\n");

    Use->addEdge<WAREdge>(*Use->Def, SameAccRel);
  }

  for (auto &Stmt : S) {
    DefNode &Def = *DefNodeMap.lookup(Stmt.WriteMA);

    auto *SAI = Def.MA->getScopArrayInfo();
    DefNodeGroup *&DNG = DefNodeGroupMap[SAI];
    if (DNG)
      DNG->addNode(Def);
    else
      DNG = new DefNodeGroup(SAI, Def, *TLI);
  }

  for (auto &It : DefNodeMap) {
    DefNode &Def = *It.second;
    restrictStmt(Def, false);
  }

  for (auto &It : DefNodeGroupMap) {
    Worklist.push(It.second);
    WorklistSet.insert(It.second);
  }
}

void ExpressionPropagation::propagateEdgesToReloadOp(UseNode &Op,
                                                     UseNode &ReloadOp,
                                                     RAWEdge &Edge) {
  DEBUG({
    errs() << "propagateEdgesToReloadOp from: " << Op.MA->getAccessRelationStr()
           << " [" << Op.MA << "]\n\tto " << ReloadOp.MA->getAccessRelationStr()
           << " [" << ReloadOp.MA << "]\n\tover " << Edge.getDepStr()
           << "\nOp:\n";
    Op.print(errs(), 4);
    errs() << "\nReloadOp before:\n";
    ReloadOp.print(errs(), 4);
  });

  isl_map *PropagationDep = Edge.getDep();
  Op.copyEdgesTo<RAWEdge>(ReloadOp, PropagationDep);
  Op.copyEdgesFrom<WAREdge>(ReloadOp, PropagationDep);
  isl_map_free(PropagationDep);

  //auto &RAWEdges = ReloadOp.getEdges<RAWEdge>();
  //for (auto &It : ReloadOp.getEdges<WAREdge>()) {
    //auto &KeyPair = It.first;
    //if (auto *RAW = RAWEdges.lookup({KeyPair.second, KeyPair.first})) {
      //It.second->setDep(isl_map_union(
          //It.second->getDep(),
          //isl_map_apply_range(It.second->getDep(), RAW->getDep())));
    //}
  //}

  DEBUG({
    errs() << "\nReloadOp after:\n";
    ReloadOp.print(errs(), 4);
  });
}

UseNode *ExpressionPropagation::copyOperandInto(UseNode &OpUse,
                                                DefNode &StmtDef,
                                                MemoryAccess &OpUseCopyMA) {
  UseNode *OpUseCopy = new UseNode(&OpUseCopyMA, &StmtDef);
  auto *&Ptr = UseNodeMap[&OpUseCopyMA];
  assert(!Ptr);
  Ptr = OpUseCopy;
  return OpUseCopy;
}

void ExpressionPropagation::addRecomputePair(
    ScopStmt &Stmt, RAWEdge &RAW,
    DenseMap<MemoryAccess *, MemoryAccess *> &NewAccs) {
  assert(RAW);
  isl_map *Dep = RAW.getDep();
  DEBUG(errs() << "Add recompute pair in: " << Stmt.getBaseName() << "\n");
  DEBUG(errs() << "\t: " << RAW.Trg->MA->getAccessRelationStr() << " => "
               << RAW.Src->MA->getAccessRelationStr() << " : " << Dep << "\n");
  Stmt.addRecomputeValue(RAW.Trg->MA, RAW.Src->MA, Dep, NewAccs);
}

void ExpressionPropagation::addReloadAccesses(
    ScopStmt &Stmt, RAWEdge &RAW,
    DenseMap<MemoryAccess *, MemoryAccess *> &NewAccs) {
  assert(RAW);
  Scop &S = *Stmt.getParent();
  DEBUG(RAW.Src->print(errs()));
  for (auto *OpUse : RAW.Src->Operands) {
    MemoryAccess *MA = OpUse->MA;
    assert(MA->isRead());
    DEBUG(errs() << "SrcOp: "; OpUse->print(errs()));

    DEBUG(errs() << "Add reload acc: " << MA->getAccessRelationStr() << "\n";
          OpUse->print(errs()));

    isl_map *Dep = RAW.getDep();
    auto *AccRel = MA->getAccessRelation();
    AccRel = isl_map_set_tuple_id(AccRel, isl_dim_in,
                                  isl_map_get_tuple_id(Dep, isl_dim_in));
    auto *NewAccRel = isl_map_apply_domain(AccRel, Dep);

    NewAccRel = isl_map_set_tuple_id(NewAccRel, isl_dim_in, Stmt.getDomainId());
    //errs() << "  NewAccRel: " <<   NewAccRel << "\n";
    NewAccRel = isl_map_gist_domain(NewAccRel, RAW.Trg->Def->getDomain());
    NewAccRel = isl_map_gist_params(NewAccRel, getAssumedContext());

    UseNode *OpUseCopy = nullptr;
    if (MA->isAffine()) {
      for (auto *TrgOp : RAW.Trg->getDefNode().Operands) {
        if (TrgOp->MA->getBaseAddr() != MA->getBaseAddr())
          continue;
        if (TrgOp->MA->getType() != MA->getType())
          continue;
        if (TrgOp->MA->getElementType() != MA->getElementType())
          continue;
        isl_map *TrgOpAccRel = TrgOp->MA->getAccessRelation();
        bool IsEqual = isl_map_plain_is_equal(NewAccRel, TrgOpAccRel);
        isl_map_free(TrgOpAccRel);
        if (IsEqual) {
          DEBUG(errs() << "0 Found OpUseCopy in trg!"; TrgOp->print(errs()););
          TrgOp->MA->setAccessRelation(nullptr);
          TrgOp->MA->setNewAccessRelation(NewAccRel);
          OpUseCopy = TrgOp;
          break;
        }
      }
    }
    //isl_set_free(NewAccRelDom);

    if (!OpUseCopy) {
      auto *PropagatedMA = new MemoryAccess(
          &Stmt, MA->getAccessInstruction(), MA->getType(), MA->getBaseAddr(),
          MA->getElementType(), MA->isAffine(), MA->getSubscripts(),
          MA->getSizes(), MA->getAccessValue(), MA->getOriginalKind(),
          MA->getBaseName());
      PropagatedMA->setAccessRelation(nullptr);
      PropagatedMA->setNewAccessRelation(NewAccRel);

      Stmt.addAccess(PropagatedMA);
      S.addAccessFunction(PropagatedMA);
      OpUseCopy = copyOperandInto(*OpUse, *RAW.Trg->Def, *PropagatedMA);
    }

    propagateEdgesToReloadOp(*OpUse, *OpUseCopy, RAW);
    RAW.Trg->Def->addOperand(OpUseCopy);
    NewAccs[MA] = OpUseCopy->MA;
  }
}

static bool containsUDiv(Value *V) {
  return false;
    auto *I = dyn_cast<Instruction>(V);
    if (!I || isa<PHINode>(I))
      return false;
    if (I->getOpcode() == BinaryOperator::UDiv)
      return true;
    return std::any_of(I->op_begin(), I->op_end(), containsUDiv);
  };

bool ExpressionPropagation::propagateEdges(DefNodeGroup &DNG) {

  auto IsVectorizable = [](DefNode *Def) {
    if (!Def->MA->isAffine())
      return false;
    if (std::any_of(Def->Operands.begin(), Def->Operands.end(),
                    [](UseNode *Op) {
                      if (!Op->isAffine())
                        return true;
                      auto *LI = dyn_cast<LoadInst>(Op->MA->getAccessInstruction());
                      return LI && containsUDiv(LI->getPointerOperand());
                    }))
      return false;
    return true;
  };

  unsigned NumNonVecDims = 0;
  unsigned NumNewNonVecDims = 0;
  SmallPtrSet<DefNode *, 8> NonPropTrgs;
  DenseMap<DefNode *, SmallVector<RAWEdge *, 8>> TrgEdgeMap;
  SmallPtrSet<DefNodeGroup *, 8> NonVecTrgGroups;
  for (auto *Def : DNG.Nodes) {
    bool SrcIsVectorizable = IsVectorizable(Def);
    if (!SrcIsVectorizable && NumNonVecDims == 0) {
      NumNonVecDims += Def->getStmt()->getNumIterators();
    }
    for (auto &EdgeIt : Def->getEdges<RAWEdge>()) {
      auto *RAW = EdgeIt.second;
      if (RAW->Trg->isAffine()) {
        if (!NonVecTrgGroups.count(RAW->Trg->getDefNode().Group)) {
          bool TrgIsVectorizable = IsVectorizable(&RAW->Trg->getDefNode());
          if (!SrcIsVectorizable && TrgIsVectorizable) {
            NumNewNonVecDims += RAW->Trg->getDefNode().getStmt()->getNumIterators();
            NonVecTrgGroups.insert(RAW->Trg->getDefNode().Group);
          }
        }

        TrgEdgeMap[&RAW->Trg->getDefNode()].push_back(RAW);
        continue;
      }

      (errs() << "Trg is not affine: " << RAW->Trg->MA->getAccessRelationStr()
              << "\n");
      if (!DNG.IsLiveOut) {
        return false;
      } else {
        NonPropTrgs.insert(&RAW->Trg->getDefNode());
      }
    }
  }
  (errs() << "Got " << TrgEdgeMap.size() << " different target nodes and add "
          << NumNewNonVecDims << " non vectorizable dims to stmts [before: " <<
          NumNonVecDims << "]!\n");
  if (NumNewNonVecDims > NumNonVecDims) {
    (errs() << " Skip due to new non vectorizable dims in "
                 << NumNewNonVecDims << " vs " << NumNonVecDims << "!\n");
    return false;
  }

  auto IsExpandingEdge = [&](RAWEdge &Edge) {
    return false;
    if (Edge.Src->Operands.size() <= 1)
      return false;

    // compare non fixed input dims of Src & Trg
    isl_map *Dep = Edge.getDep();
    isl_set *Dom = isl_map_domain(isl_map_copy(Dep));
    isl_set *Rng = isl_map_range(Dep);

    unsigned DomNonFixed = 0;
    unsigned NumDims = isl_set_dim(Dom, isl_dim_set);
    for (unsigned d = 0; d < NumDims; d++) {
      isl_val *V = isl_set_plain_get_val_if_fixed(Dom, isl_dim_set, d);
      DomNonFixed += !V ||  isl_val_is_nan(V);
      isl_val_free(V);
    }
    isl_set_free(Dom);

    unsigned RngNonFixed = 0;
    NumDims = isl_set_dim(Rng, isl_dim_set);
    for (unsigned d = 0; d < NumDims; d++) {
      isl_val *V = isl_set_plain_get_val_if_fixed(Rng, isl_dim_set, d);
      RngNonFixed += !V ||  isl_val_is_nan(V);
      isl_val_free(V);
    }
    isl_set_free(Rng);

    errs() << "EXPANSION? " << DomNonFixed << " vs " << RngNonFixed << " for " << Edge.getDepStr() << "\n";
    return DomNonFixed < RngNonFixed;
  };

  DenseMap<const ScopArrayInfo *, SmallVector<DefNode *, 8>> PerSAITargets;
  for (auto &TrgEdgeMapIt : TrgEdgeMap) {
    DefNode *Trg = TrgEdgeMapIt.first;
    if (NonPropTrgs.count(Trg))
      continue;
    auto &Edges = TrgEdgeMapIt.second;
    for (RAWEdge *Edge : Edges) {
      DefNode *Src = Edge->Src;
      if (TrgEdgeMap.count(Src)) {
        (errs() << "Prohibit propagation inside DNG to " << Trg->getName()
                << "\n");
        if (!DNG.IsLiveOut) {
          return false;
        }
        NonPropTrgs.insert(Trg);
        continue;
      } else if (IsExpandingEdge(*Edge)) {
        (errs() << "Prohibit expansion of edge to " << Trg->getName() << "\n");
        if (!DNG.IsLiveOut) {
          return false;
        }
        NonPropTrgs.insert(Trg);
        continue;
      }
    }
    PerSAITargets[Trg->MA->getScopArrayInfo()].push_back(Trg);
  }

  unsigned Streams = 0;
  for (auto &It : PerSAITargets) {
    auto &Trgs = It.second;
    SmallVector<RAWEdge *, 15> Edges;
    for (auto *Trg : Trgs)
      Edges.append(TrgEdgeMap[Trg].begin(), TrgEdgeMap[Trg].end());
    DEBUG(errs() << "\tCheck targets with SAI: " << It.first->getName() << "\n");
    if (isWorthRecomputing(Trgs.back(), Edges, Streams))
      continue;
    if (!DNG.IsLiveOut) {
      (errs()<< "Not worth to recompute in wrtie Stmts to " << It.first->getName() << "\n");
      return false;
    }
    NonPropTrgs.insert(Trgs.begin(), Trgs.end());
  }

  if (TrgEdgeMap.size() == NonPropTrgs.size()) {
    (errs()<< "No targets worth recomputing\n");
    return false;
  }

  DEBUG(errs() << "Try to propagate to " << TrgEdgeMap.size() - NonPropTrgs.size()
          << " different target nodes!\n");

  DenseMap<DefNode *, SmallVector<std::pair<RAWEdge *, isl_set *>, 8>>
      TargetDomains;
  auto ClearTargetDomains = [&]() {
    for (auto &TDIt : TargetDomains)
      for (auto &TD : TDIt.second)
        isl_set_free(TD.second);
  };

  for (auto &TrgEdgeMapIt : TrgEdgeMap) {
    DefNode &Trg = *TrgEdgeMapIt.first;
    if (NonPropTrgs.count(&Trg))
      continue;
    DEBUG(errs() << "Check recompute domain for trg: " << Trg.getName() << "\n");
    auto &Edges = TrgEdgeMapIt.second;
    for (auto *RAW : Edges) {
      DEBUG(errs() << "Check edge " << RAW->getDepStr() << "\n");
      isl_set *RecomputeDomain = getRecomputeDomain(*RAW);
      DEBUG(errs() << "\tEdge RecomputeDomain: " << RecomputeDomain << "\n");
      isl_set *EdgeDom = isl_map_domain(RAW->getDep());
      bool FullProp = isl_set_plain_is_equal(RecomputeDomain, EdgeDom);
      isl_set_free(EdgeDom);
      DEBUG(errs() << "\tEdge can " << (FullProp ? "" : "not ") << "be propagated completely\n");

      auto *TargetDomain = isl_set_apply(RecomputeDomain, RAW->getDep());
      TargetDomains[&Trg].push_back({RAW, TargetDomain});

      #if 0
      if (!FullProp) {
        if (!DNG.IsLiveOut) {
          ClearTargetDomains();
          (errs()<< "Cannot completely propagate to " << Trg.getName() << "! Skip!\n");
          return false;
        }
        NonPropTrgs.insert(&RAW->Trg->getDefNode());
        break;
      }
      #endif
    }
  }

  SmallPtrSet<DefNode *, 8> NonPropSrcs;
  for (auto *Def : DNG.Nodes) {
    for (auto &EdgeIt : Def->getEdges<RAWEdge>())
      if (NonPropTrgs.count(&EdgeIt.second->Trg->getDefNode())) {
        NonPropSrcs.insert(Def);
        break;
      }
  }

  DEBUG(errs() << "Got " << DNG.Nodes.size() << " source nodes and "
          << NonPropSrcs.size() << " non prop sources!\nGot "
          << TrgEdgeMap.size() << " target nodes and " << NonPropTrgs.size()
          << " non prop targets!\n");

  unsigned NumRequiredSplits = 0;
  unsigned NumPropagatedSrcs = DNG.Nodes.size() - NonPropSrcs.size();
  unsigned NumPropagatedTrgs = TargetDomains.size() - NonPropTrgs.size();

  if (NumPropagatedSrcs == 0 || NumPropagatedTrgs == 0) {
    (errs()<< "No targets or sources worth recomputing\n");
    ClearTargetDomains();
    return false;
  }

  DenseMap<DefNode *, SmallVector<isl_set *, 8>> SplitsMap;
  auto ClearSplitMap = [&]() {
    for (auto &SplitsMapIt : SplitsMap)
      for (auto *Split : SplitsMapIt.second)
        isl_set_free(Split);
  };

  bool Valid = true;
  DEBUG(errs() << "Check " << NumPropagatedTrgs << " target domains\n");
  for (auto &TDIt : TargetDomains) {
    DefNode &Trg = *TDIt.first;
    if (NonPropTrgs.count(&Trg))
      continue;

    auto &Splits = SplitsMap[&Trg];
    auto &TDs = TDIt.second;
    DEBUG(errs() << "\tTrg " << Trg.getName() << " has " << TDs.size() << " target domains!\n");
    Splits.push_back(Trg.getDomain());
    DEBUG(errs() << "\t\tTrg domain: " << Splits[0] << "\n");
    for (auto &TD : TDs) {
      isl_set *TargetDom = TD.second;
      RAWEdge *RAW = TD.first;
      if (NonPropSrcs.count(RAW->Src))
        continue;
      DEBUG(errs() << "\tRAW: " << RAW->getDepStr() << "\n");
      DEBUG(errs() << "\tTargetDom: " << TargetDom << "\n");
      bool Contained = std::any_of(Splits.begin(), Splits.end(), [=](isl_set *Split){ return isl_set_is_equal(Split, TargetDom); });
      if (Contained) {
        DEBUG(errs() << "\tTargetDom is contained in splits!\n");
        continue;
      }
      DEBUG(errs() << "Check " << Splits.size() << " splits\n");
      for (unsigned u = 0, e = Splits.size(); u < e; u++) {
        isl_set *Split = Splits[u];
        isl_set *Same = isl_set_intersect(isl_set_copy(Split), isl_set_copy(TargetDom));
        if (isl_set_is_empty(Same)) {
          isl_set_free(Same);
          continue;
        }
        DEBUG(errs() << "\t Split " << u << " intersects target dom: " << Split << "\n");

        Splits[u] = Same;
        isl_set *Left = isl_set_subtract(Split, isl_set_copy(Same));
        DEBUG(errs() << "\t\tSame: " << Same << " Left: " << Left << "\n");
        //TargetDom = isl_set_subtract(TargetDom, isl_set_copy(Left));
        bool Contained =
            isl_set_is_empty(Left) ||
            std::any_of(Splits.begin(), Splits.end(), [=](isl_set *Split) {
              return isl_set_is_equal(Split, Left);
            });
        if (Contained) {
          DEBUG(errs() << "\t\t Left ist contained in splitS!\n");
          isl_set_free(Left);
          continue;
        }

        Splits.push_back(Left);
      }
    }

    {
      DEBUG(errs() << "\tTrg " << Trg.getName() << " needs to be split " << Splits.size() -1 << " times:\n");
      isl_set *FullDom = Trg.getDomain();
      if (Splits.size() > 1)
        for (isl_set *Split : Splits) {
          DEBUG(errs() << "\t\tSplit: " << Split << " [FD: " << FullDom << "]\n");
          assert(isl_set_is_subset(Split, FullDom));
          FullDom = isl_set_subtract(FullDom, isl_set_copy(Split));
        }
      DEBUG(errs() << "LeftDom: " << FullDom << "\n");
      assert(Splits.size() == 1 || isl_set_is_empty(FullDom));
      isl_set_free(FullDom);
    };

    assert(!Splits.empty());
    NumRequiredSplits += Splits.size() - 1;

    if (!Trg.isAffine() && Splits.size() > 1) {
      errs() << "Do not split non affine trg!\n";
      Valid = false;
      break;
    }
    //if (NumRequiredSplits > NumPropagatedSrcs * 2) {
      //(errs() << "\tStop split computation, to many already!\n");
      //break;
    //}
  }
  ClearTargetDomains();

  DEBUG(errs() << "\tRequired splits: " << NumRequiredSplits << " for "
          << NumPropagatedSrcs << " propagates sources!\n");

  if (!Valid) {
    (errs() << "Would need to split non affine statements!\n");
    ClearSplitMap();
    return false;
  }

  //unsigned SrcInnerFixedDims = 0;
  //for (auto *Def : DNG.Nodes) {
    //if (Def->isEmpty() || NonPropSrcs.count(Def))
      //continue;
    //isl_set *Domain = Def->getDomain();
    //errs() << "Dom: " << Domain << "\n";
    //unsigned NumDims = isl_set_dim(Domain, isl_dim_set);
    //for (unsigned d = 0; d < NumDims; d++) {
      //isl_val *V = isl_set_plain_get_val_if_fixed(Domain, isl_dim_set, d);
      //SrcInnerFixedDims += V && !isl_val_is_nan(V);
      //isl_val_free(V);
    //}
    //isl_set_free(Domain);
  //}
  unsigned TrgInnerFixedDims = 0;
  for (auto &SplitsMapIt : SplitsMap) {
    auto &Splits = SplitsMapIt.second;
    unsigned NumDims = isl_set_dim(Splits[0], isl_dim_set);
    for (unsigned u = 0, e = Splits.size(); u < e; u++) {
      auto *Split = Splits[u];

      bool AllFixed = true;
      bool FixedDims[NumDims];
      bool SmallDims[NumDims];
      for (int d = 0; d < NumDims; d++) {
        FixedDims[d] = isSmallerInDim(isl_set_copy(Split), d, 2);
        SmallDims[d] = isSmallerInDim(isl_set_copy(Split), d, VectorWidth);
        if (!FixedDims[d])
          AllFixed = false;
      }
      //if (AllFixed) {
        //errs() << "Skip due complete fix for " << Split << "\n";
        //ClearSplitMap();
        //return false;
      //}

      bool FixedFromBelow = true;
      for (int d = 1; d < NumDims; d++) {
        if (FixedDims[d] && !FixedDims[d-1])
          FixedFromBelow = false;
      }
      bool FixedFromAbove = true;
      for (int d = 0; d + 1 < NumDims; d++) {
        if (FixedDims[d] && !FixedDims[d+1])
          FixedFromAbove = false;
      }

      if (!FixedFromBelow && !FixedFromAbove) {
        errs() << "Skip due to added middle fix for " << Split << "\n";
        ClearSplitMap();
        return false;
      }
    }
  }
  //errs() << "InnerFixedDims: SRC: " << SrcInnerFixedDims << " * " << NumPropagatedTrgs << " vs TRG: " << TrgInnerFixedDims << " * " << NumPropagatedSrcs << "\n";
  //if (NumRequiredSplits > NumPropagatedSrcs) {
    //errs() << "Skip due to added statements!\n";
    //ClearSplitMap();
    //return false;
  //}
  //if (TrgInnerFixedDims > SrcInnerFixedDims) {
    //errs() << "Skip due to added fixed dims!\n";
    //ClearSplitMap();
    //return false;
  //}

  DEBUG(errs() << "\tSplit target statements:\n");
  for (auto &SplitsMapIt : SplitsMap) {
    DefNode *Trg = SplitsMapIt.first;

    auto &Splits = SplitsMapIt.second;
    isl_set_free(Splits[0]);

    for (unsigned u = 1, e = Splits.size(); u < e; u++) {
      auto *Split = Splits[u];
      Split = isl_set_set_tuple_id(Split, Trg->getStmt()->getDomainId());

      DEBUG(errs() << "\tSplit of " << Split << "\n\t    from "
              << Trg->getDomainStr() << "\n");
      isl_set *TrgDom = Trg->getDomain();
      assert(isl_set_is_subset(Split, TrgDom));
      isl_set_free(TrgDom);

      DefNode *TrgSplit = splitUsingDefNode(*Trg, isl_set_copy(Split));
      assert(TrgSplit);
      TrgDom = Trg->getDomain();
      isl_set *TrgSplitDom = TrgSplit->getDomain();
      DEBUG(errs() << "Split: " << Split << "\nTrgDom: " << TrgDom
             << "\nTrgSplitDom: " << TrgSplitDom << "\n");
      assert(isl_set_is_equal(Split, TrgDom));
      isl_set_free(TrgSplitDom);
      isl_set_free(TrgDom);
      isl_set_free(Split);

      SplitStmts.insert(Trg);

      DEBUG({
        errs() << "Verify trg after split\n";
        Trg->verify();
        errs() << "Verify split after split\n";
        TrgSplit->verify();
      });

      Trg = TrgSplit;
    }
  }

  DenseMap<std::pair<BasicBlock *, DefNode *>, unsigned> SeenPairs;
  for (auto *Def : DNG.Nodes) {
    if (NonPropSrcs.count(Def))
      continue;

    DEBUG(errs() << "\tPropagate edges from:\n"; Def->print(errs(), 4););
    SmallVector<RAWEdge *, 8> Edges;
    for (auto &EdgeIt : Def->getEdges<RAWEdge>())
      Edges.push_back(EdgeIt.second);

    for (auto *RAW : Edges) {
      assert(!NonPropTrgs.count(&RAW->Trg->getDefNode()));
      DEBUG(errs() << "\tPropagate edge: " << RAW->getDepStr() << "\n");
      RAW->RR.Value = RR_PROPAGATED;
      propagateEdge(*RAW);
      RAW->clear();
    }
    restrictStmt(*Def, false);
  }

  errs() << "Successfully finished propagation!\n";
  return true;
}

void ExpressionPropagation::propagateEdge(RAWEdge &RAW) {
  assert(RAW);
  ScopStmt &TrgStmt = *RAW.Trg->getStmt();
  DEBUG(errs() << "Prop RAW [" << RAW.Src->MA << " -> " << RAW.Trg->MA << "]"
               << RAW.getDepStr() << "\n\tfrom:\n";
        RAW.Src->getStmt()->dump(); errs() << "\tTo:\n"; TrgStmt.dump(););

  //RAW.Trg->getDefNode().print(errs());
  //RAW.Trg->print(errs());

  assert(RAW.Trg->getEdges<RAWEdge>().size() == 1);
  assert(RAW.Trg->getEdges<RAWEdge>().begin()->getSecond() == &RAW);
  assert(RAW.isPropagated());

  auto &TrgDef = RAW.Trg->getDefNode();

  // TODO model the ScopStmts at the end after the def nodes
  auto *AI = dyn_cast<AllocaInst>(RAW.Trg->MA->getBaseAddr());
  if (AI && isa<ConstantInt>(AI->getArraySize()) &&
      cast<ConstantInt>(AI->getArraySize())->isOne()) {
    REMOVED_SCALAR_READS++;
  } else {
    REMOVED_MEMORY_READS++;
  }

  //if (!RAW.Src->isLiveOut() || !RAW.Src->IsOverwritten) {
  TrgDef.Blocks.insert(RAW.Src->Blocks.begin(), RAW.Src->Blocks.end());
  TrgDef.removeOperand(*RAW.Trg);
  TrgStmt.removeOneMemoryAccess(RAW.Trg->MA);
  //}

  DenseMap<MemoryAccess *, MemoryAccess *> NewAccs;
  addReloadAccesses(TrgStmt, RAW, NewAccs);
  addRecomputePair(TrgStmt, RAW, NewAccs);

  DEBUG({
    errs() << "\tTo:\n";
    TrgStmt.dump();
    errs() << "Done prop RAW: " << RAW.getDepStr() << "\n";
  });

  EDGES_PROPAGATED++;
}

DefNode *
ExpressionPropagation::splitUsingDefNode(DefNode &UsingDef,
                                         __isl_take isl_set *RecomputeDomain) {
  ScopStmt &DefStmt = *UsingDef.getStmt();
  isl_set *DefDom = UsingDef.getDomain();

  DEBUG(errs() << "DefDom: " << DefDom << "\nRecDom: " << RecomputeDomain
               << "\n");
  assert(isl_set_is_subset(RecomputeDomain, DefDom));

  isl_set *ReachingDom =
      isl_set_intersect(isl_set_copy(DefDom), RecomputeDomain);
  isl_set *LeftoverDom = isl_set_subtract(DefDom, isl_set_copy(ReachingDom));
  // LeftoverDom = isl_set_gist_params(LeftoverDom, getAssumedContext());
  // ReachingDom = isl_set_gist_params(ReachingDom, getAssumedContext());
  isl_set *LeftoverAndLiveDom = isl_set_intersect_params(
      isl_set_copy(LeftoverDom), getAssumedContext());

  DEBUG(errs() << "ReachingDom  : " << ReachingDom << "\n"
               << "LeftoverDom  : " << LeftoverDom << "\n"
               << "Left&LiveDom : " << LeftoverAndLiveDom << "\n");
  if (isl_set_is_empty(LeftoverAndLiveDom)) {
    isl_set_free(ReachingDom);
    isl_set_free(LeftoverDom);
    isl_set_free(LeftoverAndLiveDom);
    DEBUG(errs() << "LeftoverAndLiveDom is empty! No split!\n");
    return nullptr;
  }
  isl_set_free(LeftoverAndLiveDom);

  STATEMENT_SPLITS++;
  if (DefStmt.ParentStmt == nullptr && DefStmt.CopyStmts.empty())
    STATEMENTS_SPLIT++;

  //UsingDef.print(errs());
  //DefStmt.dump();
  assert(UsingDef.Operands.size() + 1 == DefStmt.size());

  Scop &S = *DefStmt.getParent();
  Loop *L = DefStmt.getSurroundingLoop();
  ScopStmt *CopyStmt = DefStmt.isBlockStmt()
                           ? S.addScopStmt(DefStmt.getBasicBlock(), L)
                           : S.addScopStmt(DefStmt.getRegion(), L);
  auto AccMap = CopyStmt->copyStmt(DefStmt, DefStmt.getNumCopyStmts());

  MemoryAccess *CopyDefMA = AccMap[UsingDef.MA];
  assert(CopyDefMA);
  DefNode *DefCopy =
      new DefNode(UsingDef, CopyDefMA, AccMap, LeftoverDom, ReachingDom);
  UsingDef.Group->addNode(*DefCopy);

  DEBUG(errs() << "Verify def copy\n"; DefCopy->verify(););

  //UsingDef.setDomain(ReachingDom);
  setDomain(UsingDef, ReachingDom);

  DEBUG(errs() << " DefStmt Dom: " << UsingDef.getDomainStr() << "\n";
        errs() << "CopyStmt Dom: " << DefCopy->getDomainStr() << "\n";
        errs() << "\n\nAfter edge copy:\n"; UsingDef.print(errs());
        DefCopy->print(errs()));

  return DefCopy;
}

bool ExpressionPropagation::isWorthRecomputing(
    DefNode *Trg, SmallVectorImpl<RAWEdge *> &Edges, unsigned &Streams) {

  static int X = -1;
  X++;
  if (MAX_PROP >= 0 && X >= MAX_PROP)
    return false;
  if (MIN_PROP >= 0 && X < MIN_PROP)
    return false;

#if 0
  unsigned NumAccs = 0, NumAccsEdge = 0;
  unsigned NumInsts = 0;
  SmallPtrSet<BasicBlock *, 8> SeenBBs;
  for (RAWEdge *Edge : Edges) {
    NumAccsEdge += Edge->Src->Operands.size();
    if (!SeenBBs.insert(Edge->Src->getStmt()->getBasicBlock()).second)
      continue;
    unsigned SrcNumInsts = Edge->Src->getNumInsts();
    errs() << "\t EdgeSrc: " << SrcNumInsts  << " [" << Edge->Src->getName() << "]\n";
    NumInsts += SrcNumInsts;
  }
  for (auto *Def : Trg->Group->Nodes) {
    NumAccs += Def->Operands.size();
    if (!SeenBBs.insert(Def->getStmt()->getBasicBlock()).second)
      continue;
    unsigned DefNumInsts = Def->getNumInsts();
    errs() << "\t Def: " << DefNumInsts << " [" << Def->getName() << "]\n";
    NumInsts += DefNumInsts;
  }
  errs() << "NumInsts: " << NumInsts << " for "
         << Trg->MA->getScopArrayInfo()->getName() << "\nNumAccs: " << NumAccs
         << " Edge: " << NumAccsEdge << "\n";
#endif

  //if (NumInst > 1000) {
    //errs() << "Got more than 1000 instructions! Not worth it!\n";
    //return false;
  //}


  auto NumAccessStreams = computeNumAccessedStreams(Trg, Edges, NumStreamTreshold);
  Streams += NumAccessStreams;

  //int NumCores;
  //if (PollyNumThreads != 0)
    //NumCores = PollyNumThreads;
  //else
    //NumCores = sys::getHostNumPhysicalCores() * (PollyHyperthreading ? 2 : 1);
  //isl_set *TrgDom = Trg->getDomain();
  //bool HasSmallOuterDim = isSmallerInDim(isl_set_copy(TrgDom), 0, NumCores);
  //unsigned Factor = 1;
  //if (HasSmallOuterDim) {
    //while (!isSmallerInDim(isl_set_copy(TrgDom), 0, Factor+1))
      //Factor++;
  //}
  //(errs() << "GOT " << NumAccessStreams << " and factor " << Factor << " for " << TrgDom << "\n");
  //isl_set_free(TrgDom);
  if (NumAccessStreams  > NumStreamTreshold) {
    static int W = 0;
    if (getenv("WORTH")) {
      errs() << "WORTH is " << getenv("WORTH") << " W: " << W << "\n";
      if (atoi(getenv("WORTH")) == W++)
        return true;
    }
    return false;
  }

#if 0
  if (TTI && TTI->getNumberOfRegisters(true)) {
    auto NumRegsNeeded = computeNumOfRegsNeeded(Edges);
    (errs() << "Num Regs needed after recomputation: " << NumRegsNeeded.first
            << "/" << NumRegsNeeded.second
            << " [Available: " << TTI->getNumberOfRegisters(false) << "/"
            << TTI->getNumberOfRegisters(true) << "]\n");
    if (NumRegsNeeded.first > TTI->getNumberOfRegisters(true) ||
        NumRegsNeeded.second > TTI->getNumberOfRegisters(false)) {
      return false;
    }
  }
#endif

  return true;
}

unsigned
ExpressionPropagation::getNeededFixedDimSplits(RAWEdge &Edge,
                                               __isl_keep isl_set *RecDom) {

  //auto Key = std::make_pair(Edge.Src->MA->getAccessValue(),
                            //Edge.Trg->MA->getAccessValue());
  //if (NUM_PROP >= 0 && RAWProps[Key]++ > NUM_PROP) {
    //DEBUG(errs() << "\t SKIP edge already propagated " << NUM_PROP
                 //<< " times!\n");
    //Edge.RR = RR_PROPAGATION_TIMES;
    //return 2;
  //}

  static int X = 0;
  if (MAX_PROP >= 0 && X >= MAX_PROP) {
    X++;
    Edge.RR = RR_COMMAND_LINE;
    return 2;
  }
  if (MIN_PROP >= 0 && X < MIN_PROP) {
    Edge.RR = RR_COMMAND_LINE;
    X++;
    return 2;
  }
  X++;

  //isl_set *TrgDom = Edge.Trg->getDomain();
  //bool Split = !isl_set_plain_is_equal(RecDom, TrgDom);
  //errs() << "TrgDom: " << TrgDom << "\nRecDom: " << RecDom
         //<< "\nSplit: " << Split << "\n";
  //isl_set_free(TrgDom);
  //return Split;

  isl_set *TrgDom = Edge.Trg->getDomain();
  //errs() << "TrgDom: " << TrgDom << "\n";
  unsigned TrgNonFixed = 0;
  for (unsigned u = 0, e = isl_set_dim(TrgDom, isl_dim_set); u < e; u++) {
    isl_val *V = isl_set_plain_get_val_if_fixed(TrgDom, isl_dim_set, u);
    //errs() << "u: " <<u << " V: "; isl_val_dump(V);
    TrgNonFixed += !V || isl_val_is_nan(V);
    isl_val_free(V);
  }
  isl_set_free(TrgDom);

  unsigned RngNonFixed = 0;
  //errs() << "RecDom: " << RecDom << "\n";
  for (unsigned u = 0, e = isl_set_dim(RecDom, isl_dim_set); u < e; u++) {
    isl_val *V =
        isl_set_plain_get_val_if_fixed(RecDom, isl_dim_set, u);
    RngNonFixed += !V || isl_val_is_nan(V);
    isl_val_free(V);
  }

  DEBUG(errs() << "TrgNonFixed: " << TrgNonFixed << "  RngNonFixed: " << RngNonFixed << "\n");
  if (RngNonFixed < TrgNonFixed) {
    (errs() << "Do not further restrict statements\n");
    return 1;
  }

  return 0;

#if 0
  if ((!Edge.Src->IsLiveOut || Edge.Src->IsOverwritten) &&
      (Edge.Trg->getDefNode().IsLiveOut ||
       Edge.Trg->getDefNode().IsOverwritten)) {
    //auto *EdgeTrgStmt = Edge.Trg->getStmt();
    bool Valid = true;
    //Edge.Src->applyToEdges<RAWEdge>([&](const RAWEdge &RAW) {
      //Valid &= RAW.Trg->getStmt() == EdgeTrgStmt;
    //});
    if (Valid) {
      DEBUG(errs() << "OV/LO heurist for last edge!\n");
      return 1;
    }
  }

  if (HEURISTIC <= 7) {
    isl_set *TrgDom = Edge.Trg->getDomain();
    DEBUG(errs() << "TrgDom: " << TrgDom << "\n");
    isl_set *EdgeDom = isl_map_domain(
        isl_map_intersect_domain(Edge.getDep(), isl_set_copy(RecomputeDomain)));
    DEBUG(errs() << "EdgeDom: " << EdgeDom << "\n");
    bool IsFixedInDim = false;
    for (unsigned d = 0, e = isl_set_dim(EdgeDom, isl_dim_set);
         d != e && !IsFixedInDim; d++) {
      auto *DimMin = isl_set_dim_min(isl_set_copy(EdgeDom), d);
      auto *DimMax = isl_set_dim_max(isl_set_copy(EdgeDom), d);
      IsFixedInDim = isl_pw_aff_is_equal(DimMin, DimMax);
      isl_pw_aff_free(DimMin);
      isl_pw_aff_free(DimMax);
    }
    isl_set_free(EdgeDom);
    isl_set *EdgeRng = isl_map_range(
        isl_map_intersect_domain(Edge.getDep(), isl_set_copy(RecomputeDomain)));
    DEBUG(errs() << "EdgeRn: " << EdgeRng << "\n");
    for (unsigned d = 0, e = isl_set_dim(EdgeRng, isl_dim_set);
         d != e && !IsFixedInDim; d++) {
      auto *DimMin = isl_set_dim_min(isl_set_copy(EdgeRng), d);
      auto *DimMax = isl_set_dim_max(isl_set_copy(EdgeRng), d);
      IsFixedInDim = isl_pw_aff_is_equal(DimMin, DimMax);
      isl_pw_aff_free(DimMin);
      isl_pw_aff_free(DimMax);
    }
    isl_set *LeftDom = isl_set_intersect_params(
        isl_set_subtract(TrgDom, EdgeRng), getAssumedContext());
    for (unsigned d = 0, e = isl_set_dim(LeftDom, isl_dim_set);
         d != e && !IsFixedInDim; d++) {
      auto *DimMin = isl_set_dim_min(isl_set_copy(LeftDom), d);
      auto *DimMax = isl_set_dim_max(isl_set_copy(LeftDom), d);
      IsFixedInDim = isl_pw_aff_is_equal(DimMin, DimMax);
      isl_pw_aff_free(DimMin);
      isl_pw_aff_free(DimMax);
    }
    DEBUG(errs() << "LeftDom " << LeftDom << " : " << IsFixedInDim << "\n");
    bool NoSplit = isl_set_is_empty(LeftDom);
    isl_set_free(LeftDom);
    if (!NoSplit && IsFixedInDim) {
      Edge.RR = RR_DOMAIN_SPLIT;
      DEBUG(errs() << "\tSKIP edge due to fixed dim split!\n");
      return -1;
    }
  }

  if (HEURISTIC >= 6) {
    isl_set *SrcDom = Edge.Src->getDomain();
    isl_set *EdgeDom = isl_map_domain(Edge.getDep());
    assert(isl_set_is_subset(EdgeDom, SrcDom));
    DEBUG(errs() << "SrcDom: " << SrcDom << "\nEdgeDom: " << EdgeDom << "\n");
    bool FullProp = isl_set_is_equal(SrcDom, EdgeDom);
    isl_set_free(EdgeDom);
    isl_set_free(SrcDom);
    if (FullProp) {
      DEBUG(errs() << "FULL PROP is beneficial!\n");
      return 1;
    }
  }

  if (HEURISTIC >= 8) {
    if (!Edge.Src->IsLiveOut || Edge.Src->IsOverwritten) {
      DEBUG(errs() << "OV/LO heuristic!\n");
      return 1;
    }
  }

  if (HEURISTIC > 0) {
    isl_set *TrgDom = Edge.Trg->getDomain();
    DEBUG(errs() << "TrgDom: " << TrgDom << "\n");
    isl_set *EdgeRng = isl_map_range(
        isl_map_intersect_domain(Edge.getDep(), isl_set_copy(RecomputeDomain)));
    bool IsSingleton = isl_set_is_singleton(EdgeRng);
    DEBUG(errs() << "EdgeRn: " << EdgeRng << "\n");
    isl_set *LeftDom = isl_set_intersect_params(
        isl_set_subtract(TrgDom, EdgeRng), getAssumedContext());
    IsSingleton |= isl_set_is_singleton(LeftDom);
    DEBUG(errs() << "LeftDom " << LeftDom << "\n");
    bool NoSplit = isl_set_is_empty(LeftDom);
    isl_set_free(LeftDom);
    if (!NoSplit && IsSingleton) {
      Edge.RR = RR_DOMAIN_SPLIT;
      DEBUG(errs() << "\tSKIP edge due to singleton split!\n");
      return -1;
    }
  }

  if (HEURISTIC <= 7) {
    isl_set *TrgDom = Edge.Trg->getDomain();
    DEBUG(errs() << "TrgDom: " << TrgDom << "\n");
    isl_set *EdgeRng = isl_map_range(
        isl_map_intersect_domain(Edge.getDep(), isl_set_copy(RecomputeDomain)));
    DEBUG(errs() << "EdgeRn: " << EdgeRng << "\n");
    isl_set *LeftDom = isl_set_intersect_params(
        isl_set_subtract(TrgDom, EdgeRng), getAssumedContext());
    DEBUG(errs() << "LeftDom " << LeftDom << "\n");
    bool NoSplit = isl_set_is_empty(LeftDom);
    isl_set_free(LeftDom);
    if (!NoSplit) {
      Edge.RR = RR_DOMAIN_SPLIT;
      DEBUG(errs() << "\tSKIP edge due to required split!\n");
      return -1;
    }
  }

  return 1;
#endif
}

__isl_give isl_set *
ExpressionPropagation::getRecomputeDomain(RAWEdge &Edge) {

  if (!Edge.Src->hasBarrier())
    return isl_map_domain(Edge.getDep());

  isl_map *Dep = isl_map_intersect_domain(Edge.getDep(), Edge.Src->getDomain());
  DEBUG(errs() << "ED: " << Dep << "\n";);
  isl_map *UseSchedule = Edge.Trg->getSchedule();
  DEBUG(errs() << "US: " << UseSchedule << "\n";);
  UseSchedule = isl_map_intersect_domain(UseSchedule, Edge.Trg->getDomain());
  DEBUG(errs() << "US: " << UseSchedule << "\n";);
  UseSchedule = isl_map_apply_range(Dep, UseSchedule);
  DEBUG(errs() << "US: " << UseSchedule << "\n");

  isl_map *Barrier = Edge.Src->getBarrier();
  DEBUG(errs() << "BR: " << Barrier << "\n");

  if (!Barrier || isl_map_is_empty(Barrier)) {
    isl_map_free(Barrier);
    return isl_map_domain(UseSchedule);
  }

  if (auto *SelfWARDep = Edge.Src->getSelfWARDep()) {
    DEBUG(errs() << "Self Overwrite : " << SelfWARDep << " [LO: " << Edge.Src->isLiveOut() << "]!\n");
    isl_set *PropDom = nullptr;
    PropDom = isl_map_domain(Edge.getDep());
    Edge.Src->applyToEdges<RAWEdge>([&](const RAWEdge &RAW) {
      if (&RAW == &Edge)
        return;
      isl_set *RAWDom = isl_map_domain(RAW.getDep());
      DEBUG(errs() << "RAWDom: " << RAWDom << "\n");
      RAWDom =
          isl_set_intersect(RAWDom, isl_map_range(isl_map_copy(SelfWARDep)));
      DEBUG(errs() << "RAWDom: " << RAWDom << "\n");
      PropDom = isl_set_subtract(PropDom, RAWDom);
    });
    isl_map_free(SelfWARDep);
    DEBUG(errs() << "PropDom: " << PropDom << "!\n");
    //if (Edge.Src->isLiveOut()) {
      auto *OverwrittenSet = Edge.Src->getOverwrittenSet<false>();
      DEBUG(errs() << "OverwrittenSet: " << OverwrittenSet << "\n");
      if (isl_set_is_empty(OverwrittenSet)) {
        isl_set_free(OverwrittenSet);
      } else {
        DEBUG(errs() << "Add MustDeleteNode: " << Edge.Src << "!\n");
        MustDeleteNodes.insert(Edge.Src);
        PropDom = isl_set_intersect(PropDom, OverwrittenSet);
      }
    //}
    DEBUG(errs() << "PropDom: " << PropDom << "!\n");

    UseSchedule = isl_map_intersect_domain(UseSchedule, PropDom);
    PropDom = nullptr;
    DEBUG(errs() << "US: " << UseSchedule << "\n");
    Barrier = isl_map_subtract(Barrier, Edge.Src->getSchedule());
    DEBUG(errs() << "BR: " << Barrier << "\n");
  }

  if (isl_map_is_empty(Barrier) || isl_map_is_empty(UseSchedule)) {
    isl_map_free(Barrier);
    return isl_map_domain(UseSchedule);
  }

  isl_map *WriteReadPairMap =
      isl_map_apply_domain(isl_map_copy(UseSchedule), Barrier);
  DEBUG(errs() << "WRP: " << WriteReadPairMap << "\n");
  isl_map *LTMap =
      isl_map_lex_lt(isl_space_domain(isl_map_get_space(WriteReadPairMap)));
  DEBUG(errs() << "LTM: " << LTMap << "\n");
  WriteReadPairMap = isl_map_intersect(WriteReadPairMap, LTMap);
  DEBUG(errs() << "WRP: " << WriteReadPairMap << "\n");
  auto *BlockedSet = isl_map_range(WriteReadPairMap);
  DEBUG(errs() << "BS: " << BlockedSet << "\n");
  UseSchedule = isl_map_subtract_range(UseSchedule, BlockedSet);
  DEBUG(errs() << "US: " << UseSchedule << "\n");
  return isl_set_coalesce(isl_map_domain(UseSchedule));
}

static void setSplitStmtDom(ScopStmt *SplitStmt) {
  if (SplitStmt->CopyDomain)
    return;
  if (SplitStmt->CopyStmts.empty())
    return;

  isl_set *Domain = SplitStmt->getDomain();
  isl_id *DomainId = isl_set_get_tuple_id(Domain);
  for (ScopStmt *CopyStmt : SplitStmt->CopyStmts) {
    setSplitStmtDom(CopyStmt);
    isl_set *CopyDomain = CopyStmt->getDomain(true);
    CopyDomain = isl_set_set_tuple_id(CopyDomain, isl_id_copy(DomainId));
    Domain = isl_set_union(Domain, CopyDomain);
  }
  isl_id_free(DomainId);
  Domain = isl_set_coalesce(Domain);
  assert(!SplitStmt->CopyDomain);
  SplitStmt->CopyDomain = Domain;
}

void ExpressionPropagation::adjustSchedule(Scop &S) {
  if (SplitStmts.empty())
    return;

  isl_union_map *ContractionUMap = isl_union_map_empty(S.getParamSpace());
  isl_union_set *UnionDomOfSplits = isl_union_set_empty(S.getParamSpace());

  for (DefNode *Def : SplitStmts) {
    DEBUG(errs() << "Adjust schedule of " << Def->getName() << "\n");
    ScopStmt *SplitStmt = Def->getStmt();
    setSplitStmtDom(SplitStmt);

    ScopStmt *TopStmt = SplitStmt;
    while (TopStmt->ParentStmt) {
      assert(TopStmt != TopStmt->ParentStmt);
      TopStmt = TopStmt->ParentStmt;
    }

    isl_space *RngSpace = TopStmt->getDomainSpace();
    assert(RngSpace);
    bool AddedCopyStmt = false;
    for (ScopStmt *CopyStmt : SplitStmt->CopyStmts) {
      auto *CopyDomain = CopyStmt->getDomain();
      if (isl_set_is_empty(CopyDomain)) {
        isl_set_free(CopyDomain);
        continue;
      }
      AddedCopyStmt = true;
      auto *DomSpace = CopyStmt->getDomainSpace();
      auto *ContractionMap =
          isl_map_identity(isl_space_map_from_domain_and_range(
              DomSpace, isl_space_copy(RngSpace)));
      assert(ContractionMap);
      ContractionUMap = isl_union_map_add_map(ContractionUMap, ContractionMap);
      UnionDomOfSplits = isl_union_set_add_set(UnionDomOfSplits, CopyDomain);
    }

    if (!AddedCopyStmt) {
      isl_space_free(RngSpace);
      continue;
    }

    auto *ContractionMap = isl_map_identity(isl_space_map_from_domain_and_range(
        isl_space_copy(RngSpace), isl_space_copy(RngSpace)));
    isl_space_free(RngSpace);
    assert(ContractionMap);
    ContractionUMap = isl_union_map_add_map(ContractionUMap, ContractionMap);
    UnionDomOfSplits =
        isl_union_set_add_set(UnionDomOfSplits, Def->getDomain());
  }

  DEBUG(errs() << "ContractionUMap: "; isl_union_map_dump(ContractionUMap));
  DEBUG(errs() << "UnionDomOfSplits: "; isl_union_set_dump(UnionDomOfSplits));
  assert(ContractionUMap && UnionDomOfSplits);
  auto *Contraction = isl_union_pw_multi_aff_from_union_map(ContractionUMap);
  isl_schedule *UnionSplitSchedule = isl_schedule_from_domain(UnionDomOfSplits);
  isl_schedule *Schedule = S.getScheduleTree();
  DEBUG(errs() << "Contraction: "; isl_union_pw_multi_aff_dump(Contraction);
        errs() << "UnionSplitSchedule: " << UnionSplitSchedule
               << "\nOrigSchedule: ";
        isl_schedule_dump(Schedule));
  Schedule = isl_schedule_expand(Schedule, Contraction, UnionSplitSchedule);
  DEBUG(errs() << "NewSchedule: "; isl_schedule_dump(Schedule));
  DEBUG(errs() << "NewDomain:\n"; auto *ND = isl_schedule_get_domain(Schedule);
        isl_union_set_dump(ND); isl_union_set_free(ND););
  S.setScheduleTree(Schedule);
}

static
isl_stat checkForUnroll(isl_map *Map, void *User) {
  auto &Unroll = *static_cast<bool *>(User);
  //errs() << "Unroll [" << Unroll << "] map: " << Map << "\n";
  isl_set *Range = isl_map_range(Map);
  unsigned NumDims = isl_set_dim(Range, isl_dim_set);
  if (!isSmallerInDim(Range, NumDims - 1, VectorWidth))
    Unroll = false;
  //errs() << "unroll: " << Unroll << "\n";
  return isl_stat_ok;
}
static
isl_stat checkForFullUnroll(isl_map *Map, void *User) {
  auto &Unroll = *static_cast<bool *>(User);
  isl_set *Range = isl_map_range(Map);
  unsigned NumDims = isl_set_dim(Range, isl_dim_set);
  for (unsigned d = 0; Unroll && d < NumDims; d++)
    if (!isSmallerInDim(isl_set_copy(Range), d, VectorWidth))
      Unroll = false;
  isl_set_free(Range);
  return isl_stat_ok;
}

struct MoveInfo {
  bool Move = true;
  SmallVector<ScopStmt *, 8> Stmts;
  int NumCores;
  int LowerBound;
};

static
isl_stat checkForMove(isl_map *Map, void *User) {
  auto &MI = *static_cast<MoveInfo *>(User);
  isl_id *StmtId = isl_map_get_tuple_id(Map, isl_dim_in);
  ScopStmt *Stmt = static_cast<ScopStmt*>(isl_id_get_user(StmtId));
  isl_id_free(StmtId);
  MI.Stmts.push_back(Stmt);
  //if (auto *C = dyn_cast<Constant>(Stmt->WriteMA->getAccessValue()))
    //if (C->isNullValue())
      //MI.Move = false;

  isl_set *Range = isl_map_range(Map);
  if (!isSmallerInDim(isl_set_copy(Range), 0, MI.NumCores) ||
      (isl_set_dim(Range, isl_dim_set) > 2 &&
       isSmallerInDim(isl_set_copy(Range), 1, MI.NumCores))) {
    isl_set_free(Range);
    MI.Move = false;
    return isl_stat_error;
  }
  //int LB = 0;
  //while (!isSmallerInDim(isl_set_copy(Range), 0, LB))
    //LB++;
  isl_set_free(Range);

  //MI.LowerBound = std::max(MI.LowerBound, LB - 1);

  return isl_stat_ok;
}

static __isl_give isl_schedule_node *
unrollInnerMost(__isl_take isl_schedule_node *Node, void *User) {
  if (isl_schedule_node_get_type(Node) == isl_schedule_node_band) {
    isl_schedule_node *P = isl_schedule_node_parent(isl_schedule_node_copy(Node));
    bool PisDom =
        isl_schedule_node_get_type(P) == isl_schedule_node_domain ||
        isl_schedule_node_get_parent_type(P) == isl_schedule_node_domain;
    if (!PisDom){
      isl_schedule_node *PP = isl_schedule_node_parent(isl_schedule_node_copy(P));
      PisDom =
          isl_schedule_node_get_parent_type(PP) == isl_schedule_node_domain;
      isl_schedule_node_free(PP);
    }
    isl_schedule_node_free(P);
    //errs() << "PIsDom " << PisDom << "\n";
    if (PisDom) {
      //isl_schedule_node_dump(Node);
      auto *SchedRelUMap =
          isl_schedule_node_get_subtree_schedule_union_map(Node);
      auto *Domain = isl_schedule_node_get_domain(Node);
      SchedRelUMap = isl_union_map_intersect_domain(SchedRelUMap, Domain);
      MoveInfo MI;
      int NumCores;
      if (PollyNumThreads != 0)
        NumCores = PollyNumThreads;
      else
        NumCores = sys::getHostNumPhysicalCores() * (PollyHyperthreading ? 2 : 1);
      MI.NumCores = NumCores;
      MI.LowerBound = 0;
      isl_union_map_foreach_map(SchedRelUMap, checkForMove, &MI);
      isl_union_map_free(SchedRelUMap);

      if (MI.Move && !MI.Stmts.empty()) {
        Node = isl_schedule_node_band_member_set_ast_loop_type(
            Node, 0, isl_ast_loop_unroll);
        return Node;
        ExpressionPropagation &ExprProp = *static_cast<ExpressionPropagation *>(User);
        SmallPtrSet<DefNodeGroup *, 8> DNGs;
        unsigned Streams = 0;
        for (auto *Stmt : MI.Stmts) {
          auto *Def = ExprProp.DefNodeMap.lookup(Stmt->WriteMA);
          assert(Def);
          auto *DNG = Def->Group;
          if (!DNGs.insert(DNG).second)
            continue;
          assert(ExprProp.FullMergeCostMap.count(DNG));
          Streams += ExprProp.FullMergeCostMap[DNG];
        }
        errs() << " Streams before move: " << Streams << " after: ~" << Streams
               << " * " << MI.LowerBound << " [Treshold: " << NumStreamTreshold
               << "] [" << MI.Stmts.front()->getBaseName() << "]\n";
        if (Streams * MI.LowerBound > NumStreamTreshold) {
          return Node;
        }

        //errs() << "MOVE\n";
        //isl_schedule_node_dump(Node);
//#ifdef UNROLL
        //Node = isl_schedule_node_band_member_set_ast_loop_type(
            //Node, 0, isl_ast_loop_separate);
//#endif
        //errs() << "unroll set\n";
        //isl_schedule_node_dump(Node);
        if (isl_schedule_node_band_n_member(Node) > 1) {
          Node = isl_schedule_node_band_split(Node, 1);
          //isl_schedule_node_dump(Node);
          //errs() << "band split\n";
        }
        Node = isl_schedule_node_band_sink(Node);
        //isl_schedule_node_dump(Node);
        //errs() << "band sinked\n";
#if 0
        if (isl_schedule_node_band_n_member(Node) > 1) {
          Node = isl_schedule_node_band_split(Node, 1);
          isl_schedule_node_dump(Node);
        }
        isl_schedule_node *Child = isl_schedule_node_get_child(Node, 0);
        isl_schedule_node_free(Node);
        Child = isl_schedule_node_band_sink(Child);
        Node = isl_schedule_node_parent(Child);
#endif
        //if (isl_schedule_node_band_member_get_ast_loop_type(Node, 0) != isl_ast_loop_unroll) {
        //isl_schedule_node_dump(Node);
          //return setSeparateOption(Node, 0);
        //}
      }
    }
  }

//#ifdef UNROLL_INNERMOST
  //if (isl_schedule_node_get_type(Node) == isl_schedule_node_band) {
    //auto *SchedRelUMap =
        //isl_schedule_node_get_subtree_schedule_union_map(Node);
    //auto *Domain = isl_schedule_node_get_domain(Node);
    //SchedRelUMap = isl_union_map_intersect_domain(SchedRelUMap, Domain);
    //bool FullUnroll = true;
    //isl_union_map_foreach_map(SchedRelUMap, checkForFullUnroll, &FullUnroll);
    //if (FullUnroll) {
      //auto Space = isl_schedule_node_band_get_space(Node);
      //auto Dims = isl_space_dim(Space, isl_dim_set);
      //isl_space_free(Space);
      //for (unsigned d = 0; d < Dims; d++)
        //Node = isl_schedule_node_band_member_set_ast_loop_type(
            //Node, d, isl_ast_loop_unroll);
    //}
    //isl_union_map_free(SchedRelUMap);
    //if (FullUnroll)
      //return Node;
  //}

  if (isl_schedule_node_get_type(Node) == isl_schedule_node_band) {
    if (isl_schedule_node_n_children(Node) != 1)
      return Node;
    isl_schedule_node *Child = isl_schedule_node_get_child(Node, 0);
    bool ChildIsLeaf = isl_schedule_node_n_children(Child) == 0;
    isl_schedule_node_free(Child);
    if (ChildIsLeaf) {
      auto *SchedRelUMap =
          isl_schedule_node_get_subtree_schedule_union_map(Node);
      auto *Domain = isl_schedule_node_get_domain(Node);
      SchedRelUMap = isl_union_map_intersect_domain(SchedRelUMap, Domain);
      bool Unroll = true;
      isl_union_map_foreach_map(SchedRelUMap, checkForUnroll, &Unroll);
      if (Unroll)  {
        auto Space = isl_schedule_node_band_get_space(Node);
        auto Dims = isl_space_dim(Space, isl_dim_set);
        isl_space_free(Space);
        Node = isl_schedule_node_band_member_set_ast_loop_type(
            Node, Dims - 1, isl_ast_loop_unroll);
      }
      isl_union_map_free(SchedRelUMap);
    }
  }
//#endif
  return Node;
}

static __isl_give isl_schedule_node *
setSeparateOption(__isl_take isl_schedule_node *Node, void *User) {
  if (isl_schedule_node_get_type(Node) == isl_schedule_node_band) {
    auto Space = isl_schedule_node_band_get_space(Node);
    auto Dims = isl_space_dim(Space, isl_dim_set);
    isl_space_free(Space);
    for (unsigned Dim = 0; Dim < Dims; Dim++)
      Node = isl_schedule_node_band_member_set_ast_loop_type(
          Node, Dim, isl_ast_loop_separate);
  }
  return Node;
}

static
isl_stat extractDomains(isl_map *Map, void *User) {
  isl_id *Id = isl_map_get_tuple_id(Map, isl_dim_in);
  ScopStmt *Stmt = static_cast<ScopStmt*>(isl_id_get_user(Id));

  isl_union_set **Domain = static_cast<isl_union_set **>(User);
  *Domain = isl_union_set_add_set(*Domain, Stmt->getDomain());

  isl_map_free(Map);
  isl_id_free(Id);

  return isl_stat_ok;
}

isl_schedule *
ExpressionPropagation::schedule(Scop &S, isl_union_set *Domain,
                                isl_union_map *Dependences,
                                __isl_keep isl_schedule_node *Node) {

  isl_schedule_constraints *ScheduleConstraints;
  ScheduleConstraints = isl_schedule_constraints_on_domain(Domain);
  ScheduleConstraints = isl_schedule_constraints_set_context(
      ScheduleConstraints,
      isl_set_union(S.getAssumedContext(), S.getContext()));
  ScheduleConstraints =
      isl_schedule_constraints_set_validity(ScheduleConstraints, Dependences);
  ScheduleConstraints = isl_schedule_constraints_set_proximity(
      ScheduleConstraints, isl_union_map_copy(MergeDependences));
  //ScheduleConstraints = isl_schedule_constraints_set_coincidence(
      //ScheduleConstraints, isl_union_map_copy(MergeDependences));
  isl_ctx *Ctx = S.getIslCtx();
  auto OnErrorStatus = isl_options_get_on_error(Ctx);
  isl_options_set_on_error(Ctx, ISL_ON_ERROR_CONTINUE);
  isl_schedule *Schedule =
      isl_schedule_constraints_compute_schedule(ScheduleConstraints);
  isl_options_set_on_error(Ctx, OnErrorStatus);
  if (!Schedule) {
    errs() << "No new schedule found!\n";
    Schedule = isl_schedule_node_get_schedule(Node);
  }

  isl_schedule_node *Root = isl_schedule_get_root(Schedule);
  isl_schedule_free(Schedule);
  Root = isl_schedule_node_map_descendant_bottom_up(Root, setSeparateOption,
                                                    this);
  Root = isl_schedule_node_map_descendant_bottom_up(Root, unrollInnerMost,
                                                    this);
  //isl_schedule_node_dump(Root);
  Schedule = isl_schedule_node_get_schedule(Root);
  assert(Schedule);
  isl_schedule_node_free(Root);
  return Schedule;
}

void ExpressionPropagation::schedule(Scop &S) {
  isl_ctx *Ctx = S.getIslCtx();
  isl_options_set_schedule_serialize_sccs(Ctx, 0);
  //isl_options_set_schedule_maximize_band_depth(Ctx, 1);
  isl_options_set_schedule_max_constant_term(Ctx, 200000);
  isl_options_set_schedule_max_coefficient(Ctx, 200000);
  isl_options_set_schedule_whole_component(Ctx, 0);
  isl_options_set_schedule_maximize_coincidence(Ctx, 0);
  isl_options_set_schedule_outer_coincidence(Ctx, 0);
  isl_options_set_schedule_split_scaled(Ctx, 1);
  isl_options_set_schedule_separate_components(Ctx, 0);
  isl_options_set_schedule_treat_coalescing(Ctx, 0);
  isl_options_set_tile_scale_tile_loops(Ctx, 0);
  //isl_options_set_tile_scale_tile_loops(Ctx, 0);

  auto *RAW = isl_union_map_empty(S.getParamSpace());
  auto *WAR = isl_union_map_copy(RAW);
  auto *WAW = isl_union_map_copy(RAW);
  addDependences(S, RAW, WAR, WAW);
  isl_union_map *ValidityDependences = isl_union_map_union(RAW, isl_union_map_union(WAR, WAW));

  if (ValidityDeps)
    ValidityDependences = isl_union_map_union(ValidityDependences,
                                              isl_union_map_copy(ValidityDeps));
  isl_schedule *Schedule = S.getScheduleTree();
  isl_schedule_node *Root = isl_schedule_get_root(Schedule);
  isl_schedule_free(Schedule);

  if (ScheduleAll) {
    isl_union_set *Domain =
        isl_union_set_intersect_params(S.getDomains(), S.getAssumedContext());
    ValidityDependences = isl_union_map_gist_domain(ValidityDependences, isl_union_set_copy(Domain));
    ValidityDependences = isl_union_map_gist_range(ValidityDependences, Domain);
    //(errs() << "VldDependences: " << ValidityDependences << "\n");
    //(errs() << "MrgDependences: " << MergeDependences << "\n");

    auto *NewSchedule = schedule(S, S.getDomains(), ValidityDependences, Root);
    isl_schedule_node_free(Root);
    DEBUG({
      errs() << "NewSchedule [full]:\n";
      isl_schedule_dump(NewSchedule);
    });
    S.setScheduleTree(NewSchedule);
    return;
  }

  assert(isl_schedule_node_n_children(Root) == 1);
  isl_schedule_node *Sequence = isl_schedule_node_child(Root, 0);
  if (isl_schedule_node_get_type(Sequence) != isl_schedule_node_sequence) {
    errs() << "not a sequence node!\n";
    isl_schedule_node_free(Sequence);
    return;
  }

  //errs() << "Sequence:\n";
  //isl_schedule_node_dump(Sequence);

  //isl_union_map *ProximityDependences = isl_union_map_copy(MergeDependences);

  isl_schedule *NewSchedule = nullptr;
  unsigned NumChildren = isl_schedule_node_n_children(Sequence);
  for (unsigned u = 0; u < NumChildren; u++) {
    isl_schedule_node *Node =
        isl_schedule_node_child(isl_schedule_node_copy(Sequence), u);
    errs() << "Schedule node [" << u << "]:\n";
    isl_schedule_node_dump(Node);

    isl_union_map *UnionMap =
        isl_schedule_node_get_subtree_schedule_union_map(Node);

    DEBUG(errs() << "UnionMap: " << UnionMap << "\n");
    isl_union_set *Domain = isl_union_set_empty(S.getParamSpace());
    isl_union_map_foreach_map(UnionMap, extractDomains, &Domain);
    Domain = isl_union_set_intersect_params(Domain, S.getAssumedContext());
    DEBUG(errs() << "Domain: " << Domain << "\n");
    isl_union_map_free(UnionMap);

    isl_union_map *Dependences = isl_union_map_copy(ValidityDependences);
    //Dependences =
        //isl_union_map_intersect_domain(Dependences, isl_union_set_copy(Domain));
    //Dependences =
        //isl_union_map_intersect_range(Dependences, isl_union_set_copy(Domain));
    Dependences = isl_union_map_gist_domain(Dependences, isl_union_set_copy(Domain));
    Dependences = isl_union_map_gist_range(Dependences, isl_union_set_copy(Domain));
    DEBUG(errs() << "ValDependences: " << Dependences << "\n");

    isl_schedule *Schedule = schedule(S, Domain, Dependences, Node);
    isl_schedule_node_free(Node);
    if (NewSchedule)
      NewSchedule = isl_schedule_sequence(NewSchedule, Schedule);
    else
      NewSchedule = Schedule;
  }
  isl_union_map_free(ValidityDependences);
  isl_schedule_node_free(Sequence);

  DEBUG({
    errs() << "NewSchedule:\n";
    isl_schedule_dump(NewSchedule);
  });
  S.setScheduleTree(NewSchedule);
}

bool ExpressionPropagation::setDomain(DefNode &Def,
                                      __isl_take isl_set *NewDomain) {
  DEBUG(errs() << "\nBefore set domain of: " << NewDomain << "\n";
        Def.print(errs()));

  if (!Def.setDomain(NewDomain))
    return false;

  isl_set *Domain = Def.getDomain();

  Def.applyToAndCheckEdges<WAREdge>([&](WAREdge &Edge) {
    Edge.intersectRange(isl_set_copy(Domain));
    return !Edge.isEmpty();
  });

  for (auto *Op : Def.Operands) {
    Op->applyToAndCheckEdges<WAREdge>([=](WAREdge &Edge) {
      Edge.intersectDomain(isl_set_copy(Domain));
      return !Edge.isEmpty();
    });
  }

  Def.applyToAndCheckEdges<WAWEdge>([&](WAWEdge &Edge) {
    if (Edge.Src == &Def) {
      Edge.intersectDomain(isl_set_copy(Domain));
    }
    if (Edge.Trg == &Def) {
      Edge.intersectRange(isl_set_copy(Domain));
    }
    assert(Edge.Src == &Def || Edge.Trg == &Def);
    return !Edge.isEmpty();
  });

  Def.applyToAndCheckEdges<RAWEdge>([&](RAWEdge &Edge) {
    Edge.intersectDomain(isl_set_copy(Domain));
    return !Edge.isEmpty();
  });

  for (auto *Op : Def.Operands) {
    Op->applyToAndCheckEdges<RAWEdge>([&](RAWEdge &Edge) {
      Edge.intersectRange(isl_set_copy(Domain));
      return !Edge.isEmpty();
    });
  }

  DEBUG(errs() << "\nAfter set domain of: " << Domain << "\n";
        Def.print(errs()));
  isl_set_free(Domain);
  //Def.getStmt()->restrictDomain(Domain);
  return true;
}

bool ExpressionPropagation::restrictStmt(DefNode &Def, bool Force) {
  // TODO:
  bool Changed = false;
  static int RESTRICT = 0;
  if (MAX_RESTRICT >= 0 && RESTRICT >= MAX_RESTRICT)
    return Changed;

  if (!Def.MA->isAffine())
    return Changed;

  Force |= MustDeleteNodes.count(&Def);
  DEBUG(errs() << "Restrict " << Def.getName() << " [Force: " << Force << "] [Def: " << &Def << "]\n");
  if (!Force && (Def.isLiveOut() || !Def.getEdges<RAWEdge>().empty()))
    return Changed;

  isl_set *Domain = Def.getDomain();
  isl_set *UsedDomain = Def.getUsedDomain(Force);
  if (!UsedDomain) {
    DEBUG(errs() << "Used domain too complex!\n");
    isl_set_free(Domain);
    isl_set_free(UsedDomain);
    return Changed;
  }

  if (Def.isLiveOut()) {
    isl_set *OverwrittenDomain = Def.getOverwrittenSet<false>();
    OverwrittenDomain = isl_set_subtract(OverwrittenDomain, UsedDomain);

    DEBUG(errs() << "Def: " << Def.getName() << "\n"
                 << "\tDomain: " << Domain
                 << "\n\tOverwrittenDomain: " << OverwrittenDomain << "\n");

    if (!Force && isl_set_n_basic_set(OverwrittenDomain) > 4) {
      DEBUG(errs() << "Overwritten domain too complex!\n");
      isl_set_free(Domain);
      isl_set_free(OverwrittenDomain);
      return Changed;
    }

    UsedDomain = isl_set_subtract(Domain, OverwrittenDomain);
  } else {
    isl_set_free(Domain);
  }

  isl_set *OldDomain = Def.getDomain();
  auto OldDomainComplexity = isl_set_n_basic_set(OldDomain);
  isl_set_free(OldDomain);

  //if (OldDomainComplexity < isl_set_n_basic_set(UsedDomain) &&
      //!Force) {
    //DEBUG(errs() << "Skip delete dead iterations for " << Def.getName()
                  //<< "\n");
    //isl_set_free(UsedDomain);
    //return false;
  //} else {
    DEBUG(errs() << "Delete dead iterations for " << Def.getName() << "\n");
    Changed = setDomain(Def, UsedDomain);
  //}

  DEBUG(errs() << "Restrict stmt [" << Def.getName()
               << "] domain finished, changed: " << Changed << "\n");
  RESTRICT += Changed;
  DEBUG({
    errs() << "Verify def after restrict\n";
    Def.verify();
  });
  return Changed;
}

void ExpressionPropagation::simplifyAccesses(Scop &S) {
  #if 0
  for (ScopStmt &Stmt : S) {
    for (MemoryAccess *MA : Stmt) {
      if (!MA->isArrayKind() || !MA->isAffine())
        continue;
      isl_map *AccRel = MA->getAccessRelation();
      bool IsInjective = isl_map_is_injective(AccRel);
      isl_map_free(AccRel);
      if (IsInjective)
        continue;
      errs() << "MA is not injective:\n";
      MA->dump();

      //auto &Subscripts = MA->getSubscripts();
      //for (unsigned u = 0; u < Subscripts.size(); u++) {
        //const SCEV *Subscript = Subscripts[u];
        //SetVector<Value *> Values;
        //findValues(Subscript, *S.getSE(), Values);
        //if (Values.empty())
      //}
    }
  }
  #endif
}

void ExpressionPropagation::mergeSimpleNodes(Scop &S) {
  for (const auto &DefNodeGroupIt : DefNodeGroupMap) {
    DenseMap<Constant *, SmallPtrSet<DefNode *, 8>> ConstantStoreMap;
    DefNodeGroup *DNG = DefNodeGroupIt.second;
    DEBUG(errs() << "Check " << DNG->SAI->getName() << " for merge opportunities\n");
    for (DefNode *Node : DNG->Nodes) {
      if (Node->isEmpty())
        continue;
      auto *SI = dyn_cast<StoreInst>(Node->MA->getAccessInstruction());
      if (!SI)
        continue;
      auto *CI = dyn_cast<Constant>(SI->getValueOperand());
      if (!CI)
        continue;
      DEBUG(errs() << "Found constant store: " << *SI << " in " << Node->getName()
              << "\n[#RAW: " << Node->getEdges<RAWEdge>().size() << "]"
              << " [#WAW: " << Node->getEdges<WAWEdge>().size() << "]");
      if (Node->getEdges<RAWEdge>().empty())
        continue;
      if (!Node->getEdges<WAWEdge>().empty())
        continue;
      ConstantStoreMap[CI].insert(Node);
    }

    for (auto &CSMapIt : ConstantStoreMap) {
      Constant *CI = CSMapIt.first;
      auto &ConstantStores = CSMapIt.second;
      errs() << "Found " << ConstantStores.size() << " stores of " << *CI
             << "\n";
      if (ConstantStores.size() < 2)
        continue;

      auto CSIt = ConstantStores.begin();
      auto CSEnd = ConstantStores.end();
      DefNode *SummaryNode = (*CSIt);
      isl_id *SummaryNodeId = SummaryNode->getStmt()->getDomainId();
      {
        errs() << "SummaryNode before merge:\n";
        SummaryNode->print(errs(), 4);
      }
      while (++CSIt != CSEnd) {
        DefNode *Node = (*CSIt);
        {
          errs() << "  MergedNode:\n";
          Node->print(errs(), 8);
        }
        isl_set *NodeDomain = Node->getDomain();
        isl_id *NodeId = isl_set_get_tuple_id(NodeDomain);
        isl_set *EmpyNodeDomain = isl_set_empty(isl_set_get_space(NodeDomain));
        isl_set *NewDomain =
            isl_set_set_tuple_id(NodeDomain, isl_id_copy(SummaryNodeId));
        Node->copyEdgesFrom<RAWEdge>(*SummaryNode, NewDomain, NodeId);
        isl_id_free(NodeId);
        SummaryNode->addDomain(NewDomain);
        Node->setDomain(EmpyNodeDomain);
        Node->applyToAndCheckEdges<RAWEdge>(
            [&](RAWEdge &RAW) { return false; });
      }
      isl_id_free(SummaryNodeId);
      {
        errs() << "SummaryNode after merge:\n";
        SummaryNode->print(errs(), 4);
      }
    }
  }
}

bool ExpressionPropagation::propagateExpressions(Scop &S) {
  bool Changed = false;

  DEBUG(errs() << "Initial Worklist: " << Worklist.size() << "\n");
  while (!Worklist.empty()) {
    DEBUG(errs() << "Worklist: " << Worklist.size() << "\n");
    DEBUG_WITH_TYPE("polly-exp-light", errs() << "Worklist: " << Worklist.size()
                                              << "\n");
    DefNodeGroup *DNG = Worklist.top();
    Worklist.pop();

    auto Potential = DNG->getNumArrayLoweringPotential();
    (errs() << "\n------!!CHECK!!: " << DNG->SAI->getName()
                 << " [#N: " << DNG->Nodes.size()
                 << "] [#E: " << DNG->numOutgoingEdges<RAWEdge>()
                 << "] [Pot: " << Potential << "]\n");
    DEBUG_WITH_TYPE("polly-exp-light",
                    errs() << "\n------!!CHECK!!: " << DNG->SAI->getName()
                           << " [#N: " << DNG->Nodes.size()
                           << "] [#E: " << DNG->numOutgoingEdges<RAWEdge>()
                           << "] [Pot: " << Potential << "]\n");

    bool Success = WorklistSet.erase(DNG);
    assert(Success);

    //if (Potential < 0) {
      //DEBUG(errs() << "DNG has negative potential! Skip!\n");
      //continue;
    //}

    if (!DNG->IsAffine) {
      (errs() << "DNG not affine! Skip!\n");
      continue;
    }

    if (DNG->isEmpty()) {
      (errs() << "DNG is empty! Skip!\n");
      continue;
    }

    DEBUG({
      errs() << "Verify DNG before prop!\n";
      DNG->verify();
    });

    bool Prop = propagateEdges(*DNG);
#if 0
    for (unsigned u = 0; u < DNG->Nodes.size(); u++) {
      auto *Def = DNG->Nodes[u];
      if (Def->isLiveOut() && !Def->IsOverwritten)
        continue;
      if (Def->getEdges<RAWEdge>().empty())
        continue;
      errs() << " Try propagation for\n"; Def->print(errs(), 4);;
      DefNodeGroup SingleDNG(DNG->SAI, *Def, *TLI);
      SingleDNG.IsLiveOut = false;
      assert(Def->Group == &SingleDNG);
      assert(!Def->isLiveOut());
      Prop |= propagateEdges(SingleDNG);
      Def->Group = DNG;
    }
#endif
    if (!Prop)
      continue;

    Changed = true;
    {
      Worklist.reheapify();
    }

    DEBUG({
      errs() << "Verify DNG after prop!\n";
      DNG->verify();
    });
  }

  return Changed;
}

struct MemOrderInfo {
  isl_map *MemOrderDep;
  unsigned FixedSrcDims;
  unsigned FixedTrgDims;
};

isl_stat filterMemOrderDeps(isl_basic_map *BM, void *User) {
  unsigned NumFixedSrc = 0;
  unsigned NumDimSrc = isl_basic_map_dim(BM, isl_dim_in);
  for (unsigned d = 0; d < NumDimSrc; d++) {
    isl_val *V = isl_basic_map_plain_get_val_if_fixed(BM, isl_dim_in, d);
    NumFixedSrc += !V || isl_val_is_nan(V);
    isl_val_free(V);
  }

  unsigned NumFixedTrg =0;
  unsigned NumDimTrg = isl_basic_map_dim(BM, isl_dim_out);
  for (unsigned d = 0; d < NumDimSrc; d++) {
    isl_val *V = isl_basic_map_plain_get_val_if_fixed(BM, isl_dim_out, d);
    NumFixedTrg += !V || isl_val_is_nan(V);
    isl_val_free(V);
  }

  MemOrderInfo &MOI = *static_cast<MemOrderInfo *>(User);
  if (NumFixedSrc < MOI.FixedSrcDims || NumFixedTrg < MOI.FixedTrgDims) {
    isl_basic_map_dump(BM);
    //errs() << "NumFixedSrc: " << NumFixedSrc << " vs " << MOI.FixedSrcDims << "\n";
    //errs() << "NumFixedTrg: " << NumFixedTrg << " vs " << MOI.FixedTrgDims << "\n";
    isl_basic_map_free(BM);
    return isl_stat_ok;
  }

  MOI.MemOrderDep = isl_map_union(MOI.MemOrderDep, isl_map_from_basic_map(BM));
  return isl_stat_ok;
}

void ExpressionPropagation::setMemOrderDeps2(DefNodeGroup *DNG,
                                            ArrayRef<DefNode *> Nodes) {

  //unsigned FixedDims[Nodes.size()];
  //for (unsigned u = 0; u < Nodes.size(); u++) {
    //FixedDims[u] = 0;
    //auto *Def = Nodes[u];
    //isl_set *Dom = Def->getDomain();
    //unsigned NumDims = isl_set_dim(Dom, isl_dim_set);
    //for (unsigned d = 0; d < NumDims; d++) {
      //isl_val *V = isl_set_plain_get_val_if_fixed(Dom, isl_dim_set, d);
      //FixedDims[u] += !V ||  isl_val_is_nan(V);
      //isl_val_free(V);
    //}
    //isl_set_free(Dom);
  //}

  unsigned NumMemDims = DNG->SAI->getNumberOfDimensions();
  isl_space *Space = DNG->SAI->getSpace();
  isl_map *MemOrder = isl_map_lex_lt(Space);
  for (unsigned u0 = 0; u0 < Nodes.size(); u0++) {
    isl_map *AccRel0 = Nodes[u0]->MA->getAccessRelation();
    AccRel0 =
        isl_map_intersect_domain(AccRel0, Nodes[u0]->getStmt()->getDomain());
    //errs() << "AR0: " << AccRel0 << "\n";
    isl_set *AccRelRng0 = isl_map_range(AccRel0);
    AccRelRng0 = isl_set_neg(isl_set_lexmin(AccRelRng0));
    isl_map *DiffMap0 = isl_map_from_domain_and_range(
        isl_set_universe(Nodes[u0]->getStmt()->getDomainSpace()), AccRelRng0);
    AccRel0 = Nodes[u0]->MA->getAccessRelation();
    AccRel0 = isl_map_sum(AccRel0, DiffMap0);

    isl_map *MemOrderS0 = isl_map_copy(MemOrder);
    MemOrderS0 = isl_map_apply_range(AccRel0, MemOrderS0);
    //errs() << "MOD: " << MemOrderS0 << "\n";
    for (unsigned u1 = 0; u1 < Nodes.size(); u1++) {
      //if (Nodes[u0]->getStmt()->getBasicBlock() == Nodes[u1]->getStmt()->getBasicBlock())
        //continue;
      isl_map *AccRel1 = Nodes[u1]->MA->getAccessRelation();
      AccRel1 =
          isl_map_intersect_domain(AccRel1, Nodes[u1]->getStmt()->getDomain());
      isl_set *AccRelRng1 = isl_map_range(AccRel1);
      AccRelRng1 = isl_set_neg(isl_set_lexmin(AccRelRng1));
      isl_map *DiffMap1 = isl_map_from_domain_and_range(
          isl_set_universe(Nodes[u1]->getStmt()->getDomainSpace()), AccRelRng1);
      AccRel1 = Nodes[u1]->MA->getAccessRelation();
      AccRel1 = isl_map_sum(AccRel1, DiffMap1);

      isl_map* MemOrderS0S1 = isl_map_copy(MemOrderS0);
      isl_map *MemOrderDep =
          isl_map_apply_range(MemOrderS0S1, isl_map_reverse(AccRel1));
      //errs() << "     Memdep: " << MemOrderDep << "\n";
      //isl_map *SimpleMemOrderDep = isl_map_empty(isl_map_get_space(MemOrderDep));
      //MemOrderInfo MOI = { SimpleMemOrderDep, FixedDims[u0], FixedDims[u1] };
      //isl_map_foreach_basic_map(MemOrderDep, filterMemOrderDeps, &MOI);
      //isl_map_free(MemOrderDep);
      //MemOrderDep = MOI.MemOrderDep;
      MemOrderDep =
          isl_map_gist_domain(MemOrderDep, Nodes[u0]->getStmt()->getDomain());
      MemOrderDep =
          isl_map_gist_range(MemOrderDep, Nodes[u1]->getStmt()->getDomain());
      //errs() << "Filterdep: " << MemOrderDep << "\n";
      assert(ValidityDeps);
      //ValidityDeps = isl_union_map_add_map(ValidityDeps, MemOrderDep);
      MergeDependences = isl_union_map_add_map(MergeDependences, MemOrderDep);
    }
    isl_map_free(MemOrderS0);
  }
  isl_map_free(MemOrder);
}
void ExpressionPropagation::setMemOrderDeps(DefNodeGroup *DNG,
                                            ArrayRef<DefNode *> Nodes) {
  unsigned NumNodes = Nodes.size();
  for (unsigned u1 = 0; u1 < NumNodes; u1++) {
    auto *Def = Nodes[u1];
    auto *Stmt = Def->getStmt();
    auto *StmtOrderMap = isl_map_lex_lt(Stmt->getDomainSpace());
    //errs() << "StmtOrderMap " << StmtOrderMap << "\n";
    ValidityDeps = isl_union_map_add_map(ValidityDeps, StmtOrderMap);
    isl_set *Min0O =
        isl_set_neg(isl_set_reset_tuple_id(isl_set_lexmin(Stmt->getDomain())));
    for (unsigned u2 = 0; u2 < NumNodes; u2++) {
      if (u1 == u2)
        continue;
      auto *NextStmt = Nodes[u2]->getStmt();
      //unsigned MinDim =
          //std::min(Stmt->getNumIterators(), NextStmt->getNumIterators());
      //if (MinDim == 0)
        //continue;
      auto *GroupOrderMap = isl_map_lex_lt(Stmt->getDomainSpace());
          //isl_map_universe(isl_space_map_from_set(Stmt->getDomainSpace()));
      GroupOrderMap = isl_map_set_tuple_id(GroupOrderMap, isl_dim_out,
                                           NextStmt->getDomainId());
      //for (unsigned d = 0; d+1 < MinDim ; d++)
        //GroupOrderMap =
            //isl_map_equate(GroupOrderMap, isl_dim_in, d, isl_dim_out, d);
      //GroupOrderMap = isl_map_order_lt(GroupOrderMap, isl_dim_in, MinDim - 1,
                                       //isl_dim_out, MinDim - 1);
      isl_set *Min1 =
          isl_set_reset_tuple_id(isl_set_lexmin(NextStmt->getDomain()));
      isl_set *Min0 = isl_set_copy(Min0O);
      int Dim0 = isl_set_dim(Min0, isl_dim_set);
      int Dim1 = isl_set_dim(Min1, isl_dim_set);
      int DimDiff = Dim1 - Dim0;
      if (DimDiff < 0) {
        Min1 = isl_set_add_dims(Min1, isl_dim_set, -DimDiff);
        for (int d = 0; d < -DimDiff; d++)
          Min1 = isl_set_fix_dim_si(Min1, Dim1 + d, 0);
      } else if (DimDiff > 0) {
        Min1 = isl_set_project_out(Min1, isl_dim_set, Dim0, DimDiff);
      }
      //errs() << Min0 << " : " << Min1 << "\n";
      isl_set *Diff = isl_set_sum(Min1, Min0);
      //errs() << "Diff: " << Diff << "\n";
      Diff = isl_set_set_tuple_id(Diff, NextStmt->getDomainId());
      isl_map *Offset = isl_map_from_domain_and_range(
          isl_set_universe(Stmt->getDomainSpace()), Diff);
      //errs() << "Offset: " << Offset << "\n";
      //errs() << "GroupOrderMap " << GroupOrderMap << "\n";
      GroupOrderMap = isl_map_sum(GroupOrderMap, Offset);
      //errs() << "GroupOrderMap " << GroupOrderMap << "\n";
      ValidityDeps = isl_union_map_add_map(ValidityDeps, GroupOrderMap);
    }
    isl_set_free(Min0O);
  }
}

void ExpressionPropagation::tryFullMerge(DefNodeGroup *DNG,
                                         SmallVectorImpl<DefNode *> &Nodes) {
  if (Nodes.empty())
    return;

  isl_map *MemOrder = isl_map_lex_lt(DNG->SAI->getSpace());
  unsigned NumMemDims = DNG->SAI->getNumberOfDimensions();
  for (unsigned u0 = 0; u0 < Nodes.size(); u0++) {
    auto *Def0 = Nodes[u0];
    auto *Stmt0 = Def0->getStmt();
    isl_set *Dom0 = Stmt0->getDomain();
    isl_map *AccRel0 = Def0->MA->getAccessRelation();
    isl_map *DomAccRel0 = isl_map_copy(AccRel0);
    DomAccRel0 = isl_map_intersect_domain(DomAccRel0, Dom0);
    //errs() << "DomAccRel0 " << DomAccRel0 << "\n";
    isl_set *DomRel0 = isl_map_range(DomAccRel0);
    DomRel0 = isl_set_lexmin(DomRel0);
    // errs() << "DomRel0 " << DomRel0 << "\n";
    isl_map *DepMap0 = isl_map_apply_range(AccRel0, isl_map_copy(MemOrder));
    // errs() << "DepMap0 " << DepMap0 << "\n";

    unsigned u1 = (u0 + 1) % Nodes.size();
    auto *Def1 = Nodes[u1];
    auto *Stmt1 = Def1->getStmt();
    auto *Dom1 = Stmt1->getDomain();

    isl_map *AccRel1 = Def1->MA->getAccessRelation();
    // errs() << "AccRel1: " << AccRel1 << "\n";
    isl_map *DomAccRel1 = isl_map_copy(AccRel1);
    DomAccRel1 = isl_map_intersect_domain(DomAccRel1, Dom1);
    // errs() << "DomAccRel1 " << DomAccRel1 << "\n";
    isl_set *DomRel1 = isl_map_range(DomAccRel1);
    DomRel1 = isl_set_lexmin(DomRel1);
    // errs() << "DomRel1 " << DomRel1 << "\n";

    isl_set *Diff = isl_set_sum(isl_set_neg(DomRel1), isl_set_copy(DomRel0));
    // errs() << "Diff: " << Diff << "\n";

    isl_map *Offset = isl_map_from_domain_and_range(
        isl_set_universe(Stmt1->getDomainSpace()), Diff);
    // errs() << "Offs: " << Offset << "\n";

    isl_map *AdjustedAccRel1 = isl_map_sum(AccRel1, Offset);
    // errs() << "AdjAccRel1: " << AdjustedAccRel1 << "\n";

    isl_map *DepMap01 = isl_map_copy(DepMap0);
    DepMap01 =
        isl_map_apply_range(DepMap01, isl_map_reverse(AdjustedAccRel1));
    // errs() << "DepMap01 " << DepMap01 << "\n\n";

    //errs() << "CopyDep " << DepMap01 <<"\n";
    //ValidityDeps =
        //isl_union_map_add_map(ValidityDeps, isl_map_copy(DepMap01));
    ValidityDeps =
        isl_union_map_add_map(ValidityDeps, isl_map_copy(DepMap01));

    isl_map_free(DepMap01);
    // errs() << "DepMap01 [" << u0 << "] " << DepMap01 << "\n";
    // DependenceChain = DependenceChain
    //? isl_map_apply_range(DependenceChain, DepMap01)
    //: DepMap01;
    // errs() << "DependenceChain [" << u0 << "] " << DependenceChain << "\n";
    //}
    isl_set_free(DomRel0);
    isl_map_free(DepMap0);
  }

  isl_map_free(MemOrder);
}

void ExpressionPropagation::mergeDefNodeGroups(Scop &S) {
  assert(!MergeDependences);
  MergeDependences = isl_union_map_empty(S.getParamSpace());
  ValidityDeps = isl_union_map_empty(S.getParamSpace());

  for (const auto &DefNodeGroupIt : DefNodeGroupMap) {
    DefNodeGroup *DNG = DefNodeGroupIt.second;

    SmallVector<isl_map *, 1> NodeDeps;
    SmallPtrSet<MemoryAccess *, 1> ExcludedMAs;
    auto NumAccessStreams =
        computeNumAccessedStreams(DNG->Nodes, NodeDeps, ExcludedMAs, -1);
    DEBUG(errs() << "Full merge of the " << DNG->Nodes.size() << " nodes will require "
            << NumAccessStreams << " [SAI: " << DNG->SAI->getName()
            << "][Treshold: " << NumStreamTreshold << "]\n");
    FullMergeCostMap[DNG] = NumAccessStreams;

    if (!DNG->IsAffine)
      continue;

    bool HasDNGDeps = false;
    for (DefNode *Def : DNG->Nodes) {
      if (Def->isEmpty())
        continue;
      Def->applyToEdges<RAWEdge>([&](const RAWEdge &Edge) {
        if (Edge.Trg->getDefNode().MA->getScopArrayInfo() == DNG->SAI)
          HasDNGDeps = true;
      });
      Def->applyToEdges<WAREdge>([&](const WAREdge &Edge) {
        if (Edge.Trg->getDefNode().MA->getScopArrayInfo() == DNG->SAI)
          HasDNGDeps = true;
      });
      Def->applyToEdges<WAWEdge>([&](const WAWEdge &Edge) {
        if (Edge.Trg->getDefNode().MA->getScopArrayInfo() == DNG->SAI)
          HasDNGDeps = true;
      });
    }

    SmallVector<DefNode *, 8> Nodes;
    if (!HasDNGDeps) {
      for (DefNode *Def : DNG->Nodes)
        if (!Def->isEmpty())
          Nodes.push_back(Def);
      setMemOrderDeps(DNG, Nodes);
      //setMemOrderDeps2(DNG, Nodes);
    }

    #if 0
    for (DefNode *Def : DNG->Nodes) {
      if (Def->getStmt()->ParentStmt)
        continue;

      Nodes.clear();
      if (!Def->isEmpty())
        Nodes.push_back(Def);

      SmallVector<DefNode *, 8> CopyNodes;
      CopyNodes.push_back(Def);
      for (unsigned u = 0; u < CopyNodes.size(); u++) {
        auto &CopyStmts = CopyNodes[u]->getStmt()->CopyStmts;
        for (auto *CopyStmt : CopyStmts) {
          auto *CopyNode = DefNodeMap.lookup(CopyStmt->WriteMA);
          if (!CopyNode->isEmpty())
            Nodes.push_back(CopyNode);
          CopyNodes.push_back(CopyNode);
        }
      }
      tryFullMerge(DNG, Nodes);
    }
    #endif
  }

  SetVector<const ScopArrayInfo *> SeenSAIs;
  const ScopArrayInfo *LastSAI = nullptr;
  ScopStmt *LastStmt = nullptr;
  for (ScopStmt &Stmt : S) {
    if (Stmt.ParentStmt)
      break;
    SmallVector<ScopStmt *, 8> CopyStmts;
    CopyStmts.push_back(&Stmt);
    for (unsigned u = 0; u < CopyStmts.size(); u++) {
      ScopStmt &CurStmt = *CopyStmts[u];
      for (auto *CopyStmt : CurStmt.CopyStmts)
        CopyStmts.push_back(CopyStmt);
      if (!CurStmt.isExecuted())
        continue;
      const ScopArrayInfo *SAI = CurStmt.WriteMA->getScopArrayInfo();
      if (!LastSAI || SAI == LastSAI) {
        if (!LastSAI)
          SeenSAIs.insert(SAI);
        LastSAI = SAI;
        LastStmt = &CurStmt;
        continue;
      }

      if (!SeenSAIs.insert(SAI)) {
        DEBUG(errs() << "SAI " << LastSAI->getName() << " accessed interleaved with "
                << SAI->getName() << "\nLastStmt: " << LastStmt->getBaseName()
                << " CurStmt: " << CurStmt.getBaseName() << "\n");
        return;
      }
      LastSAI = SAI;
      LastStmt = &CurStmt;
    }
  }

  #if 0
  isl_union_set *LastDomains = isl_union_set_empty(S.getParamSpace());
  LastSAI = nullptr;
  for (const ScopArrayInfo *SAI : SeenSAIs) {
    DefNodeGroup *DNG = DefNodeGroupMap[SAI];
    if (DNG->isEmpty()) {
      errs() << "Skip empty DNG for " << SAI->getName() << "\n";
      continue;
    }
    isl_union_set *Domains = isl_union_set_empty(S.getParamSpace());
    for (DefNode *Def : DNG->Nodes) {
      if (Def->isEmpty()) {
        errs() << "Skip empty def " << Def->getName() << " for " << SAI->getName() << "\n";
        continue;
      }
      Domains = isl_union_set_add_set(
          Domains, isl_set_universe(Def->getStmt()->getDomainSpace()));
    }
    if (!isl_union_set_is_empty(LastDomains)) {
      errs() << "Order accs from " << LastSAI->getName() << " to "
             << SAI->getName() << "\n";
      isl_union_map *OrderMap = isl_union_map_from_domain_and_range(
          LastDomains, isl_union_set_copy(Domains));
      (errs() << "OrderMap: " << OrderMap << "\n");
      MergeDependences = isl_union_map_union(MergeDependences, OrderMap);
    } else {
      isl_union_set_free(LastDomains);
    }
    LastDomains = Domains;
    LastSAI = SAI;
  }
  isl_union_set_free(LastDomains);
  #endif
  Ordered = true;

}

void ExpressionPropagation::deleteDeadIterations() {

  for (const auto &It : DefNodeMap) {
    DefNode *Def = It.getSecond();
    assert(Def);

    isl_set *OldDomain = Def->getStmt()->getDomain();
    auto OldDomainComplexity = isl_set_n_basic_set(OldDomain);
    isl_set_free(OldDomain);

    bool MustDelete = MustDeleteNodes.count(Def);
    restrictStmt(*Def, MustDelete);
    ScopStmt *Stmt = Def->getStmt();
    isl_set *Domain = Def->getDomain();

    //if (OldDomainComplexity < isl_set_n_basic_set(Domain) && !MustDelete) {
      //DEBUG(errs() << "Skip delete dead iterations for " << Def->getName()
                    //<< "\n");
      //isl_set_free(Domain);
    //} else {
      STATEMENTS_REMOVED += isl_set_is_empty(Domain);
      bool Restricted = Stmt->restrictDomain(Domain);
      (void)Restricted;
      RESTRICTED_STATEMENTS += Restricted;
    //}
  }

}

void ExpressionPropagation::addDependences(Scop &S, isl_union_map *&RAW,
                                           isl_union_map *&WAR,
                                           isl_union_map *&WAW) {
  for (const auto &It : DefNodeMap) {
    const DefNode *Def = It.getSecond();
    assert(Def);
    Def->applyToEdges<RAWEdge>([&](const RAWEdge &Edge) {
      assert(Edge);
      if (Ordered && !ScheduleAll &&
          Edge.Src->MA->getScopArrayInfo() !=
              Edge.Trg->getDefNode().MA->getScopArrayInfo())
        return;
      RAW = isl_union_map_add_map(RAW, Edge.getDep());
    });
    Def->applyToEdges<WAREdge>([&](const WAREdge &Edge) {
      if (Ordered && !ScheduleAll &&
          Edge.Src->MA->getScopArrayInfo() !=
              Edge.Trg->getDefNode().MA->getScopArrayInfo())
        return;
      auto *Dep = Edge.getDep();
      if (&Edge.Src->getDefNode() == Edge.Trg && isl_map_is_identity(Dep)) {
        // remove self write idenitiy dependences
        isl_map_free(Dep);
        return;
      }
      WAR = isl_union_map_add_map(WAR, Dep);
    });
    Def->applyToEdges<WAWEdge>([&](const WAWEdge &Edge) {
      if (Ordered && !ScheduleAll &&
          Edge.Src->MA->getScopArrayInfo() !=
              Edge.Trg->getDefNode().MA->getScopArrayInfo())
        return;
      //if (!Edge.Src->isLiveOut())
        //return;
      WAW = isl_union_map_add_map(WAW, Edge.getDep());
    });
  }

}

void ExpressionPropagation::setNewDependences(Scop &S) {
  auto &IslCtx = S.getSharedIslCtx();
  isl_union_map *RAW = nullptr;
  isl_union_map *WAR = nullptr;
  isl_union_map *WAW = nullptr;
  if (true) {
    RAW = isl_union_map_empty(S.getParamSpace());
    WAR = isl_union_map_copy(RAW);
    WAW = isl_union_map_copy(RAW);
    addDependences(S, RAW, WAR, WAW);
  } else {
    auto &CD = DI->recomputeDependences(Dependences::AL_Statement);
    RAW = CD.getDependences(Dependences::TYPE_RAW);
    WAR = CD.getDependences(Dependences::TYPE_WAR);
    WAW = CD.getDependences(Dependences::TYPE_WAW);
  }

  if (MergeDependences) {
    //MergeDependences = isl_union_map_intersect_params(MergeDependences, getAssumedContext());
    //(errs() << "Merge dependences: " << MergeDependences << "\n");
    //RAW = isl_union_map_coalesce(isl_union_map_union(RAW, MergeDependences));
    isl_union_map_free(MergeDependences);
    MergeDependences = nullptr;
  }

  DEBUG(errs() << "Set New Dependences [Ordered: " << Ordered << "]\nRAW: "
               << RAW << "\nWAR: " << WAR << "\nWAW: " << WAW << "\n");
  DEBUG_WITH_TYPE("polly-exp-light",
                  errs() << "Set New Dependences[Ordered: " << Ordered
                         << "]\nRAW: " << RAW << "\nWAR: " << WAR
                         << "\nWAW: " << WAW << "\n");

  Dependences *D =
      new Dependences(IslCtx, Dependences::AL_Statement, RAW, WAR, WAW);
  DI->setDependences(Dependences::AL_Statement, D);
}

void ExpressionPropagation::dumpGraph() {
  errs() << "\nDefNodeGroups:\n";
  for (const auto &It : DefNodeGroupMap) {
    auto &Nodes = It.second->Nodes;
    errs() << "\tSAI: " << It.first->getName() << " [Size: " << Nodes.size()
           << "] [Affine: " << It.second->IsAffine << "]\n\t{\n";
    for (auto *Def : Nodes)
      Def->print(errs(), 4);
    errs() << "\t}\n";
  }
}

bool ExpressionPropagation::runOnScop(Scop &S) {
  DI = &getAnalysis<DependenceInfo>();
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  TLI = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
  TTI = &getAnalysis<TargetTransformInfoWrapperPass>().getTTI(S.getFunction());

// isl_options_set_on_error(S.getIslCtx(), ISL_ON_ERROR_WARN);

#ifndef NDEBUG
  // Verify scop
  for (auto &Stmt : S) {
    unsigned NumWrite = 0;
    bool IsValue = false;
    for (auto *MA : Stmt) {
      NumWrite += MA->isWrite();
      IsValue |= MA->isWrite() && !MA->isArrayKind();
    }
    if ((NumWrite > 1 && !Stmt.isRegionStmt()) || IsValue)
      Stmt.dump();
    assert(NumWrite <= 1 || Stmt.isRegionStmt());
    if (IsValue)
      return false;
  }
#endif

  AssumedContext = S.getAssumedContext();
  DEBUG(errs() << "Before expression propagation:\n"; S.dump());

  createDependenceGraph(S);

  DEBUG_WITH_TYPE("polly-exp-light", dumpGraph());
  DEBUG_WITH_TYPE("polly-exp", dumpGraph());

  DEBUG({
    for (auto &It : DefNodeMap)
      It.second->verify();
  });

  //simplifyAccesses(S);
  //mergeSimpleNodes(S);

  DEBUG({
    for (auto &It : DefNodeMap)
      It.second->verify();
  });

  bool Changed = false;
  if (AllButProp)
    Changed = true;
  else
    Changed = propagateExpressions(S);

  if (!Changed) {
    DEBUG(errs() << "No propagations!\n");
    clear();
    return false;
  }

  DEBUG_WITH_TYPE("polly-exp-light", {
    errs() << "After expression propagation:\n";
    S.dump();

    dumpGraph();
  });
  DEBUG_WITH_TYPE("polly-exp", {
    errs() << "After expression propagation:\n";
    S.dump();

    dumpGraph();
  });

  deleteDeadIterations();
  mergeDefNodeGroups(S);

  DEBUG_WITH_TYPE("polly-exp-light", {
    errs() << "After dead iteration elimination:\n";
    S.dump();

    dumpGraph();
  });
  DEBUG_WITH_TYPE("polly-exp", {
    errs() << "After dead iteration elimination:\n";
    S.dump();

    dumpGraph();
  });

  adjustSchedule(S);
  schedule(S);

  DEBUG(errs() << "After schedule adjustment:\n"; S.dump());
  DEBUG_WITH_TYPE("polly-exp-light", errs() << "After schedule adjustment:\n";
                  S.dump());

  S.markAsOptimized();
  // S.simplifySCoP(true);
  if (!S.isProfitable(true, false)) {
    S.invalidate(PROFITABLE, DebugLoc());
  } else {
    DEBUG(errs() << "Set new dependences\n");
    DEBUG_WITH_TYPE("polly-exp-light", errs() << "Set new dependences\n");

    setNewDependences(S);

    DEBUG(errs() << "Finished setting new dependences\n");
    DEBUG_WITH_TYPE("polly-exp-light",
                    errs() << "Finished setting new dependences\n");
  }

  clear();
  return Changed;
}

void ExpressionPropagation::clear() {
  for (auto &It : DefNodeMap) {
    auto *Def = It.second;
    Def->applyToEdges<WAWEdge>([&](WAWEdge &Edge) {
      if (Edge.Src != Def)
        return;
      auto Key = Edge.getKey<WAWEdge>();
      Edge.Trg->getEdges<WAWEdge>().erase(Key);
      delete &Edge;
    });
  }
  DeleteContainerSeconds(DefNodeMap);
  DeleteContainerSeconds(UseNodeMap);
  SplitStmts.clear();
  UseNodeMap.clear();
  DefNodeMap.clear();
  Worklist.clear();
  DefNodeGroupMap.clear();
  Ordered = false;

  MergeDependences = isl_union_map_free(MergeDependences);
  ValidityDeps = isl_union_map_free(ValidityDeps);
  AssumedContext = isl_set_free(AssumedContext);
}

void ExpressionPropagation::getAnalysisUsage(AnalysisUsage &AU) const {
  ScopPass::getAnalysisUsage(AU);
  AU.addRequired<DependenceInfo>();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
}

void ExpressionPropagation::printScop(raw_ostream &OS, Scop &S) const {
  OS << "Scop after transformation :\n\n";
  S.print(OS);
}

Pass *polly::createExpressionPropagationPass() {
  return new ExpressionPropagation();
}

char ExpressionPropagation::ID = 0;

INITIALIZE_PASS_BEGIN(ExpressionPropagation, "polly-exp",
                      "Polly - Propagate Expressions", false, false)
INITIALIZE_PASS_DEPENDENCY(DependenceInfo)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(ScopInfoRegionPass)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_END(ExpressionPropagation, "polly-exp",
                    "Polly - Propagate Expressions", false, false)
