//===--- ExpressionPropagation.h ------- expression propagation *- C++ -*-===//
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

#ifndef POLLY_EXPRESSION_PROPAGATION_H
#define POLLY_EXPRESSION_PROPAGATION_H

#include "polly/DependenceInfo.h"
#include "polly/LinkAllPasses.h"
#include "polly/ScopInfo.h"
#include "polly/ScopPass.h"
#include "polly/Support/GICHelper.h"
#include "llvm/ADT/PriorityQueue.h"
#include "llvm/Support/raw_ostream.h"
#include "isl/map.h"
#include "isl/schedule_node.h"
#include "isl/set.h"
#include "isl/union_map.h"
#include "isl/union_set.h"

#include <chrono>
#include <ctime>

namespace llvm {
class SCEV;
class TargetLibraryInfo;
class TargetTransformInfo;
}

namespace polly {
class Scop;
class ScopStmt;
class ScopPass;
class ScopArrayInfo;

void sortBB(ScopStmt &Stmt, llvm::StoreInst *SI, llvm::Loop *L, LoopInfo &LI,
            const DominatorTree &DT);

class ExpressionPropagation;
struct UseNode;
struct DefNode;
struct DefNodeGroup;

template <typename EdgeTy>
using EdgeKey = std::pair<typename EdgeTy::SrcTy *, typename EdgeTy::TrgTy *>;

template <typename EdgeTy>
using EdgeVectorTy = DenseMap<EdgeKey<EdgeTy>, EdgeTy *>;

using DefNodeGroupMapTy = DenseMap<const ScopArrayInfo *, DefNodeGroup *>;
using DefNodeMapTy = DenseMap<MemoryAccess *, DefNode *>;
using UseNodeMapTy = DenseMap<MemoryAccess *, UseNode *>;

enum RejectionReasonTy {
  RR_NONE = 1,
  RR_DOMAIN_SPLIT = 2,
  RR_BLOCKED = 4,
  RR_PROPAGATION_TIMES = 8,
  RR_COMMAND_LINE = 16,
  RR_PROPAGATED = 32,
  RR_NON_AFFINE = 64,
  RR_EXPANSION = 128,
  RR_HEURISTIC = 256,
  RR_REGISTERS = 512,
  RR_NEXT_IT = 1024,
  RR_MEM_STREAMS = 2048,
};

template<typename TrgTy, bool IsRAW = std::is_same<TrgTy, UseNode>::value>
struct RejectionReason {
  RejectionReasonTy Value = RR_NONE;
  operator RejectionReasonTy() { return Value; }
  RejectionReason &operator=(RejectionReasonTy NewValue) {
    Value = NewValue;
    return *this;
  }
  RejectionReason &operator=(int RRValue) {
    Value = RRValue;
    return *this;
  }

  std::string toString() const {
    switch(Value) {
      case RR_NONE: return "NONE";
      case RR_DOMAIN_SPLIT: return "SPLIT";
      case RR_PROPAGATED: return "PROPAGATED";
      case RR_BLOCKED: return "BLOCKED";
      case RR_PROPAGATION_TIMES: return "PROPTIMES";
      case RR_COMMAND_LINE: return "COMMANDLINE";
      case RR_NON_AFFINE: return "NON_AFFINE";
      case RR_EXPANSION: return "EXPANSION";
      case RR_HEURISTIC: return "HEURISTIC";
      case RR_REGISTERS: return "REGISTERS";
      case RR_NEXT_IT: return "NEXT_IT";
      case RR_MEM_STREAMS: return "MEM_STREAMS";
    };
    return "ERROR";
  }
};
template <typename TrgTy> struct RejectionReason<TrgTy, false> {};


template <typename SrcTyT, typename TrgTyT> struct Edge {
  using SrcTy = SrcTyT;
  using TrgTy = TrgTyT;

  SrcTy *Src;
  TrgTy *Trg;

  RejectionReason<TrgTy> RR;

  Edge() : Src(nullptr), Trg(nullptr), Dep(nullptr) {}

  //
  Edge(SrcTy *Src, TrgTy *Trg, isl_map *Dep, bool Notify = true)
      : Src(Src), Trg(Trg), Dep(Dep) {
    assert(Dep && Src && Trg);
    assert(isl_map_has_tuple_name(Dep, isl_dim_in));
    assert(isl_map_has_tuple_name(Dep, isl_dim_out));
    //errs() << "NewE: " << getPtrPairStr() << " " << getDepStr() << " " << this << "\n";
    if (Notify)
      notifyEdgeMore(nullptr);
  }

  Edge(const Edge &E) = delete;

  ~Edge() {
    //errs() << "DelE: "<< getPtrPairStr() <<" " << getDepStr() << " " << this << "\n";
    Dep = isl_map_free(Dep);
  }

  void clear(bool Notify = true) {
    //errs() << "ClrE: "<< getPtrPairStr() <<" " << getDepStr() << " " << this << "\n";
    auto Key = getKey<Edge<SrcTy, TrgTy>>();
    Src->template getEdges<Edge<SrcTy, TrgTy>>().erase(Key);
    Trg->template getEdges<Edge<SrcTy, TrgTy>>().erase(Key);
    if (Notify) {
      isl_map *OldDep = Dep;
      Dep = nullptr;
      notifyEdgeLess(isl_map_copy(OldDep), true);
      notifyEdgeLess(OldDep, false);
    } else {
      Dep = isl_map_free(Dep);
    }
    delete this;
  }

  bool isPropagated() const {
    return false; /* no-op, specialized for RAW */
  }

  Edge &operator=(const Edge &Other) {
    Src = Other.Src;
    Trg = Other.Trg;
    Dep = Other.getDep();
    assert(Dep);
    assert(isl_map_has_tuple_name(Dep, isl_dim_in));
    assert(isl_map_has_tuple_name(Dep, isl_dim_out));
    return *this;
  }

  operator bool() const { return Dep; }

  template <typename EdgeTy> EdgeKey<EdgeTy> getKey() const {
    return Edge::template getKey<Edge<SrcTy, TrgTy>>(Src, Trg);
  }

  template <typename EdgeTy>
  static EdgeKey<EdgeTy> getKey(typename EdgeTy::SrcTy *Src,
                                typename EdgeTy::TrgTy *Trg) {
    return std::make_pair(Src, Trg);
  }

  __isl_give isl_map *getDep() const {
    assert(this);
    return isl_map_copy(Dep);
  }

  std::string getPtrStr() const {
    return "[" + std::to_string((long long)this) + "]";
    return "";
  }
  std::string getPtrPairStr() const {
    return "[" + std::to_string((long long)Src) + ":" +
           std::to_string((long long)Trg) + "]";
    return "";
  }

  std::string getDepStr() const {
    return stringFromIslObj(Dep) + " " + getPtrStr();
  }

  void addDep(SrcTy &Src, TrgTy &Trg, __isl_take isl_map *Dep) {
    assert(Dep);
    assert(isl_map_has_tuple_name(Dep, isl_dim_in));
    assert(isl_map_has_tuple_name(Dep, isl_dim_out));
    assert(this->Dep && *this);
    if (isl_map_is_subset(Dep, this->Dep)) {
      isl_map_free(Dep);
      return;
    }
    isl_map *OldDep = isl_map_copy(this->Dep);
    this->Dep = isl_map_coalesce(isl_map_union(this->Dep, Dep));
    this->Dep = isl_map_detect_equalities(this->Dep);
    notifyEdgeMore(OldDep);
  }

  void intersectRange(__isl_take isl_set *R) {
    assert(R && Dep);
    isl_map *OldDep = isl_map_copy(Dep);
    Dep = isl_map_coalesce(isl_map_intersect_range(Dep, R));
    notifyEdgeLess(OldDep, /* Domain */ false);
  }

  void intersectDomain(__isl_take isl_set *D) {
    assert(D && Dep);
    isl_map *OldDep = isl_map_copy(Dep);
    Dep = isl_map_coalesce(isl_map_intersect_domain(Dep, D));
    notifyEdgeLess(OldDep, /* Domain */ true);
  }

  void subtract(__isl_take isl_map *M) {
    assert(M && Dep);
    isl_map *OldDep = isl_map_copy(Dep);
    Dep = isl_map_coalesce(isl_map_subtract(Dep, M));
    notifyEdgeLess(isl_map_copy(OldDep), /* Domain */ false);
    notifyEdgeLess(OldDep, /* Domain */ true);
  }

  void setDep(__isl_take isl_map *M) {
    assert(M && Dep);
    isl_map_free(Dep);
    Dep = M;
  }

  bool isEmpty() const { return isl_map_is_empty(Dep); }

  void print(raw_ostream &OS) const { OS << Dep; }

private:

  void notifyEdgeLess(__isl_take isl_map *OldDep, bool Domain);
  void notifyEdgeMore(__isl_take isl_map *OldDep);

  isl_map *Dep;
};

// Write -> Read
using RAWEdge = Edge<DefNode, UseNode>;

// Read -> Write
using WAREdge = Edge<UseNode, DefNode>;

// Write -> Write
using WAWEdge = Edge<DefNode, DefNode>;

struct Node {
  template <typename EdgeTy>
  EdgeTy *addEdge(typename EdgeTy::TrgTy &Trg, __isl_take isl_map *Dep) {
    typename EdgeTy::SrcTy *Src = static_cast<typename EdgeTy::SrcTy *>(this);
    EdgeKey<EdgeTy> Key = EdgeTy::template getKey<EdgeTy>(Src, &Trg);

    EdgeTy *&CurEdge = getEdges<EdgeTy>()[Key];
    if (CurEdge) {
      CurEdge->addDep(*Src, Trg, Dep);
      return CurEdge;
    }

    CurEdge = new EdgeTy(Src, &Trg, Dep);
    addExistingEdge<EdgeTy>(Trg, Key, CurEdge);
    return CurEdge;
  }

  bool isAffine() const { return MA->isAffine(); }

  template <typename EdgeTy>
  void copyEdgesTo(typename EdgeTy::TrgTy &NewTrg,
                   __isl_keep isl_set *NewDomain,
                   __isl_keep isl_id *OldId) const {
    //assert(&getDefNode() != &NewTrg.getDefNode());
    isl_map *Mapping =
        isl_map_identity(isl_space_map_from_set(isl_set_get_space(NewDomain)));
    Mapping = isl_map_intersect_domain(Mapping, isl_set_copy(NewDomain));
    Mapping = isl_map_intersect_range(Mapping, isl_set_copy(NewDomain));
    Mapping = isl_map_set_tuple_id(Mapping, isl_dim_in, isl_id_copy(OldId));
    copyEdgesTo<EdgeTy>(NewTrg, Mapping);
    isl_map_free(Mapping);
  }

  template <typename EdgeTy>
  void copyEdgesTo(typename EdgeTy::TrgTy &NewTrg,
                   __isl_keep isl_map *Mapping) const {
    //assert(&getDefNode() != &NewTrg.getDefNode());
    applyToEdges<EdgeTy>([&](const EdgeTy &Edge) {
      assert(&getDefNode() == &Edge.Trg->getDefNode());
      isl_map *Dep = isl_map_apply_range(Edge.getDep(), isl_map_copy(Mapping));
      assert(Dep);
      if (isl_map_is_empty(Dep)) {
        isl_map_free(Dep);
      } else {
        Edge.Src->template addEdge<EdgeTy>(NewTrg, Dep);
      }
    });
  }

  template <typename EdgeTy>
  void copyEdgesFrom(typename EdgeTy::SrcTy &NewSrc,
                     __isl_keep isl_set *NewDomain,
                     __isl_keep isl_id *OldId) const {
    //assert(&getDefNode() != &NewSrc.getDefNode());
    isl_map *Mapping =
        isl_map_identity(isl_space_map_from_set(isl_set_get_space(NewDomain)));
    Mapping = isl_map_intersect_domain(Mapping, isl_set_copy(NewDomain));
    Mapping = isl_map_intersect_range(Mapping, isl_set_copy(NewDomain));
    Mapping = isl_map_set_tuple_id(Mapping, isl_dim_in, isl_id_copy(OldId));
    copyEdgesFrom<EdgeTy>(NewSrc, Mapping);
    isl_map_free(Mapping);
  }

  template <typename EdgeTy>
  void copyEdgesFrom(typename EdgeTy::SrcTy &NewSrc,
                     __isl_keep isl_map *Mapping) const {
    //assert(&getDefNode() != &NewSrc.getDefNode());
    applyToEdges<EdgeTy>([&](const EdgeTy &Edge) {
      assert(&getDefNode() == &Edge.Src->getDefNode());
      isl_map *Dep = isl_map_apply_domain(Edge.getDep(), isl_map_copy(Mapping));
      assert(Dep);
      if (isl_map_is_empty(Dep)) {
        isl_map_free(Dep);
      } else {
        NewSrc.template addEdge<EdgeTy>(*Edge.Trg, Dep);
      }
    });
  }

  template <typename EdgeTy>
  void applyToAndCheckEdges(std::function<bool(EdgeTy &)> Fn, bool Notify = true) {
    applyToEdges<EdgeTy>([&](EdgeTy &Edge) {
      if (Fn(Edge))
        return;
       Edge.clear(Notify);
    });
  }

  template <typename EdgeTy>
  void applyToEdges(std::function<void(EdgeTy &)> Fn) {
    SmallVector<EdgeTy *, 32> Edges;
    for (auto &It : getEdges<EdgeTy>()) {
      EdgeTy *Edge = It.second;
      assert(Edge);
      Edges.push_back(Edge);
    }
    for (auto *Edge : Edges)
      Fn(*Edge);
  }

  template <typename EdgeTy>
  void applyToEdges(std::function<void(const EdgeTy &)> Fn) const {
    SmallVector<EdgeTy *, 32> Edges;
    for (auto &It : getEdges<EdgeTy>()) {
      EdgeTy *Edge = It.second;
      assert(Edge);
      Edges.push_back(Edge);
    }
    for (auto *Edge : Edges)
      Fn(*Edge);
  }

  EdgeVectorTy<RAWEdge> &getEdges(RAWEdge *) { return RAWEdges; }
  EdgeVectorTy<WAREdge> &getEdges(WAREdge *) { return WAREdges; }
  EdgeVectorTy<WAWEdge> &getEdges(WAWEdge *) { return WAWEdges; }
  const EdgeVectorTy<RAWEdge> &getEdges(RAWEdge *) const { return RAWEdges; }
  const EdgeVectorTy<WAREdge> &getEdges(WAREdge *) const { return WAREdges; }
  const EdgeVectorTy<WAWEdge> &getEdges(WAWEdge *) const { return WAWEdges; }

  template <typename EdgeTy> EdgeVectorTy<EdgeTy> &getEdges() {
    return getEdges((EdgeTy *)nullptr);
  }
  template <typename EdgeTy> const EdgeVectorTy<EdgeTy> &getEdges() const {
    return getEdges((EdgeTy *)nullptr);
  }

  MemoryAccess *MA;

  Node(MemoryAccess *MA) : MA(MA) {}
  virtual ~Node() {}

  ScopStmt *getStmt() const { return MA->getStatement(); }

  virtual void verify() const = 0;
  virtual std::string getName() const { return getStmt()->getBaseName(); }

  virtual DefNode &getDefNode() = 0;
  virtual const DefNode &getDefNode() const = 0;
  virtual __isl_give isl_set *getDomain() const = 0;
  virtual std::string getDomainStr() const = 0;
  virtual __isl_give isl_map *getSchedule() const = 0;

  void printEdges(raw_ostream &OS, unsigned Indent = 0) const;

  void print(raw_ostream &OS, unsigned Indent = 0) const {}

private:
  template <typename EdgeTy>
  void addExistingEdge(typename EdgeTy::TrgTy &Trg, EdgeKey<EdgeTy> &Key,
                       EdgeTy *Edge) {
    auto *Src = Edge->Src;
    EdgeTy *&CurEdge = Trg.template getEdges<EdgeTy>()[Key];
    if (CurEdge)
      CurEdge->addDep(*Src, Trg, Edge->getDep());
    else
      CurEdge = Edge;
  }

  EdgeVectorTy<RAWEdge> RAWEdges;
  EdgeVectorTy<WAREdge> WAREdges;
  EdgeVectorTy<WAWEdge> WAWEdges;
};

struct UseNode : public Node {

  DefNode *Def;
  bool NextItNode = false;

  UseNode(MemoryAccess *UseMA, DefNode *Def) : Node(UseMA), Def(Def) {
    assert(UseMA && UseMA->isRead());
  }
  ~UseNode() {}

  void verify() const override;
  __isl_give isl_set *getDomain() const override;
  __isl_give isl_map *getSchedule() const override;
  std::string getDomainStr() const override;
  DefNode &getDefNode() override;
  const DefNode &getDefNode() const override;

  void print(raw_ostream &OS, unsigned Indent = 0) const;
};

struct DefNode : public Node {

  DefNodeGroup *Group = nullptr;
  SetVector<UseNode *> Operands;

  DenseMap<const ScopArrayInfo *, unsigned> Arrays;
  SmallPtrSet<BasicBlock *, 8> Blocks;
  bool IsOverwritten;
  bool OperandsCollected = false;

  ExpressionPropagation &ExprProp;

  DefNode(MemoryAccess *DefMA, ExpressionPropagation &ExprProp, isl_set *Domain);
  DefNode(DefNode &Other, MemoryAccess *DefMA,
          DenseMap<MemoryAccess *, MemoryAccess *> &AccMap, isl_set *Domain,
          __isl_keep isl_set *LeftDomain);

  ~DefNode() {
    MA = nullptr;
    Domain = isl_set_free(Domain);
    Schedule = isl_map_free(Schedule);
    freeUsedDomain();
    freeBarrier();
    freeOverwritten();
    DeleteContainerSeconds(getEdges<RAWEdge>());
    DeleteContainerSeconds(getEdges<WAREdge>());
  }

  bool isLiveOut() const;

  template<typename EdgeTy>
  unsigned numOutgoingEdges() const {
    return getEdges<EdgeTy>().size();
  }

  unsigned numIncomingEdges() const {
    unsigned IncomingRAWs = 0;
    for (auto *Op : Operands)
      IncomingRAWs += Op->getEdges<RAWEdge>().size();
    return IncomingRAWs;
  }

  unsigned getNumInsts() const {
    unsigned NumInsts = 0;
    for (auto *BB: Blocks)
      NumInsts += BB->size();
    return NumInsts;
  }

  void removeOperand(UseNode &Op) {
    size_t Size = Operands.size();
    Op.applyToAndCheckEdges<WAREdge>([&](WAREdge &Edge) { return false; });
    //DEBUG(Op.applyToEdges<RAWEdge>(
        //[&](const RAWEdge &Edge) { assert(Edge.isPropagated()); }););
    Operands.remove(&Op);
    assert(Operands.size() + 1 == Size);
    (void)Size;

    unsigned &Count = Arrays[Op.MA->getScopArrayInfo()];
    if (--Count == 0)
      Arrays.erase(Op.MA->getScopArrayInfo());
  }

  bool hasBarrier() {
    if (!Barrier)
      isl_map_free(getBarrier());
    return !isl_map_is_empty(Barrier);
  }

  __isl_give isl_map *getBarrier() {
    if (!Barrier) {
      for (auto *Op : Operands) {
        for (auto &It : Op->getEdges<WAREdge>()) {
          WAREdge *WAR = It.second;
          assert(WAR && *WAR);
          isl_map *Dep = WAR->getDep();
          isl_map *TrgSched = WAR->Trg->getSchedule();
          Dep = isl_map_apply_range(Dep, TrgSched);
          assert(Dep);
          Barrier = Barrier ? isl_map_union(Barrier, Dep) : Dep;
        }
      }
    }
    return isl_map_copy(Barrier);
  }
  bool freeBarrier() {
    bool Changed = Barrier;
    Barrier = isl_map_free(Barrier);
    return Changed;
  }
  void addToBarrier(isl_map *M) {
    if (UsedDomain)
      Barrier = isl_map_coalesce(isl_map_union(Barrier, M));
    else
      isl_map_free(M);
  }

  __isl_give isl_set *getUsedDomain(bool Force) {
    bool Complex = false;
    if (!UsedDomain) {
      UsedDomain = isl_set_empty(isl_set_get_space(Domain));
      applyToEdges<RAWEdge>([&](const RAWEdge &Edge) {
        if (Complex)
          return;
        if (Edge.RR.Value == RR_NEXT_IT)
          return;
        if (Edge)
          UsedDomain = isl_set_coalesce(
              isl_set_union(UsedDomain, isl_map_domain(Edge.getDep())));
        if (Force || isl_set_n_basic_set(UsedDomain) <= 4)
          return;
        UsedDomain = isl_set_free(UsedDomain);
        Complex = true;
      });
    }
    assert(Complex || UsedDomain);
    return isl_set_copy(UsedDomain);
  }
  bool freeUsedDomain() { bool Changed = UsedDomain; UsedDomain = isl_set_free(UsedDomain); return Changed; }
  void intersectUsedDomain() {
    if (UsedDomain)
      UsedDomain = isl_set_intersect(UsedDomain, isl_set_copy(Domain));
  }
  void addToUsedDomain(isl_set *S) {
    if (UsedDomain)
      UsedDomain = isl_set_coalesce(isl_set_union(UsedDomain, S));
    else
      isl_set_free(S);
  }

  template<bool Check>
  __isl_give isl_set *getOverwrittenSet() {
    if (!Overwritten) {
      Overwritten = isl_set_empty(isl_set_get_space(Domain));
      applyToEdges<WAWEdge>([&](const WAWEdge &Edge) {
        if (Edge.Src != this)
          return;

        assert(Edge);
        isl_map *EdgeDep = Edge.getDep();
        assert(EdgeDep);

        isl_set *EdgeDom = isl_map_domain(EdgeDep);
        Overwritten = isl_set_coalesce(isl_set_union(Overwritten, EdgeDom));
      });
    }
    assert(Overwritten);
    return isl_set_copy(Overwritten);
  }

  bool freeOverwritten() {
    bool Changed = Overwritten;
    Overwritten = isl_set_free(Overwritten);
    return Changed;
  }

  __isl_give isl_map *getSelfWARDep() {
    isl_map *SelfWARDep = nullptr;
    applyToEdges<WAREdge>([&](const WAREdge &Edge) {
      if (&Edge.Src->getDefNode() != Edge.Trg)
        return;

      SelfWARDep = SelfWARDep ?
                   isl_map_coalesce(isl_map_union(SelfWARDep, Edge.getDep()))
                   : Edge.getDep();
    });
    return SelfWARDep;
  }

  bool setDomain(__isl_take isl_set *NewDomain);
  void addDomain(__isl_take isl_set *NewDomain) {
    Domain = isl_set_coalesce(isl_set_union(Domain, NewDomain));
  }

  void verify() const override;
  __isl_give isl_set *getDomain() const override {
    assert(Domain);
    return isl_set_copy(Domain);
  }

  __isl_give isl_map *getSchedule() const override {
    assert(Schedule);
    return isl_map_copy(Schedule);
  }

  std::string getDomainStr() const override { return stringFromIslObj(Domain); }

  bool isEmpty() const { return isl_set_is_empty(Domain); }

  DefNode &getDefNode() override;
  const DefNode &getDefNode() const override;

  void addOperand(UseNode *Op) {
    assert(Op && Op->MA);
    Arrays[Op->MA->getScopArrayInfo()]++;
    Operands.insert(Op);
  }

  void collectOperands(UseNodeMapTy &UseNodeMap);

  void print(raw_ostream &OS, unsigned Indent = 0) const;

private:
  isl_set *Domain;
  isl_set *UsedDomain = nullptr;
  isl_map *Barrier = nullptr;
  isl_set *Overwritten = nullptr;
  isl_map *Schedule;
};

struct DefNodeGroup {

  SmallVector<DefNode *, 8> Nodes;

  const ScopArrayInfo * const SAI;

  bool IsAffine = true;
  bool IsLiveOut;

  size_t MinAccessNo = -1;

  int getNumArrayLoweringPotential() const {
    int Potential = 0;
    for (DefNode *Node : Nodes) {
      Node->applyToEdges<RAWEdge>([&](const RAWEdge &Edge){
        DefNode &Trg = Edge.Trg->getDefNode();
        Potential += 1;
        for (auto &It : Node->Arrays)
          if (!Trg.Arrays.count(It.first))
          Potential -= 1;
      });
    }
    return Potential;
  }

  DefNodeGroup(const ScopArrayInfo *SAI, DefNode &Def, TargetLibraryInfo &TLI)
      : SAI(SAI), IsLiveOut(isLiveOut(*Def.MA, &TLI)) {
    addNode(Def);
  }

  void addNode(DefNode &Def) {
    IsAffine &= Def.isAffine();
    //assert(!Def.Group);
    Def.Group = this;
    MinAccessNo = std::min(MinAccessNo, Def.MA->AccNo);
    Nodes.push_back(&Def);
  }

  template<typename EdgeTy>
  unsigned numOutgoingEdges() const {
    unsigned NumEdges = 0;
    for (auto *Def : Nodes)
      NumEdges += Def->numOutgoingEdges<EdgeTy>();
    return NumEdges;
  }

  unsigned numIncomingEdges() const {
    unsigned NumEdges = 0;
    for (auto *Def : Nodes)
      NumEdges += Def->numIncomingEdges();
    return NumEdges;
  }

  bool isEmpty() const {
    for (auto *Def : Nodes)
      if (!Def->isEmpty())
        return false;
    return true;
  }

  void verify() const {
    for (auto *Def : Nodes)
      Def->verify();
  }
};

struct GroupNodeCompare {
  // True  => D1, D0
  // False => D0, D1
  bool compare(const DefNodeGroup *DNG0, const DefNodeGroup *DNG1) const {
    int PotentialToLowerNumArrays0 = DNG0->getNumArrayLoweringPotential();
    int PotentialToLowerNumArrays1 = DNG1->getNumArrayLoweringPotential();
    if (PotentialToLowerNumArrays0 != PotentialToLowerNumArrays1)
      return PotentialToLowerNumArrays0 < PotentialToLowerNumArrays1;

    auto NumOutgoingRAW0 = DNG0->numOutgoingEdges<RAWEdge>();
    auto NumOutgoingRAW1 = DNG1->numOutgoingEdges<RAWEdge>();
    if (NumOutgoingRAW0 != NumOutgoingRAW1)
      return NumOutgoingRAW0 > NumOutgoingRAW1;

    if (DNG0->Nodes.size() != DNG1->Nodes.size())
      return DNG0->Nodes.size() > DNG1->Nodes.size();

    unsigned Ops0 = 0, Ops1 = 0;
    std::for_each(DNG0->Nodes.begin(), DNG0->Nodes.end(),
                  [&](DefNode *Def) { Ops0 += Def->Operands.size(); });
    std::for_each(DNG1->Nodes.begin(), DNG1->Nodes.end(),
                  [&](DefNode *Def) { Ops1 += Def->Operands.size(); });
    if (Ops0 != Ops1)
      return Ops0 > Ops1;

    if (DNG0->SAI->getNumberOfDimensions() !=
        DNG1->SAI->getNumberOfDimensions())
      return DNG0->SAI->getNumberOfDimensions() >
             DNG1->SAI->getNumberOfDimensions();

    auto NumIncomingRAW0 = DNG0->numIncomingEdges();
    auto NumIncomingRAW1 = DNG1->numIncomingEdges();
    if (NumIncomingRAW0 != NumIncomingRAW1)
      return NumIncomingRAW0 > NumIncomingRAW1;

    return DNG0->MinAccessNo < DNG1->MinAccessNo;
  }
  bool operator()(const DefNodeGroup *DNG0, const DefNodeGroup *DNG1) const {
    return compare(DNG0, DNG1);
  }
};

template <>
void Node::addExistingEdge<WAWEdge>(typename WAWEdge::TrgTy &Trg,
                                    EdgeKey<WAWEdge> &Key, WAWEdge *Edge) {
  auto *Src = Edge->Src;
  if (&Trg == Src)
    return;
  WAWEdge *&CurEdge = Trg.getEdges<WAWEdge>()[Key];
  if (CurEdge)
    CurEdge->addDep(*Src, Trg, Edge->getDep());
  else
    CurEdge = Edge;
}

class ExpressionPropagation : public ScopPass {
public:
  DefNodeGroupMapTy DefNodeGroupMap;
  DefNodeMapTy DefNodeMap;
  UseNodeMapTy UseNodeMap;
  SmallPtrSet<DefNode *, 8> MustDeleteNodes;

  SmallPtrSet<DefNodeGroup *, 32> WorklistSet;
  PriorityQueue<DefNodeGroup *, std::vector<DefNodeGroup *>, GroupNodeCompare> Worklist;

  bool restrictStmt(DefNode &Def, bool Force = false);

  UseNode *copyOperandInto(UseNode &OpUse, DefNode &StmtDef,
                           MemoryAccess &OpUseCopyMA);
  void propagateEdgesToReloadOp(UseNode &Op, UseNode &ReloadOp, RAWEdge &Edge);

  isl_union_map *MergeDependences = nullptr;
  isl_union_map *ValidityDeps = nullptr;
  bool Ordered = false;
  DenseMap<DefNodeGroup *, unsigned> FullMergeCostMap;

private:

  void addReloadAccesses(
      ScopStmt &Stmt, RAWEdge &Edge,
      DenseMap<MemoryAccess *, MemoryAccess *> &NewAccs);
  void addRecomputePair(
      ScopStmt &Stmt, RAWEdge &Edge,
      DenseMap<MemoryAccess *, MemoryAccess *> &NewAccs);

  bool propagateEdges(DefNodeGroup &DNG);
  void propagateEdge(RAWEdge &Edge);
  DefNode *splitUsingDefNode(DefNode &UsingDef,
                             __isl_take isl_set *RecomputeDomain);

  unsigned
  computeNumAccessedStreams(ArrayRef<DefNode *> Nodes,
                            SmallVectorImpl<isl_map *> &NodeDeps,
                            SmallPtrSetImpl<MemoryAccess *> &ExcludedMAs, unsigned Limit);
  unsigned computeNumAccessedStreams(DefNode * Trgs,
                                     SmallVectorImpl<RAWEdge *> &Edges, unsigned Limit);
  std::pair<unsigned, unsigned> computeNumOfRegsNeeded(SmallVectorImpl<RAWEdge *> &Edges);
  bool isWorthRecomputing(DefNode *Trg, SmallVectorImpl<RAWEdge *> &Edges, unsigned &Streams);
  unsigned getNeededFixedDimSplits(RAWEdge &Edge, __isl_keep isl_set *RecomputeDomain);

  __isl_give isl_set *getRecomputeDomain(RAWEdge &Edge);

  void adjustSchedule(Scop &S);
  void schedule(Scop &S);
  isl_schedule *schedule(Scop &S, isl_union_set *Domain, isl_union_map *Deps,
                         __isl_keep isl_schedule_node *Node);

  SmallPtrSet<DefNode *, 4> SplitStmts;
  void createDependenceGraph(Scop &S);

  bool setDomain(DefNode &Def, __isl_take isl_set *NewDomain);

  DefNode *getDefNode(ScopStmt &Stmt);

  void simplifyAccesses(Scop &S);
  void mergeSimpleNodes(Scop &S);
  bool propagateExpressions(Scop &S);
  void deleteDeadIterations();
  void setMemOrderDeps2(DefNodeGroup *DNG, ArrayRef<DefNode *> Nodes);
  void setMemOrderDeps(DefNodeGroup *DNG, ArrayRef<DefNode *> Nodes);
  void tryFullMerge(DefNodeGroup *DNG, SmallVectorImpl<DefNode *> &Nodes);
  void mergeDefNodeGroups(Scop &S);

  void addDependences(Scop &S, isl_union_map *&RAW, isl_union_map *&WAR,
                      isl_union_map *&WAW);
  void setNewDependences(Scop &S);
  void dumpGraph();

  DenseMap<std::pair<Value *, Value *>, int> RAWProps;

  static isl_stat createRAWEdges(__isl_take isl_map *D, void *User);
  static isl_stat createWAREdges(__isl_take isl_map *D, void *User);
  static isl_stat createWAWEdges(__isl_take isl_map *D, void *User);

private:
  DependenceInfo *DI;
  LoopInfo *LI;
  const DominatorTree *DT;
  TargetLibraryInfo *TLI;
  TargetTransformInfo *TTI;
  isl_set *AssumedContext;

  void clear();

public:
  static char ID;

  TargetLibraryInfo *getTLI() const { return TLI; }
  __isl_give isl_set *getAssumedContext() const {
    return isl_set_copy(AssumedContext);
    }

    explicit ExpressionPropagation() : ScopPass(ID), AssumedContext(nullptr) {}

    /// @brief Propagate expressions when beneficial in the schedule of @p S.
    bool runOnScop(Scop & S) override;

    /// @brief Register all analyses and transformation required.
    void getAnalysisUsage(AnalysisUsage & AU) const override;

    /// @brief Print a source code representation of the program.
    void printScop(llvm::raw_ostream & OS, Scop & S) const override;
};

template<>
bool RAWEdge::isPropagated() const {
  return RR.Value == RR_PROPAGATED;
}

template<>
void RAWEdge::notifyEdgeLess(__isl_take isl_map *OldDep, bool Domain) {

  // If the old dependence was already null the new one is too and there is
  // nothing to do.
  if (!OldDep) {
    assert(!Dep);
    return;
  }

  bool IsPropagatedEdge = RR & RR_PROPAGATED;
  if (!(IsPropagatedEdge || Trg->getDefNode().Operands.count(Trg))) {
    errs() << "Src:\n"; Src->print(errs(), 4);
    errs() << "Trg:\n"; Trg->print(errs(), 4);
    errs() << "EDGE: " << getDepStr() << "  [" << RR.toString() << "]\n";
  }
  assert(IsPropagatedEdge || Trg->getDefNode().Operands.count(Trg));

  // Restricing a RAW edge can lead to new WAW edges as well as new RAW edges.
  // The following cases are possible:
  //   W0 - [WAW] - SRC - this - TRG  + smaller domain => new RAW(W0, TRG)
  //   SRC - this - TRG - [WAR] - W1  + smaller range  => new WAW(SRC, W1)
  // However, if this is the propagated edge we shall not add new ones!

  isl_map *DiffDep = nullptr;
  if (IsPropagatedEdge) {
    isl_map_free(OldDep);
  } else {
    isl_map *NewDep = getDep();
    DiffDep =  NewDep ? isl_map_subtract(OldDep, NewDep) : OldDep;
    assert(DiffDep);
  }

  bool NoDiff = DiffDep && isl_map_is_empty(DiffDep);
  if (NoDiff) {
    isl_map_free(DiffDep);
    return;
  }

  if (Domain) {
    // Handle the case with a chained WAW dependence:
    //   W0 - [WAW] - SRC - this - TRG  + smaller domain => new RAW(W0, TRG)
    // but only if this edge was not propagated!
    if (!IsPropagatedEdge) {
      Src->applyToEdges<WAWEdge>([=](const WAWEdge &WAW) {
        if (WAW.Trg != Src)
          return;
        assert(WAW);

        isl_map *RAWDep =
            isl_map_apply_range(WAW.getDep(), isl_map_copy(DiffDep));
        assert(RAWDep);

        if (isl_map_is_empty(RAWDep)) {
          isl_map_free(RAWDep);
        } else {
          auto *NewEdge = WAW.Src->addEdge<RAWEdge>(*Trg, RAWDep);
          NewEdge->RR = RR_NONE;
        }
      });
    }
    isl_map_free(DiffDep);

    // If only the src domain got smaller only adjust the used domain accordingly.
    // There is no new propagation opportunity for this edge.
    Src->intersectUsedDomain();
    return;
  }

  // Handle the case with a chained WAR dependence:
  //   SRC - this - TRG - [WAR] - W1  + smaller range  => new WAW(SRC, W1)
  // but only if this edge was not propagated!
  if (!IsPropagatedEdge) {
    Trg->applyToEdges<WAREdge>([=](const WAREdge &WAR) {
      isl_map *WAWDep =
          isl_map_apply_range(isl_map_copy(DiffDep), WAR.getDep());
      assert(WAWDep);

      if (isl_map_is_empty(WAWDep)) {
        isl_map_free(WAWDep);
      } else {
        Src->addEdge<WAWEdge>(*WAR.Trg, WAWDep);
      }
    });
  }
  isl_map_free(DiffDep);

  // The src is now less used, consequently free its used domain.
  Src->freeUsedDomain();

  // If this edge is now empty bail.
  if (!Dep || isl_map_is_empty(Dep))
    return;

}

template<>
void RAWEdge::notifyEdgeMore(__isl_take isl_map *OldDep) {
  assert(Dep);

  Src->addToUsedDomain(isl_map_domain(getDep()));

  if (!OldDep) {
    // A new RAW edge, just add it to the worklist if it has affine end nodes.
    assert(RR == RR_NONE);
    return;
  }

  isl_map_free(OldDep);
}

template<>
void WAREdge::notifyEdgeLess(__isl_take isl_map *OldDep, bool Domain) {
  assert(OldDep);
  isl_map_free(OldDep);

  // Restricing a WAR edge can lead to new WAW edges as well as new WAR edges.
  // The following cases are possible:
  //   W0 - [RAW] - SRC - this - TRG  + smaller domain => new WAW(W0, TRG)
  //   SRC - this - TRG - [WAW] - W1  + smaller range  => new WAR(SRC, W1)
  // However, the first is already handled in the notifyEdgeLess<RAWEdge>
  // function and the second in the notifyEdgeLess<WAWEdge> function.

  // If only the src domain got smaller we do not unblock the read
  if (Domain)
    return;

  // The src is now blocked less, consequently free the barrier of the def node
  // containing src.
  DefNode &SrcDef = Src->getDefNode();
  SrcDef.freeBarrier();
}

template <> void WAREdge::notifyEdgeMore(__isl_take isl_map *OldDep) {
  assert(Dep);

  isl_map *TrgSched = Trg->getSchedule();
  auto *D = isl_map_apply_range(getDep(), TrgSched);
  assert(D);
  Src->getDefNode().addToBarrier(D);

#if 0
  // If a WAR dependence increase it might make WAW dependences obsolet if they
  // are transitively implied by the WAR and a RAW dependence:
  // W0 - [RAW] - SRC - this - TRG
  //   \                          /
  //    \--------- [WAW] --------/

  isl_map *NewDep = getDep();
  isl_map *DiffDep =  OldDep ? isl_map_subtract(NewDep, OldDep) : NewDep;
  assert(DiffDep);

  Src->applyToEdges<RAWEdge>([=](const RAWEdge &RAW) {
    isl_map *RAWWARDep = RAW.getDep();
    RAWWARDep = isl_map_apply_range(RAWWARDep, isl_map_copy(DiffDep));
    if (isl_map_is_empty(RAWWARDep)) {
      isl_map_free(RAWWARDep);
      return;
    }

    auto Key = Edge::getKey<WAWEdge>(RAW.Src, Trg);
    WAWEdge *WAW = RAW.Src->getEdges<WAWEdge>().lookup(Key);
    if (!WAW) {
      isl_map_free(RAWWARDep);
      return;
    }

    WAW->subtract(RAWWARDep);
  });
  isl_map_free(DiffDep);
#else
  isl_map_free(OldDep);
#endif
}

template<>
void WAWEdge::notifyEdgeLess(__isl_take isl_map *OldDep, bool Domain) {
  // Restricing a WAW edge can lead to new WAW edges as well as new WAR edges.
  // The following cases are possible:
  //   W0 - [WAW] - SRC - this - TRG  + smaller domain => new WAW(W0, TRG)
  //   R0 - [WAR] - SRC - this - TRG  + smaller domain => new WAR(R0, TRG)
  //   SRC - this - TRG - [WAW] - W1  + smaller range  => new WAW(SRC, W1)
  //   SRC - this - TRG - [RAW] - R1  + smaller range  => new RAW(SRC, R1)
  // The last one is handled in the notifyEdgeLess<RAW> function.

  isl_map *NewDep = getDep();
  isl_map *DiffDep =  NewDep ? isl_map_subtract(OldDep, NewDep) : OldDep;
  assert(DiffDep);

  bool NoDiff = isl_map_is_empty(DiffDep);
  if (NoDiff) {
    isl_map_free(DiffDep);
    return;
  }

  if (Domain) {
    // Handle the case with a chained WAW dependence:
    //   W0 - [WAW] - SRC - this - TRG  + smaller domain => new WAW(W0, TRG)
    Src->applyToEdges<WAWEdge>([=](const WAWEdge &WAW) {
      if (WAW.Trg != Src)
        return;
      assert(WAW);

      isl_map *WAWDep = isl_map_apply_range(WAW.getDep(), isl_map_copy(DiffDep));
      assert(WAWDep);

      if (isl_map_is_empty(WAWDep)) {
        isl_map_free(WAWDep);
      } else {
        WAW.Src->addEdge<WAWEdge>(*Trg, WAWDep);
      }
    });

    // Handle the case with a chained WAR dependence:
    //   R0 - [WAR] - SRC - this - TRG  + smaller domain => new WAR(R0, TRG)
    Src->applyToEdges<WAREdge>([=](const WAREdge &WAR) {
      assert(WAR);

      isl_map *WARDep = isl_map_apply_range(WAR.getDep(), isl_map_copy(DiffDep));
      assert(WARDep);

      if (isl_map_is_empty(WARDep)) {
        isl_map_free(WARDep);
      } else {
        WAR.Src->applyToEdges<WAREdge>([&](WAREdge &Edge) {
          assert(Edge.Src->getDefNode().Operands.count(Edge.Src));
        });
        WAR.Src->addEdge<WAREdge>(*Trg, WARDep);
        WAR.Src->applyToEdges<WAREdge>([&](WAREdge &Edge) {
          assert(Edge.Src->getDefNode().Operands.count(Edge.Src));
        });
      }
    });

  } else {
    // Handle the case with a chained WAW dependence:
    //   SRC - this - TRG - [WAW] - W1  + smaller range  => new WAW(SRC, W1)
    Trg->applyToEdges<WAWEdge>([=](const WAWEdge &WAW) {
      if (WAW.Src != Trg)
        return;
      assert(WAW);

      isl_map *WAWDep =
          isl_map_apply_range(isl_map_copy(DiffDep), WAW.getDep());
      assert(WAWDep);

      if (isl_map_is_empty(WAWDep)) {
        isl_map_free(WAWDep);
      } else {
        Src->addEdge<WAWEdge>(*WAW.Trg, WAWDep);
      }
    });
  }
  isl_map_free(DiffDep);

  // The src is now less often overwritten by the trg stmt, consequently free
  // the old overwritten set.
  Src->freeOverwritten();
}

template<>
void WAWEdge::notifyEdgeMore(__isl_take isl_map *OldDep) {
  assert(Dep);

  // The src is now more often overwritten by the trg stmt, consequently free
  // the old overwritten set.
  Src->freeOverwritten();

  isl_map_free(OldDep);
}
}

#endif
