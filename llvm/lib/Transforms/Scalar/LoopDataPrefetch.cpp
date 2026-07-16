//===-------- LoopDataPrefetch.cpp - Loop Data Prefetching Pass -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements a Loop Data Prefetching Pass.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Scalar/LoopDataPrefetch.h"
#include "llvm/InitializePasses.h"

#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/CodeMetrics.h"
#include "llvm/Analysis/DomTreeUpdater.h"
#include "llvm/Analysis/IVDescriptors.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicsAArch64.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ReplaceConstant.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/ScalarEvolutionExpander.h"

#define DEBUG_TYPE "loop-data-prefetch"

using namespace llvm;

// By default, we limit this to creating 16 PHIs (which is a little over half
// of the allocatable register set).
static cl::opt<bool>
PrefetchWrites("loop-prefetch-writes", cl::Hidden, cl::init(false),
               cl::desc("Prefetch write addresses"));

static cl::opt<unsigned>
    PrefetchDistance("prefetch-distance",
                     cl::desc("Number of instructions to prefetch ahead"),
                     cl::Hidden);

static cl::opt<unsigned>
    MinPrefetchStride("min-prefetch-stride",
                      cl::desc("Min stride to add prefetches"), cl::Hidden);

static cl::opt<unsigned> MaxPrefetchIterationsAhead(
    "max-prefetch-iters-ahead",
    cl::desc("Max number of iterations to prefetch ahead"), cl::Hidden);

static cl::opt<bool>
    IndirectLoadPrefetch("indirect-load-prefetch", cl::Hidden, cl::init(false),
                         cl::desc("Enable indirect laod prefetch"));

static cl::opt<unsigned> PrefetchIterationsAhead(
    "indirect-prefetch-iters-ahead",
    cl::desc("Number of iterations for indirect-load prefetch"), cl::Hidden, cl::init(0));

static cl::opt<bool> SkipIntermediate(
    "indirect-prefetch-skip-intermediate", cl::Hidden, cl::init(false),
    cl::desc(
        "Skip prefetching intermediate loads while doing indirect prefetch"));

static cl::opt<unsigned> IndirectionLevel(
    "indirect-level",
    cl::desc("Indirection level considered for indirect load prefetch"),
    cl::Hidden, cl::init(2));

static cl::opt<bool> RandomAccessPrefetchOnly(
    "random-access-prefetch-only", cl::Hidden, cl::init(false),
    cl::desc("Enable only outer loop indirect load prefetch"));

static cl::opt<unsigned> CachelineSize("prefetch-cache-line-size",
                                       cl::desc("Specify cache line size"),
                                       cl::Hidden, cl::init(64));

static cl::opt<bool>
    OuterLoopPrefetch("outer-loop-prefetch", cl::Hidden, cl::init(false),
                      cl::desc("Enable prefetch in outer loops"));

static cl::opt<bool>
    DisableDirectLoadPrefetch("disable-direct-prefetch", cl::Hidden,
                              cl::init(false),
                              cl::desc("Disable direct load prefetch"));

static cl::opt<unsigned>
    PrefetchLoopDepth("prefetch-loop-depth",
                      cl::desc("Least loop depth to insert prefetch"),
                      cl::Hidden, cl::init(1));

STATISTIC(NumPrefetches, "Number of prefetches inserted");
STATISTIC(NumIndPrefetches, "Number of indirect prefetches inserted");
STATISTIC(NumOuterLoopPrefetches, "Number of outer loop prefetches inserted");

namespace {

// Helper function to return a type with the same size as
// given step size
static Type *getPtrTypefromPHI(PHINode *PHI, int64_t StepSize) {
  Type *Int8Ty = Type::getInt8Ty(PHI->getParent()->getContext());
  return ArrayType::get(Int8Ty, StepSize);
}

/// Loop prefetch implementation class.
class LoopDataPrefetch {
public:
  LoopDataPrefetch(AliasAnalysis *AA, AssumptionCache *AC, DominatorTree *DT,
                   LoopInfo *LI, ScalarEvolution *SE,
                   const TargetTransformInfo *TTI,
                   OptimizationRemarkEmitter *ORE)
      : AA(AA), AC(AC), DT(DT), LI(LI), SE(SE), TTI(TTI), ORE(ORE) {}

  bool run();

private:
  bool runOnLoop(Loop *L);

  Value *getCanonicalishSizeVariable(Loop *L, PHINode *PHI) const;
  Value *
  getLoopIterationNumber(Loop *L,
                         SmallPtrSet<Instruction *, 4> &LoopAuxIndPHINodes,
                         ValueMap<PHINode *, Value *> &AuxIndBounds);
  /// If prefetch instruction is not inserted, need to clean iteration
  /// instructions in the preheader.
  void cleanLoopIterationNumber(Value *NumIterations);
  /// Returns whether the auxiliary induction variable can generate bound.
  /// If it can, add PHI to LoopAuxIndPHINodes
  bool canGetAuxIndVarBound(Loop *L, PHINode *PHI,
                            SmallPtrSet<Instruction *, 4> &LoopAuxIndPHINodes);

  /// Generate bound for the auxiliary induction variable at the
  /// preheader and add it to AuxIndBounds.
  /// Returns whether the bound was successfully generated.
  bool getAuxIndVarBound(Loop *L, PHINode *PHI, Value *NumIterations,
                         ValueMap<PHINode *, Value *> &AuxIndBounds);

  bool insertPrefetcherInOuterloopForIndirectLoad(
      Loop *L, unsigned Idx, Value *NumIterations,
      SmallVector<Instruction *, 4> &CandidateMemoryLoads,
      SmallSetVector<Instruction *, 8> &DependentInsts,
      ValueMap<PHINode *, Value *> &AuxIndBounds,
      SmallVectorImpl<DenseMap<Value *, Value *>> &Transforms,
      unsigned ItersAhead);

  bool insertPrefetcherForIndirectLoad(
      Loop *L, unsigned Idx, Value *NumIterations,
      SmallVector<Instruction *, 4> &CandidateMemoryLoads,
      SmallSetVector<Instruction *, 8> &DependentInsts,
      ValueMap<PHINode *, Value *> &AuxIndBounds,
      SmallVectorImpl<DenseMap<Value *, Value *>> &Transforms,
      unsigned ItersAhead);

  bool findCandidateMemoryLoads(
      Instruction *I, SmallSetVector<Instruction *, 8> &InstList,
      SmallPtrSet<Instruction *, 8> &InstSet,
      SmallVector<Instruction *, 4> &CandidateMemoryLoads,
      std::vector<SmallSetVector<Instruction *, 8>> &DependentInstList,
      SmallPtrSet<Instruction *, 4> LoopAuxIndPHINodes,
      bool PrefetchInOuterLoop, Loop *L);

  /// Helper function to determine whether the given load is in
  /// CandidateMemoryLoads. If yes, add the candidate's depending inst to the
  /// list
  bool isLoadInCandidateMemoryLoads(
      LoadInst *LoadI, SmallSetVector<Instruction *, 8> &InstList,
      SmallPtrSet<Instruction *, 8> &InstSet,
      SmallVector<Instruction *, 4> &CandidateMemoryLoads,
      std::vector<SmallSetVector<Instruction *, 8>> &DependentInstList);

  /// Returns whether the given loop can do indirect prefetch and should be
  /// processed to insert prefetches for indirect loads.
  bool canDoIndirectPrefetch(Loop *L);

  bool isCrcHashDataAccess(Instruction *I, Instruction *PrefetchingLoad);

  /// Check if the stride of the accesses is large enough to
  /// warrant a prefetch.
  bool isStrideLargeEnough(const SCEVAddRecExpr *AR, unsigned TargetMinStride);

  unsigned getMinPrefetchStride(unsigned NumMemAccesses,
                                unsigned NumStridedMemAccesses,
                                unsigned NumPrefetches,
                                bool HasCall) {
    if (MinPrefetchStride.getNumOccurrences() > 0)
      return MinPrefetchStride;
    return TTI->getMinPrefetchStride(NumMemAccesses, NumStridedMemAccesses,
                                     NumPrefetches, HasCall);
  }

  unsigned getPrefetchDistance() {
    if (PrefetchDistance.getNumOccurrences() > 0)
      return PrefetchDistance;
    return TTI->getPrefetchDistance();
  }

  unsigned getMaxPrefetchIterationsAhead() {
    if (MaxPrefetchIterationsAhead.getNumOccurrences() > 0)
      return MaxPrefetchIterationsAhead;
    return TTI->getMaxPrefetchIterationsAhead();
  }

  bool doPrefetchWrites() {
    if (PrefetchWrites.getNumOccurrences() > 0)
      return PrefetchWrites;
    return TTI->enableWritePrefetching();
  }

  AliasAnalysis *AA;
  AssumptionCache *AC;
  DominatorTree *DT;
  LoopInfo *LI;
  ScalarEvolution *SE;
  const TargetTransformInfo *TTI;
  OptimizationRemarkEmitter *ORE;
};

/// Legacy class for inserting loop data prefetches.
class LoopDataPrefetchLegacyPass : public FunctionPass {
public:
  static char ID; // Pass ID, replacement for typeid
  LoopDataPrefetchLegacyPass() : FunctionPass(ID) {
    initializeLoopDataPrefetchLegacyPassPass(*PassRegistry::getPassRegistry());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<AAResultsWrapperPass>();
    AU.addRequired<AssumptionCacheTracker>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequiredID(LoopSimplifyID);
    AU.addRequired<OptimizationRemarkEmitterWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<TargetTransformInfoWrapperPass>();
  }

  bool runOnFunction(Function &F) override;
  };
}

char LoopDataPrefetchLegacyPass::ID = 0;
INITIALIZE_PASS_BEGIN(LoopDataPrefetchLegacyPass, "loop-data-prefetch",
                      "Loop Data Prefetch", false, false)
INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopSimplify)
INITIALIZE_PASS_DEPENDENCY(OptimizationRemarkEmitterWrapperPass)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
INITIALIZE_PASS_END(LoopDataPrefetchLegacyPass, "loop-data-prefetch",
                    "Loop Data Prefetch", false, false)

FunctionPass *llvm::createLoopDataPrefetchPass() {
  return new LoopDataPrefetchLegacyPass();
}

bool LoopDataPrefetch::isStrideLargeEnough(const SCEVAddRecExpr *AR,
                                           unsigned TargetMinStride) {
  // No need to check if any stride goes.
  if (TargetMinStride <= 1)
    return true;

  const auto *ConstStride = dyn_cast<SCEVConstant>(AR->getStepRecurrence(*SE));
  // If MinStride is set, don't prefetch unless we can ensure that stride is
  // larger.
  if (!ConstStride)
    return false;

  unsigned AbsStride = std::abs(ConstStride->getAPInt().getSExtValue());
  return TargetMinStride <= AbsStride;
}

/// Use the induction variable to generate value represeting the total num of
/// iterations for the loop in the preheader.
Value *LoopDataPrefetch::getLoopIterationNumber(
    Loop *L, SmallPtrSet<Instruction *, 4> &LoopAuxIndPHINodes,
    ValueMap<PHINode *, Value *> &AuxIndBounds) {
  Value *LoopBoundValue;
  Value *LoopStepValue;
  Value *LoopStartValue;
  Value *NumIterations;

  // Use induction variable to derive number of iterations for the loop which
  // will be used to calculate the upper bound for other auxiliary induction
  // variables.
  PHINode *PHI = L->getInductionVariable(*SE);
  if (PHI == nullptr)
    return nullptr;

  auto LoopLB = L->getBounds(*SE);
  if (!LoopLB)
    return nullptr;

  LoopStartValue = &(LoopLB->getInitialIVValue());
  LoopStepValue = LoopLB->getStepValue();
  LoopBoundValue = &(LoopLB->getFinalIVValue());

  if (LoopStartValue == nullptr || LoopStepValue == nullptr ||
      LoopBoundValue == nullptr)
    return nullptr;

  // Step should be constant.
  if (!isa<SCEVConstant>(SE->getSCEV(LoopStepValue)))
    return nullptr;

  // Make sure each of them is invariant so we can use them in the preheader.
  if (!L->isLoopInvariant(LoopBoundValue) ||
      !L->isLoopInvariant(LoopStepValue) || !L->isLoopInvariant(LoopStartValue))
    return nullptr;

  // Generate instruction that calculated the total number of iterations of the
  // loop in the preheader.
  IRBuilder<> Builder(L->getLoopPreheader()->getTerminator());
  Value *Range = Builder.CreateSub(LoopBoundValue, LoopStartValue);
  NumIterations = Builder.CreateSDiv(Range, LoopStepValue);

  LoopAuxIndPHINodes.insert(PHI);
  Value *Bound = nullptr;
  // If the step is positive, the upper bound isn't included, i.e. accessing
  // [bound] is not legal, so subtract the bound by LoopStepValue to prevent out
  // of bounds memory access.
  if (SE->isKnownNegative(SE->getSCEV(LoopStepValue)))
    Bound = LoopBoundValue;
  else
    Bound = Builder.CreateSub(LoopBoundValue, LoopStepValue);
  AuxIndBounds.insert(std::pair<PHINode *, Value *>(PHI, Bound));
  return NumIterations;
}

/// If prefetch instruction is not inserted. Need to clean iteration instruction
/// in the preheader.
void LoopDataPrefetch::cleanLoopIterationNumber(Value *NumIterations) {
  auto *IDiv = dyn_cast<Instruction>(NumIterations);
  if (IDiv != nullptr && IDiv->getOpcode() == Instruction::SDiv &&
      IDiv->use_empty()) {
    auto *IRange = dyn_cast<Instruction>(IDiv->getOperand(0));
    IDiv->eraseFromParent();
    if (IRange != nullptr && IRange->getOpcode() == Instruction::Sub &&
	IRange->use_empty()) {
      IRange->eraseFromParent();
    }
  }
}

/// Returns whether the auxiliary induction variable can generate bound.
/// If it can genearte a bound, add PHI to LoopAuxIndPHINodes
bool LoopDataPrefetch::canGetAuxIndVarBound(
    Loop *L, PHINode *PHI, SmallPtrSet<Instruction *, 4> &LoopAuxIndPHINodes) {
  Value *AuxIndVarStartValue =
      PHI->getIncomingValueForBlock(L->getLoopPreheader());
  if (AuxIndVarStartValue == nullptr)
    return false;

  const SCEV *LSCEV = SE->getSCEV(PHI);
  const SCEVAddRecExpr *LSCEVAddRec = dyn_cast<SCEVAddRecExpr>(LSCEV);

  if (LSCEVAddRec == nullptr)
    return false;

  // Currently, we only support constant steps.
  if (dyn_cast<SCEVConstant>(LSCEVAddRec->getStepRecurrence(*SE))) {
    InductionDescriptor IndDesc;
    if (!InductionDescriptor::isInductionPHI(PHI, L, SE, IndDesc))
      return false;

    if (IndDesc.getInductionOpcode() != Instruction::Add &&
        IndDesc.getInductionOpcode() != Instruction::Sub &&
        IndDesc.getKind() != InductionDescriptor::IK_PtrInduction)
      return false;

    LoopAuxIndPHINodes.insert(PHI);

    return true;
  }
  return false;
}

/// Generate bound for the auxiliary induction variable at the preheader and add
/// it to AuxIndBounds. Returns whether the bound was successfully generated.
bool LoopDataPrefetch::getAuxIndVarBound(
    Loop *L, PHINode *PHI, Value *NumIterations,
    ValueMap<PHINode *, Value *> &AuxIndBounds) {
  IRBuilder<> Builder(L->getLoopPreheader()->getTerminator());
  Value *AuxIndVarStartValue =
      PHI->getIncomingValueForBlock(L->getLoopPreheader());
  if (AuxIndVarStartValue == nullptr)
    return false;

  const SCEV *LSCEV = SE->getSCEV(PHI);
  const SCEVAddRecExpr *LSCEVAddRec = dyn_cast<SCEVAddRecExpr>(LSCEV);

  // Currently, we only support constant steps.
  if (const SCEVConstant *ConstPtrDiff =
          dyn_cast<SCEVConstant>(LSCEVAddRec->getStepRecurrence(*SE))) {
    Value *AuxIndVarBound;
    InductionDescriptor IndDesc;
    if (!InductionDescriptor::isInductionPHI(PHI, L, SE, IndDesc))
      return false;

    // Calculate the upper bound for the auxiliary induction variable.
    Value *CastedNumIterations =
        Builder.CreateSExtOrTrunc(NumIterations, ConstPtrDiff->getType());

    // Subtract one from CastedNumIterations as we want the bound to be in
    // bounds. If there are N iterations, the first iteration will access the
    // array at offset 0. On the N-th iteration, it will access the array at
    // offset N-1, not N.
    CastedNumIterations = Builder.CreateSub(
        CastedNumIterations, ConstantInt::get(ConstPtrDiff->getType(), 1));
    // Teh induction operator is add / sub
    if (IndDesc.getInductionOpcode() == Instruction::Add ||
        IndDesc.getInductionOpcode() == Instruction::Sub) {
      Value *Range =
          Builder.CreateMul(ConstPtrDiff->getValue(), CastedNumIterations);
      AuxIndVarBound = Builder.CreateAdd(Range, AuxIndVarStartValue);
    } else if (IndDesc.getKind() == InductionDescriptor::IK_PtrInduction) {
      // The induction variable is a pointer
      int64_t StepSize = ConstPtrDiff->getAPInt().getSExtValue();
      if (SE->isKnownNegative(ConstPtrDiff)) {
        StepSize = -StepSize;
        CastedNumIterations = Builder.CreateMul(
            ConstantInt::getSigned(ConstPtrDiff->getType(), -1),
            CastedNumIterations);
      }
      Type *GEPType = getPtrTypefromPHI(PHI, StepSize);
      AuxIndVarBound = Builder.CreateInBoundsGEP(GEPType, AuxIndVarStartValue,
                                                 CastedNumIterations);
    } else
      return false;

    LLVM_DEBUG(dbgs() << "Added "
                      << (isa<SCEVConstant>(SE->getSCEV(AuxIndVarBound))
                              ? "Constant "
                              : "")
                      << "AuxIndVarBound " << *AuxIndVarBound
                      << " for AuxIndVar:" << *PHI << "\n");
    AuxIndBounds.insert(std::pair<PHINode *, Value *>(PHI, AuxIndVarBound));

    return true;
  }
  return false;
}

// Helper function to calculate the step for a given loop
static uint64_t getStep(PHINode *PN, ScalarEvolution *SE) {
  // Get the constant step for the induction phi so we can use it to calculate
  // how much we should increase the induction for prefetching.
  uint64_t Step = 0;
  const SCEV *LSCEV = SE->getSCEV(PN);
  const SCEVAddRecExpr *LSCEVAddRec = dyn_cast<SCEVAddRecExpr>(LSCEV);

  if (LSCEVAddRec == nullptr)
    return Step;

  if (const SCEVConstant *ConstPtrDiff =
          dyn_cast<SCEVConstant>(LSCEVAddRec->getStepRecurrence(*SE))) {
    Step = ConstPtrDiff->getAPInt().getZExtValue();
  }
  return Step;
}

// Helper function to determine if the loop step is positive
static bool isPositiveStep(PHINode *PN, ScalarEvolution *SE) {
  bool PositiveStep = true;
  const SCEV *LSCEV = SE->getSCEV(PN);
  const SCEVAddRecExpr *LSCEVAddRec = dyn_cast<SCEVAddRecExpr>(LSCEV);
  if (const SCEVConstant *ConstPtrDiff =
          dyn_cast<SCEVConstant>(LSCEVAddRec->getStepRecurrence(*SE))) {
    if (SE->isKnownNegative(ConstPtrDiff)) {
      PositiveStep = false;
    }
  }
  return PositiveStep;
}

// Helper function to calculate the step type of a PHI node. If the PHI node is
// not a pointer type, get the type PHI Node itself. Otherwise, get the integer
// type of the PHI's step/offset value.
static Type *getStepTypeFromPHINode(PHINode *PN, ScalarEvolution *SE) {
  // Get the constant step for the induction phi so we can use it to calculate
  // how much we should increase the induction for prefetching.
  Type *T = PN->getType();
  if (!T->isPointerTy())
    return T;

  const SCEV *LSCEV = SE->getSCEV(PN);
  const SCEVAddRecExpr *LSCEVAddRec = dyn_cast<SCEVAddRecExpr>(LSCEV);
  if (const SCEVConstant *ConstPtrDiff =
          dyn_cast<SCEVConstant>(LSCEVAddRec->getStepRecurrence(*SE)))
    return ConstPtrDiff->getType();

  return T;
}

/// This function will take an instr list that contains indirect loads and
/// transform them into prefetchers. E.g. Transform following indirect load
/// A[B[i]]:
///   phi indvar [0] [bound]
///   idxB = gep *B, indvar
///   offsetA = load * idxB
///   idxA = gep *A, offsetA
///   valueA = load *idxA
/// To indirect load with prefetchers N iteration ahead:
///   phi indvar [0] [bound]
///   offsetN = add indvar, N
///   offset2N = add indvar, 2N
///   compare = icmp offsetN, bound
///   offsetN = select compare, offsetN, bound
///   preIdxN = gep *B, offsetN
///   preIdx2N = get *B, offset2N
///   call prefetch(preIdx2N)
///   preOffsetA = load preIdxN
///   preIdxA = gep *A, preOffsetA
///   call prefetch(preIdxA)
///   idxB = gep *B, indvar
///   offsetA = load *idxB
///   idxA = gep *A, offsetA
///   valueA = load *idxA
bool LoopDataPrefetch::insertPrefetcherForIndirectLoad(
    Loop *L, unsigned Idx, Value *NumIterations,
    SmallVector<Instruction *, 4> &CandidateMemoryLoads,
    SmallSetVector<Instruction *, 8> &DependentInsts,
    ValueMap<PHINode *, Value *> &AuxIndBounds,
    SmallVectorImpl<DenseMap<Value *, Value *>> &Transforms,
    unsigned ItersAhead) {
  bool PositiveStep = true;
  Instruction *TargetIndirectLoad = CandidateMemoryLoads[Idx];
  IRBuilder<> Builder(TargetIndirectLoad);
  Module *M = TargetIndirectLoad->getModule();
  Type *I32Ty = Type::getInt32Ty(TargetIndirectLoad->getParent()->getContext());

  if (RandomAccessPrefetchOnly) {
    bool isRandomAccess = false;
    for (auto *I : DependentInsts) {
      if (isCrcHashDataAccess(I, TargetIndirectLoad)) {
        isRandomAccess = true;
        break;
      }
    }
    if (!isRandomAccess)
      return false;
  }

  LLVM_DEBUG(dbgs() << "Inserting indirect prefetchers for\t"
                    << *TargetIndirectLoad << "\twith " << DependentInsts.size()
                    << " dependent instructions\n");

  // Keep track of the number of prefetches left to process among the
  // DependentInst List. We assume that for given indirectLevel N, we will have
  // N prefetches to do, unless we are skipping intermediate loads, then we are
  // only doing 1 prefetch.
  size_t NumPrefetchesLeft = SkipIntermediate ? 1 : IndirectionLevel;
  int64_t Step;
  while (!DependentInsts.empty()) {
    Instruction *DependentInst = DependentInsts.pop_back_val();
    Instruction *Inst = dyn_cast<Instruction>(DependentInst);

    switch (Inst->getOpcode()) {
    case Instruction::PHI: {
      // Get the constant step for the induction phi so we can use it to
      // calculate how much we should increase the induction for prefetching.
      PHINode *PN = dyn_cast<PHINode>(Inst);
      Step = getStep(PN, SE);
      PositiveStep = isPositiveStep(PN, SE);
      Type *InstType = getStepTypeFromPHINode(PN, SE);
      if (!PositiveStep)
        Step = -Step;

      // Make sure phi node is i64 or i32.
      if (!InstType->isIntegerTy(64) && !InstType->isIntegerTy(32))
        return false;

      // Create the bound for this PHI if needed:
      if (!AuxIndBounds.count(PN))
        getAuxIndVarBound(L, PN, NumIterations, AuxIndBounds);

      //  We create values based on the induction variable so we can use it to
      //  generate prefetcher later on. The first value (indvar + IterationAhead
      //  * step) will be used for the load of prefetched address and it must
      //  not exceeding the bound. The second value (indvar + 2 * IterationAhead
      //  * step) will be used to generate prefether for the load of address.
      //  The subsequent values are generated in a similar fashion to generate
      //  prefetchers for offset of all dependent loads.

      //  Insert the new instruction after all PHI node.
      auto InsertionPoint = Inst;
      if (auto FirstNonPHI = Inst->getParent()->getFirstNonPHI())
        InsertionPoint = FirstNonPHI->getPrevNode();

      for (size_t i = 0; i < NumPrefetchesLeft; i++) {
        if (i > 0 && SkipIntermediate)
          break;

        if (Transforms.size() < i + 1) {
          Transforms.push_back(DenseMap<Value *, Value *>());
        } else if (Transforms[i].count(Inst))
          continue;

        // Create the new operation for the target load
        Value *NewOp = nullptr;
        if (Inst->getType()->isPointerTy()) {
          Type *GEPType = getPtrTypefromPHI(PN, Step);
          int64_t Offset =
              PrefetchIterationsAhead ? PrefetchIterationsAhead : ItersAhead;
          if (!PositiveStep)
            Offset = -Offset;
          // Do not need to calculate Offset * Step as it is calculated
          // implicitly within the GEP instruction
          NewOp = Builder.CreateInBoundsGEP(
              GEPType, Inst,
              ConstantInt::getSigned(InstType, (i + 1) * Offset));
        } else {
          // FullStep is the initial offset for the new value, taking into
          // account, both Step and the number of iterations ahead to prefetch.
          // If indirect prefetch iterations ahead is enabled, we directly use
          // the supplied indirect-prefetch-iters-ahead value.
          int64_t FullStep = PrefetchIterationsAhead
                                 ? PrefetchIterationsAhead * Step
                                 : ItersAhead * Step;

          Instruction::BinaryOps BiOp =
              PositiveStep ? Instruction::Add : Instruction::Sub;
          NewOp = Builder.CreateBinOp(
              BiOp, Inst,
              ConstantInt::get(Inst->getType(), (i + 1) * FullStep));
        }

        if (auto NewOpInstr = dyn_cast<Instruction>(NewOp)) {
          NewOpInstr->moveAfter(InsertionPoint);
          InsertionPoint = NewOpInstr;
        }

        // Create the new operations for the offset loads
        if (i > 0 && i == NumPrefetchesLeft - 1) {
          Transforms[i].insert(std::pair<Value *, Value *>(Inst, NewOp));
        } else {
          Value *NewCmp = Builder.CreateICmp(
              PositiveStep ? CmpInst::ICMP_SLT : CmpInst::ICMP_SGT, NewOp,
              AuxIndBounds[cast<PHINode>(Inst)]);
          Value *NewSelect =
              Builder.CreateSelect(NewCmp, NewOp, AuxIndBounds[PN]);
          Transforms[i].insert(std::pair<Value *, Value *>(Inst, NewSelect));

          if (auto NewCmpInstr = dyn_cast<Instruction>(NewCmp)) {
            NewCmpInstr->moveAfter(InsertionPoint);
            InsertionPoint = NewCmpInstr;
          }

          if (auto NewSelectInstr = dyn_cast<Instruction>(NewSelect)) {
            NewSelectInstr->moveAfter(InsertionPoint);
            InsertionPoint = NewSelectInstr;
          }
        }
      }
      break;
    }
    case Instruction::Load: {
      LoadInst *LoadI = dyn_cast<LoadInst>(Inst);
      Value *LoadPtr = LoadI->getPointerOperand();
      if (!SkipIntermediate)
        NumPrefetchesLeft--;

      auto GeneratePrefetcher = [&](llvm::Value *PrefetchPtr) {
        Function *PrefetchFunc = Intrinsic::getDeclaration(
            M, Intrinsic::prefetch, LoadPtr->getType());
        Value *PrefetchArg[] = {PrefetchPtr, ConstantInt::get(I32Ty, 0),
                                ConstantInt::get(I32Ty, 3),
                                ConstantInt::get(I32Ty, 1)};
        CallInst *PrefetchCall = CallInst::Create(PrefetchFunc, PrefetchArg);
        return PrefetchCall;
      };

      if (!DependentInsts.empty()) {
        // For any intermediate (not last) load, we generate a load for all the
        // offset at min(indvar+N*IterationsAhead*step, bound)] for each N up to
        // NumPrefetchesLeft - 1, and generate a prefetcher at
        // (indvar+(N+1)*IterationAhead*step) for the offset load.
        Instruction *PrefetchOffsetLoad = nullptr;
        for (size_t i = 0; i < NumPrefetchesLeft; i++) {
          if (Transforms[i].count(LoadI))
            continue;
          PrefetchOffsetLoad = LoadI->clone();
          Builder.Insert(PrefetchOffsetLoad);
          for (size_t i = 0; i < NumPrefetchesLeft; i++) {
            if (Transforms[i].count(LoadI))
              continue;
            PrefetchOffsetLoad = LoadI->clone();
            Builder.Insert(PrefetchOffsetLoad);
            PrefetchOffsetLoad->moveAfter(LoadI);
            PrefetchOffsetLoad->replaceUsesOfWith(LoadPtr,
                                                  Transforms[i][LoadPtr]);

            Transforms[i].insert(
                std::pair<Value *, Value *>(LoadI, PrefetchOffsetLoad));
          }
        }

        if (SkipIntermediate)
          break;

        // Create a prefetcher for the offset laod.
        if (PrefetchOffsetLoad) {
          CallInst *PrefetchCall =
              GeneratePrefetcher(Transforms[NumPrefetchesLeft][LoadPtr]);
          PrefetchCall->insertAfter(PrefetchOffsetLoad);
          NumIndPrefetches++;
        }
      } else {
        CallInst *PrefetchCall = GeneratePrefetcher(Transforms[0][LoadPtr]);
        PrefetchCall->insertAfter(LoadI);
        NumIndPrefetches++;
      }
      break;
    }
    default: {
      // For other types of instructions, we make a clone of the instruction and
      // repalce operands that we already transformed before.
      for (size_t j = 0; j < NumPrefetchesLeft; j++) {
        if (j >= Transforms.size() || Transforms[j].count(Inst))
          continue;
        Instruction *TransformedInst = Inst->clone();
        Builder.Insert(TransformedInst);
        TransformedInst->moveAfter(Inst);
        for (unsigned i = 0; i < TransformedInst->getNumOperands(); i++) {
          Value *Operand = TransformedInst->getOperand(i);
          if (Transforms[j].count(Operand))
            TransformedInst->replaceUsesOfWith(Operand, Transforms[j][Operand]);
        }

        Transforms[j].insert(
            std::pair<Value *, Value *>(Inst, TransformedInst));
      }
      break;
    }
    }
  }
  return true;
}

/// Find the indirect load that depends on the auxiliary induction variable and
/// construct an instr list that contains loop variant instruction from the
/// target load to the candidate phi instr.
bool LoopDataPrefetch::findCandidateMemoryLoads(
    Instruction *I, SmallSetVector<Instruction *, 8> &InstList,
    SmallPtrSet<Instruction *, 8> &InstSet,
    SmallVector<Instruction *, 4> &CandidateMemoryLoads,
    std::vector<SmallSetVector<Instruction *, 8>> &DependentInstList,
    SmallPtrSet<Instruction *, 4> LoopAuxIndPHINodes, bool PrefetchInOuterLoop,
    Loop *L) {
  bool ret = false;

  for (Use &U : I->operands()) {
    // If value is loop invariant, just continue
    if (PrefetchInOuterLoop) {
      if (L->getParentLoop()->isLoopInvariant(U.get()))
        continue;
    } else if (LI->getLoopFor(I->getParent())->isLoopInvariant(U.get()))
      continue;

    Instruction *OperandInst = dyn_cast<Instruction>(U.get());
    if (OperandInst != nullptr) {
      switch (OperandInst->getOpcode()) {
      case Instruction::Load: {
        // Check if the load instruction that it depends on is already in the
        // candidate. If yes, add the canddiate's depending instr to the list.
        // If not, the load instruction it depends on is invalid.
        LoadInst *LoadI = dyn_cast<LoadInst>(OperandInst);
        if (isLoadInCandidateMemoryLoads(LoadI, InstList, InstSet,
                                         CandidateMemoryLoads,
                                         DependentInstList)) {
          // We do not return early in case there are other auxiliary induction
          // variables to check.
          ret = true;
        }
        break;
      }
      case Instruction::PHI: {
        // Check if PHI is the loop auxiliary induction PHI. If yes, found a
        // valid load dependent on loop auxiliary induction variable. If not,
        // invalid candidate.
        PHINode *PhiInst = dyn_cast<PHINode>(OperandInst);
        if (LoopAuxIndPHINodes.contains(PhiInst)) {
          // In order to prevent the size of SmallVector from going out of
          // bounds for large cases, only the last access of the element is
          // retained. Update the position of OperandInst in the InstList.
          if (InstList.count(OperandInst))
            InstList.remove(OperandInst);
          InstList.insert(OperandInst);
          return true;
        }
        break;
      }
      case Instruction::Call: {
        if (PrefetchInOuterLoop || RandomAccessPrefetchOnly) {
          if (OperandInst->mayReadOrWriteMemory())
            return false;
          CallInst *Call = dyn_cast<CallInst>(OperandInst);
          if (!Call->doesNotThrow())
            return false;

          // Use DFS to search though the operands.
          InstList.insert(OperandInst);
          if (findCandidateMemoryLoads(OperandInst, InstList, InstSet,
                                       CandidateMemoryLoads, DependentInstList,
                                       LoopAuxIndPHINodes, PrefetchInOuterLoop,
                                       L)) {
            // We do not return early in case there are other auxiliary
            // induction variable to check
            ret = true;
          } else {
            // If the Operand isn't dependent on an auxiliary induction
            // variable, remove any instructions added to DependentInstList from
            // this operand
            if (InstList.count(OperandInst))
              InstList.remove(OperandInst);
            InstList.insert(OperandInst);
            return false;
          }
          break;
        } else {
          // We currently can not handle case where indirect load depends on
          // other functions yet.
          return false;
        }
      }
      case Instruction::Invoke: {
        // We currently can not handle case where indirect load depends on other
        // functions yet.
        return false;
      }
      default: {
        // Use DFS to search though the operands.
        if (InstList.count(OperandInst))
          InstList.remove(OperandInst);
        InstList.insert(OperandInst);
        if (findCandidateMemoryLoads(OperandInst, InstList, InstSet,
                                     CandidateMemoryLoads, DependentInstList,
                                     LoopAuxIndPHINodes, PrefetchInOuterLoop,
                                     L)) {
          // We do not return early in case there are other auxiliary induction
          // variables to check
          ret = true;
        } else {
          // If the operand isn't dependent on an auxiliary induction variable,
          // remove any instructions added to DependentInstList from this
          // operand
          InstList.remove(OperandInst);
        }
      }
      }
    }
  }
  return ret;
}

/// Helper function to determine whether the given load is in
/// CandidateMemoryLoads. If Yes, add the candidate's depending instr to the
/// list.
bool LoopDataPrefetch::isLoadInCandidateMemoryLoads(
    LoadInst *LoadI, SmallSetVector<Instruction *, 8> &InstList,
    SmallPtrSet<Instruction *, 8> &InstSet,
    SmallVector<Instruction *, 4> &CandidateMemoryLoads,
    std::vector<SmallSetVector<Instruction *, 8>> &DependentInstList) {
  size_t CandidateLoadIndex = 0;
  for (auto CandidateMemoryLoad : CandidateMemoryLoads) {
    if (LoadI == CandidateMemoryLoad)
      break;
    CandidateLoadIndex++;
  }

  if (CandidateLoadIndex >= CandidateMemoryLoads.size() || InstSet.count(LoadI))
    return false;

  for (auto CandidateInst : DependentInstList[CandidateLoadIndex]) {
    if (InstList.count(CandidateInst))
      InstList.remove(CandidateInst);
    InstList.insert(CandidateInst);
    InstSet.insert(CandidateInst);
  }
  return true;
}

/// Returns whether the given loop should be processed to insert prefetches for
/// indirect loads.
bool LoopDataPrefetch::canDoIndirectPrefetch(Loop *L) {
  // Support inner most loops in a simple form. However, the parent of inner
  // loop will be processed as well in the case of nested loops. If
  // indirectLevel is low, only allow one block loop, otherwise, allow up to 5
  // under certain conditions.
  if (!L->isInnermost() || !L->getLoopPreheader() ||
      (IndirectionLevel <= 3 && L->getNumBlocks() != 1) ||
      (IndirectionLevel > 3 && L->getNumBlocks() == 1) || L->getNumBlocks() > 5)
    return false;
  return true;
}

/// Check if the load depends on Crc Hash functions.
bool LoopDataPrefetch::isCrcHashDataAccess(Instruction *I,
                                           Instruction *PrefetchingLoad) {
  if (llvm::IntrinsicInst *II = dyn_cast<llvm::IntrinsicInst>(I))
    // If CRC functions are used for offset calculation then offset will be
    // random. To avoid cache misses, data prefetch is needed.
    switch (II->getIntrinsicID()) {
    case Intrinsic::aarch64_crc32b:
    case Intrinsic::aarch64_crc32cb:
    case Intrinsic::aarch64_crc32h:
    case Intrinsic::aarch64_crc32ch:
    case Intrinsic::aarch64_crc32w:
    case Intrinsic::aarch64_crc32cw:
    case Intrinsic::aarch64_crc32x:
    case Intrinsic::aarch64_crc32cx: {
      // Checking Candidate load is incremented by 1.
      if (auto *LI = dyn_cast<LoadInst>(PrefetchingLoad)) {
        if (auto *GEPI = dyn_cast<GetElementPtrInst>(LI->getPointerOperand())) {
          // The data access will be consecutive, if the gep has one indices.
          if (GEPI->getNumOperands() > 2)
            return false;
          auto *PtrIndices = dyn_cast<Instruction>(GEPI->getOperand(1));
          if (!PtrIndices || isa<GlobalValue>(PtrIndices))
            return true;
          for (auto &U : PtrIndices->uses())
            if (auto *PN = dyn_cast<PHINode>(U.getUser()))
              if (getStep(PN, SE) <= 1)
                return true;
        }
      }
      break;
    }
    }
  return false;
}

/// Checks the indirect loads inside the inner loop and
/// it is derived from induction variable of outer loop then,
/// insert the prefetch instruction in outer loop.
/// It maintains the same CFG structure of inner loop and
/// clone it in the outerloop. Insert the prefetch for
/// the last indirect load, not for the intermediate loads.
bool LoopDataPrefetch::insertPrefetcherInOuterloopForIndirectLoad(
    Loop *L, unsigned Idx, Value *NumIterations,
    SmallVector<Instruction *, 4> &CandidateMemoryLoads,
    SmallSetVector<Instruction *, 8> &DependentInsts,
    ValueMap<PHINode *, Value *> &AuxIndBounds,
    SmallVectorImpl<DenseMap<Value *, Value *>> &Transforms,
    unsigned ItersAhead) {
  Instruction *TargetIndirectLoad = CandidateMemoryLoads[Idx];
  IRBuilder<> Builder(TargetIndirectLoad);
  Module *M = TargetIndirectLoad->getModule();
  auto *ParentLoop = L->getParentLoop();

  if (!ParentLoop)
    return false;

  SmallVector<BasicBlock *, 9> ExitBlocks;
  L->getUniqueExitBlocks(ExitBlocks);
  bool HasCatchSwitch = llvm::any_of(ExitBlocks, [](BasicBlock *Exit) {
    return isa<CatchSwitchInst>(Exit->getTerminator());
  });
  if (HasCatchSwitch)
    return false;

  SmallVector<BasicBlock *, 8> NewBBlocks;
  SmallVector<Instruction *, 8> AllDependentInsts;
  SmallPtrSet<Instruction *, 4> Visited;
  SmallPtrSet<Instruction *, 4> IndirectLoadDependents;
  SmallPtrSet<Instruction *, 4> BranchInsts;
  SmallPtrSet<CallInst *, 4> InsertedPrefetchCalls;
  DenseMap<BasicBlock *, BasicBlock *> BBTransforms;
  DenseMap<BasicBlock *, int> BBPostNumbers;
  BasicBlock *NewRootBB = nullptr;
  DomTreeUpdater DTU(DT, DomTreeUpdater::UpdateStrategy::Lazy);

  if (!isa<PHINode>(DependentInsts[DependentInsts.size() - 1])) {
    return false;
  } else {
    if (auto *PN =
            dyn_cast<PHINode>(DependentInsts[DependentInsts.size() - 1])) {
      if (!ParentLoop->contains(PN)) {
        return false;
      }
      if (!getStep(PN, SE))
        return false;
      if (isa<PointerType>(PN->getType()))
        return false;
    }
  }

  ExitBlocks.clear();
  ParentLoop->getUniqueExitBlocks(ExitBlocks);
  if (HasCatchSwitch)
    return false;

  Instruction *CandidateLoad = DependentInsts[0];
  BasicBlock *LoopPreheader = L->getLoopPreheader();

  // Only consider crc hashed random data accesses.
  bool isRandomAccess = false;
  for (auto *I : DependentInsts) {
    IndirectLoadDependents.insert(I);
    Visited.insert(I);
    isRandomAccess |= isCrcHashDataAccess(I, CandidateLoad);
  }
  if (!isRandomAccess)
    return false;

  if (!LoopPreheader || !ParentLoop->getLoopPreheader())
    return false;

  if (LoopPreheader->getTerminator() == nullptr ||
      !isa<BranchInst>(LoopPreheader->getTerminator()))
    return false;
  if (Visited.insert(LoopPreheader->getTerminator()).second)
    DependentInsts.insert(LoopPreheader->getTerminator());

  // Start from target indirect load block, get the list of predecessor blocks
  // till loop preheader. And we assign each block with post order number with
  // which we can sort.
  SmallSetVector<BasicBlock *, 8> BBPredecessors;
  BBPredecessors.insert(CandidateLoad->getParent());
  BBPostNumbers.insert({CandidateLoad->getParent(), 0});
  while (BBPredecessors.size()) {
    BasicBlock *BBPred = BBPredecessors[0];
    BBPredecessors.remove(BBPred);
    int Depth = BBPostNumbers[BBPred];
    // Check all predecessors and add their branch instr into dependent list
    for (BasicBlock *Predecessor : predecessors(BBPred)) {
      if (LoopPreheader != Predecessor && !DT->dominates(BBPred, Predecessor)) {
        if (BBPostNumbers.end() == BBPostNumbers.find(Predecessor)) {
          BBPostNumbers.insert({Predecessor, Depth - 1});
          BBPredecessors.insert(Predecessor);
          // Check each terminator is a branch instr.
          if (Predecessor->getTerminator() == nullptr ||
              !isa<BranchInst>(Predecessor->getTerminator()))
            return false;
          // Add branch instruction as dependent instr.
          if (Visited.insert(Predecessor->getTerminator()).second)
            DependentInsts.insert(Predecessor->getTerminator());
        }
      }
    }
  }

  // Loop preheader is last depend block.
  BBPostNumbers.insert({LoopPreheader, -1 * BBPostNumbers.size()});

  // Update DependentInsts to include instructions that branch instruction
  // depends.
  for (unsigned j = 0; j < DependentInsts.size(); j++) {
    Instruction *Inst = DependentInsts[j];
    if (Inst == nullptr)
      return false;

    if (auto *PN = dyn_cast<PHINode>(Inst)) {
      if (!IndirectLoadDependents.count(Inst)) {
        if (0 > PN->getBasicBlockIndex(LoopPreheader))
          return false;
      }
    } else if (auto *BranchInstr = dyn_cast<BranchInst>(Inst)) {
      // Add condition of branch instruction into dependent insts.
      if (BranchInstr->isConditional()) {
        auto *BranchCond = BranchInstr->getCondition();
        if (BranchCond == nullptr)
          return false;
        if (Instruction *BranchCondInst = dyn_cast<Instruction>(BranchCond))
          if (Visited.insert(BranchCondInst).second)
            DependentInsts.insert(BranchCondInst);
      } else if (BranchInstr->getSuccessor(0)->isEHPad())
        return false;
    } else if (isa<InvokeInst>(Inst)) {
      return false;
    } else {
      if (CallInst *Call = dyn_cast<CallInst>(Inst))
        if (Inst->mayReadOrWriteMemory() || !Call->doesNotThrow())
          return false;
      // Traverse instruction operands and add dependent instructions till
      // function argument, constant or value outside current loop.
      for (unsigned i = 0; i < Inst->getNumOperands(); i++) {
        Value *Operand = Inst->getOperand(i);
        if (Operand == nullptr)
          return false;
        if (isa<Argument>(Operand) || isa<Constant>(Operand))
          continue;
        if (Instruction *I = dyn_cast<Instruction>(Operand))
          if (L->contains(I) || I->getParent() == LoopPreheader)
            if (Visited.insert(I).second)
              DependentInsts.insert(I);
      }
    }
  }

  // Sort dependent instruction based on PostNumber id and instruction ordering
  // in the same block.
  SmallVector<std::tuple<Instruction *, int>, 8> SortedDependentInsts;
  DT->updateDFSNumbers();
  SortedDependentInsts.reserve(DependentInsts.size());
  for (auto I : DependentInsts) {
    auto *NodeI = DT->getNode(I->getParent());
    SortedDependentInsts.push_back({I, NodeI->getDFSNumIn()});
  }
  llvm::sort(SortedDependentInsts, [&](auto const &LHS, auto const &RHS) {
    if (get<0>(RHS)->getParent() == get<0>(LHS)->getParent())
      return get<0>(RHS)->comesBefore(get<0>(LHS));
    if (BBPostNumbers.end() == BBPostNumbers.find(get<0>(LHS)->getParent()) ||
        BBPostNumbers.end() == BBPostNumbers.find(get<0>(RHS)->getParent()))
      return get<1>(RHS) < get<1>(LHS);
    if (BBPostNumbers[get<0>(LHS)->getParent()] ==
        BBPostNumbers[get<0>(RHS)->getParent()])
      return get<1>(RHS) < get<1>(LHS);
    return BBPostNumbers[get<0>(LHS)->getParent()] >
           BBPostNumbers[get<0>(RHS)->getParent()];
  });

  // Checking all the BasicBlocks have branch instruction
  int BBDepth = 0;
  for (auto I : SortedDependentInsts) {
    if (BBDepth && get<1>(I) != BBDepth)
      if (!isa<BranchInst>(get<0>(I)) &&
          BBPostNumbers.end() != BBPostNumbers.find(get<0>(I)->getParent()))
        return false;
    BBDepth = get<1>(I);
  }

  if (!isa<LoadInst>(get<0>(SortedDependentInsts[0])))
    return false;

  if (!L->contains(get<0>(SortedDependentInsts[0])))
    return false;

  if (!isa<PHINode>(
          get<0>(SortedDependentInsts[SortedDependentInsts.size() - 1])))
    return false;
  else if (auto *PN = dyn_cast<PHINode>(
               get<0>(SortedDependentInsts[SortedDependentInsts.size() - 1])))
    if (!ParentLoop->contains(PN))
      return false;

  auto cloneInstructionWithBB = [&](llvm::Instruction *Inst,
                                    llvm::Instruction *NewInstr = nullptr) {
    Instruction *TransformedInstr = NewInstr;
    if (TransformedInstr == nullptr)
      TransformedInstr = Inst->clone();

    BasicBlock *NewBlock;
    BasicBlock *OldBlock = Inst->getParent();
    // Check if block had been created before.
    if (BBTransforms.count(OldBlock)) {
      NewBlock = BBTransforms[OldBlock];
    } else {
      NewBlock = BasicBlock::Create(OldBlock->getContext(),
                                    "prefetch." + OldBlock->getName());
      NewBlock->insertInto(OldBlock->getParent(), LoopPreheader);
      if (NewRootBB == nullptr)
        NewRootBB = NewBlock;
      if (!ParentLoop->contains(NewBlock))
        ParentLoop->addBasicBlockToLoop(NewBlock, *LI);
      BBTransforms.insert(
          std::pair<BasicBlock *, BasicBlock *>(OldBlock, NewBlock));
      NewBBlocks.push_back(NewBlock);
    }
    TransformedInstr->insertInto(NewBlock, NewBlock->end());
    if (NewInstr == nullptr) {
      for (unsigned i = 0; i < TransformedInstr->getNumOperands(); i++) {
        Value *Operand = TransformedInstr->getOperand(i);
        if (Transforms[0].count(Operand))
          TransformedInstr->replaceUsesOfWith(Operand, Transforms[0][Operand]);
      }
    }
    Transforms[0].insert(std::pair<Value *, Value *>(Inst, TransformedInstr));
    AllDependentInsts.push_back(TransformedInstr);
    return TransformedInstr;
  };

  // We create block and instructions with topdown manner, e.g. from PHI node in
  // the parent loop to target indirect load.
  bool PositiveStep = true;
  int64_t Step;
  while (!SortedDependentInsts.empty()) {
    Instruction *DependentInst = get<0>(SortedDependentInsts.pop_back_val());
    Instruction *Inst = dyn_cast<Instruction>(DependentInst);

    // For target load related instruction.
    switch (Inst->getOpcode()) {
    case Instruction::PHI: {
      // For non-root phi node, replace phi node with incoming value.
      if (!IndirectLoadDependents.count(Inst)) {
        if (Transforms[0].count(Inst))
          continue;
        auto *PN = dyn_cast<PHINode>(Inst);
        Transforms[0].insert(std::pair<Value *, Value *>(
            Inst, PN->getIncomingValueForBlock(LoopPreheader)));
        break;
      }
      // Replace root phi node with following value:
      // select((phi + step) < bound, (phi + step), bound)
      // Get the constant step for the induction phi so we can use it to
      // calculate how much we should increase the induction for prefetching
      PHINode *PN = dyn_cast<PHINode>(Inst);
      Step = getStep(PN, SE);
      PositiveStep = isPositiveStep(PN, SE);
      Type *InstType = getStepTypeFromPHINode(PN, SE);
      if (!PositiveStep)
        Step = -Step;

      // Make sure phi node is i64 or i32.
      if (!InstType->isIntegerTy(64) && !InstType->isIntegerTy(32))
        return false;

      // Create the bound for this PHI if needed:
      if (!AuxIndBounds.count(PN))
        getAuxIndVarBound(ParentLoop, PN, NumIterations, AuxIndBounds);

      // Insert the new instruction after all PHI nodes
      auto InsertionPoint = Inst;
      if (auto FirstNonPHI = Inst->getParent()->getFirstNonPHI())
        InsertionPoint = FirstNonPHI->getPrevNode();

      if (Transforms.size() < 1)
        Transforms.push_back(DenseMap<Value *, Value *>());
      else if (Transforms[0].count(Inst))
        continue;

      // FullStep is the inital offset for the new value, taking into account,
      // both Step and the number of iterations ahead to prefetch. If indirect
      // prefetch iteration ahead is enabled, we directly use the supplied
      // indirect-prefetch-iters-ahead value.
      int64_t FullStep = PrefetchIterationsAhead
                             ? PrefetchIterationsAhead * Step
                             : ItersAhead * Step;

      Instruction::BinaryOps BiOp =
          PositiveStep ? Instruction::Add : Instruction::Sub;
      auto *NewOp = Builder.CreateBinOp(
          BiOp, Inst, ConstantInt::get(Inst->getType(), FullStep));
      if (auto NewOpInstr = dyn_cast<Instruction>(NewOp)) {
        NewOpInstr->moveAfter(InsertionPoint);
        InsertionPoint = NewOpInstr;
        AllDependentInsts.push_back(NewOpInstr);
      }

      Value *NewCmp = Builder.CreateICmp(
          PositiveStep ? CmpInst::ICMP_SLT : CmpInst::ICMP_SGT, NewOp,
          AuxIndBounds[cast<PHINode>(Inst)]);
      Value *NewSelect = Builder.CreateSelect(NewCmp, NewOp, AuxIndBounds[PN]);
      Transforms[0].insert(std::pair<Value *, Value *>(Inst, NewSelect));

      if (auto NewCmpInstr = dyn_cast<Instruction>(NewCmp)) {
        NewCmpInstr->moveAfter(InsertionPoint);
        InsertionPoint = NewCmpInstr;
        AllDependentInsts.push_back(NewCmpInstr);
      }
      if (auto NewSelectInstr = dyn_cast<Instruction>(NewSelect)) {
        NewSelectInstr->moveAfter(InsertionPoint);
        InsertionPoint = NewSelectInstr;
        AllDependentInsts.push_back(NewSelectInstr);
      }
      break;
    }
    case Instruction::Load: {
      LoadInst *LoadI = dyn_cast<LoadInst>(Inst);
      Value *LoadPtr = LoadI->getPointerOperand();
      auto GeneratePrefetcher = [&](llvm::Value *PrefetchPtr) {
        Function *PrefetchFunc = Intrinsic::getDeclaration(
            M, Intrinsic::prefetch, LoadPtr->getType());
        Type *I32Ty =
            Type::getInt32Ty(CandidateLoad->getParent()->getContext());
        Value *PrefetchArg[] = {PrefetchPtr, ConstantInt::get(I32Ty, 0),
                                ConstantInt::get(I32Ty, 3),
                                ConstantInt::get(I32Ty, 1)};
        CallInst *PrefetchCall = CallInst::Create(PrefetchFunc, PrefetchArg);
        return PrefetchCall;
      };

      // We clone the intermediate load but prefetch the target load.
      if (!SortedDependentInsts.empty()) {
        if (Transforms[0].count(LoadI))
          continue;
        cloneInstructionWithBB(LoadI);
      } else {
        CallInst *PrefetchCall = GeneratePrefetcher(Transforms[0][LoadPtr]);
        cloneInstructionWithBB(LoadI, PrefetchCall);
        InsertedPrefetchCalls.insert(PrefetchCall);
      }
      break;
    }
    case Instruction::Br: {
      BranchInsts.insert(cloneInstructionWithBB(Inst));
      break;
    }
    default: {
      // For other types of instructions, we make a clone of the instruction and
      // replace operands that we already transformed before.
      if (Transforms[0].count(Inst))
        continue;
      cloneInstructionWithBB(Inst);
      break;
    }
    }
  }

  BasicBlock *EndBlock =
      BasicBlock::Create(LoopPreheader->getContext(), "prefetch.end");
  ParentLoop->addBasicBlockToLoop(EndBlock, *LI);
  EndBlock->insertInto(LoopPreheader->getParent(), LoopPreheader);

  // Create branch from prefetch call block to end block.
  for (CallInst *PrefetchCall : InsertedPrefetchCalls)
    if (!PrefetchCall->getParent()->getTerminator()) {
      AllDependentInsts.push_back(
          BranchInst::Create(EndBlock, PrefetchCall->getParent()));
    }

  // Checking all the newly created BasicBlock has Terminator instruction. If
  // not, considered as incomplete. Delete all new BasicBlocks and return.
  for (BasicBlock *BB : NewBBlocks) {
    if (BB->getTerminator() == nullptr) {
      for (unsigned j = 0; j < AllDependentInsts.size(); j++) {
        auto *I = AllDependentInsts[j];
        I->replaceAllUsesWith(UndefValue::get(I->getType()));
        I->eraseFromParent();
      }
      for (unsigned j = 0; j < NewBBlocks.size(); j++) {
        auto *DelBBlock = NewBBlocks[j];
        ParentLoop->removeBlockFromLoop(DelBBlock);
        DelBBlock->eraseFromParent();
      }
      ParentLoop->removeBlockFromLoop(EndBlock);
      EndBlock->eraseFromParent();
      return false;
    }
  }

  // Updating with branch from Entry to PreHeader to NewRootBB
  for (BasicBlock *PredecessorBB : predecessors(LoopPreheader)) {
    auto *BrInstr = PredecessorBB->getTerminator();
    for (unsigned i = 0, NumSuccessor = BrInstr->getNumSuccessors();
         i < NumSuccessor; i++) {
      auto *OldSuccessor = BrInstr->getSuccessor(i);
      if (OldSuccessor == LoopPreheader) {
        DTU.applyUpdates(
            {{DominatorTree::Delete, PredecessorBB, LoopPreheader}});
        BrInstr->setSuccessor(i, NewRootBB);
        DTU.applyUpdates({{DominatorTree::Insert, PredecessorBB, NewRootBB}});
      }
    }
  }
  AllDependentInsts.push_back(BranchInst::Create(LoopPreheader, EndBlock));

  // Updating with new BasicBlock in all newly created branch instruction.
  // Updating DominatorTree for all new BasicBlocks.
  for (auto *I : BranchInsts) {
    auto *BrInstr = dyn_cast<llvm::BranchInst>(I);
    for (unsigned i = 0, NumSuccessor = BrInstr->getNumSuccessors();
         i < NumSuccessor; i++) {
      auto *OldSuccessor = BrInstr->getSuccessor(i);
      if (BBTransforms.end() != BBTransforms.find(OldSuccessor)) {
        auto *NewSuccessor = BBTransforms[OldSuccessor];
        BrInstr->setSuccessor(i, NewSuccessor);
        DTU.applyUpdates(
            {{DominatorTree::Insert, BrInstr->getParent(), NewSuccessor}});
      } else {
        BrInstr->setSuccessor(i, EndBlock);
        DTU.applyUpdates(
            {{DominatorTree::Insert, BrInstr->getParent(), EndBlock}});
      }
    }
  }

  for (CallInst *PrefetchCall : InsertedPrefetchCalls) {
    if (!PrefetchCall->getParent()->getTerminator()) {
      DTU.applyUpdates(
          {{DominatorTree::Insert, PrefetchCall->getParent(), EndBlock}});
    }
  }

  auto *InsertPoint = ParentLoop->getLoopPreheader();
  auto *BBTerminator = InsertPoint->getTerminator();
  Instruction *EndPoint = nullptr;
  if (InsertPoint) {
    for (unsigned j = 0; j < AllDependentInsts.size(); j++) {
      auto *I = AllDependentInsts[j];
      if (I->getOpcode() != Instruction::Br)
        if (ParentLoop->hasLoopInvariantOperands(I)) {
          auto *InvariantInstr = I->clone();
          InvariantInstr->insertInto(InsertPoint, InsertPoint->end());
          EndPoint = InvariantInstr;
          I->replaceAllUsesWith(InvariantInstr);
          I->eraseFromParent();
        }
    }
    if (EndPoint)
      BBTerminator->moveAfter(EndPoint);
    NumOuterLoopPrefetches++;
  }
  return true;
}

PreservedAnalyses LoopDataPrefetchPass::run(Function &F,
                                            FunctionAnalysisManager &AM) {
  AliasAnalysis *AA = &AM.getResult<AAManager>(F);
  DominatorTree *DT = &AM.getResult<DominatorTreeAnalysis>(F);
  LoopInfo *LI = &AM.getResult<LoopAnalysis>(F);
  ScalarEvolution *SE = &AM.getResult<ScalarEvolutionAnalysis>(F);
  AssumptionCache *AC = &AM.getResult<AssumptionAnalysis>(F);
  OptimizationRemarkEmitter *ORE =
      &AM.getResult<OptimizationRemarkEmitterAnalysis>(F);
  const TargetTransformInfo *TTI = &AM.getResult<TargetIRAnalysis>(F);

  // Ensure loops are in simplified form which is a pre-requisite for loop data
  // prefetch pass. Added only for new PM since the legacy PM has already added
  // LoopSimplify pass as a dependency.
  bool Changed = false;
  for (auto &L : *LI) {
    Changed |= simplifyLoop(L, DT, LI, SE, AC, nullptr, false);
  }

  LoopDataPrefetch LDP(AA, AC, DT, LI, SE, TTI, ORE);
  Changed |= LDP.run();

  if (Changed) {
    PreservedAnalyses PA;
    PA.preserve<DominatorTreeAnalysis>();
    PA.preserve<LoopAnalysis>();
    return PA;
  }

  return PreservedAnalyses::all();
}

bool LoopDataPrefetchLegacyPass::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  AliasAnalysis *AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();
  DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  ScalarEvolution *SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  AssumptionCache *AC =
      &getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);
  OptimizationRemarkEmitter *ORE =
      &getAnalysis<OptimizationRemarkEmitterWrapperPass>().getORE();
  const TargetTransformInfo *TTI =
      &getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);

  LoopDataPrefetch LDP(AA, AC, DT, LI, SE, TTI, ORE);
  return LDP.run();
}

bool LoopDataPrefetch::run() {
  // If PrefetchDistance is not set, don't run the pass.  This gives an
  // opportunity for targets to run this pass for selected subtargets only
  // (whose TTI sets PrefetchDistance and CacheLineSize).
  if (getPrefetchDistance() == 0 ||
      (TTI->getCacheLineSize() == 0 && CachelineSize == 0)) {
    LLVM_DEBUG(dbgs() << "Please set both PrefetchDistance and CacheLineSize "
                         "for loop data prefetch.\n");
    return false;
  }

  bool MadeChange = false;

  for (Loop *I : *LI)
    for (Loop *L : depth_first(I))
      MadeChange |= runOnLoop(L);

  return MadeChange;
}

/// A record for a potential prefetch made during the initial scan of the
/// loop. This is used to let a single prefetch target multiple memory accesses.
struct Prefetch {
  /// The address formula for this prefetch as returned by ScalarEvolution.
  const SCEVAddRecExpr *LSCEVAddRec;
  /// The point of insertion for the prefetch instruction.
  Instruction *InsertPt = nullptr;
  /// True if targeting a write memory access.
  bool Writes = false;
  /// The (first seen) prefetched instruction.
  Instruction *MemI = nullptr;

  /// Constructor to create a new Prefetch for \p I.
  Prefetch(const SCEVAddRecExpr *L, Instruction *I) : LSCEVAddRec(L) {
    addInstruction(I);
  };

  /// Add the instruction \param I to this prefetch. If it's not the first
  /// one, 'InsertPt' and 'Writes' will be updated as required.
  /// \param PtrDiff the known constant address difference to the first added
  /// instruction.
  void addInstruction(Instruction *I, DominatorTree *DT = nullptr,
                      int64_t PtrDiff = 0) {
    if (!InsertPt) {
      MemI = I;
      InsertPt = I;
      Writes = isa<StoreInst>(I);
    } else {
      BasicBlock *PrefBB = InsertPt->getParent();
      BasicBlock *InsBB = I->getParent();
      if (PrefBB != InsBB) {
        BasicBlock *DomBB = DT->findNearestCommonDominator(PrefBB, InsBB);
        if (DomBB != PrefBB)
          InsertPt = DomBB->getTerminator();
      }

      if (isa<StoreInst>(I) && PtrDiff == 0)
        Writes = true;
    }
  }
};

bool LoopDataPrefetch::runOnLoop(Loop *L) {
  bool MadeChange = false;

  if (L->getLoopDepth() < PrefetchLoopDepth)
    return MadeChange;

  bool IsInnerMost = true;
  // Prefetch outer loop if needed.
  if (!L->isInnermost()) {
    if (OuterLoopPrefetch)
      IsInnerMost = false;
    else
      return MadeChange;
  }

  SmallPtrSet<const Value *, 32> EphValues;
  CodeMetrics::collectEphemeralValues(L, AC, EphValues);

  CodeMetrics InnerLoopMetrics;
  // Calculate the sub loop size when prefetching outer loops.
  SmallPtrSet<const BasicBlock *, 8> InnerMostBBs;
  if (!IsInnerMost) {
    for (Loop *LL : L->getSubLoops()) {
      // Make sure all sub loops are inner most loop.
      if (!LL->isInnermost())
        return MadeChange;
      for (const auto BB : LL->blocks()) {
        InnerMostBBs.insert(BB);
        InnerLoopMetrics.analyzeBasicBlock(BB, *TTI, EphValues);
      }
    }
  }

  // Calculate the number of iterations ahead to prefetch
  CodeMetrics Metrics;
  bool HasCall = false;
  for (const auto BB : L->blocks()) {
    // If the loop already has prefetches, then assume that the user knows
    // what they are doing and don't add any more.
    for (auto &I : *BB) {
      if (isa<CallInst>(&I) || isa<InvokeInst>(&I)) {
        if (const Function *F = cast<CallBase>(I).getCalledFunction()) {
          if (F->getIntrinsicID() == Intrinsic::prefetch)
            return MadeChange;
          if (TTI->isLoweredToCall(F))
            HasCall = true;
        } else { // indirect call.
          HasCall = true;
        }
      }
    }
    Metrics.analyzeBasicBlock(BB, *TTI, EphValues);
  }

  if (!Metrics.NumInsts.isValid())
    return MadeChange;

  unsigned LoopSize = *Metrics.NumInsts.getValue();
  if (!LoopSize)
    LoopSize = 1;

  // Only prefetch small outer loops with small sub loops.
  if (!IsInnerMost)
    if (LoopSize - InnerLoopMetrics.NumInsts > 128 ||
        InnerLoopMetrics.NumInsts > 128)
      return MadeChange;

  unsigned ItersAhead = getPrefetchDistance() / LoopSize;
  if (!ItersAhead)
    ItersAhead = 1;

  if (ItersAhead > getMaxPrefetchIterationsAhead())
    return MadeChange;

  unsigned ConstantMaxTripCount = SE->getSmallConstantMaxTripCount(L);
  if (ConstantMaxTripCount && ConstantMaxTripCount < ItersAhead + 1)
    return MadeChange;

  unsigned NumMemAccesses = 0;
  unsigned NumStridedMemAccesses = 0;
  SmallVector<Prefetch, 16> Prefetches;
  for (const auto BB : L->blocks()) {
    // If this is not inner most, we avoid prefetching in sub loops.
    for (auto &I : *BB) {
      Value *PtrValue = nullptr;
      Instruction *MemI;

      if (LoadInst *LMemI = dyn_cast<LoadInst>(&I)) {
        MemI = LMemI;
        PtrValue = LMemI->getPointerOperand();
      } else if (StoreInst *SMemI = dyn_cast<StoreInst>(&I)) {
        if (!doPrefetchWrites()) continue;
        MemI = SMemI;
        PtrValue = SMemI->getPointerOperand();
      } else continue;

      if (!PtrValue)
        continue;
      if (getPrefetchDistance() == 0)
        continue;

      unsigned PtrAddrSpace = PtrValue->getType()->getPointerAddressSpace();
      if (!TTI->shouldPrefetchAddressSpace(PtrAddrSpace))
        continue;
      NumMemAccesses++;
      if (L->isLoopInvariant(PtrValue))
        continue;

      const SCEV *LSCEV = SE->getSCEV(PtrValue);
      const SCEVAddRecExpr *LSCEVAddRec = dyn_cast<SCEVAddRecExpr>(LSCEV);
      if (!LSCEVAddRec)
        continue;
      NumStridedMemAccesses++;

      // For outer loops, we only prefetch memory instruction with stride
      // depending on the current loop.
      if (!IsInnerMost && LSCEVAddRec->getLoop() != L)
        continue;

      // We don't want to double prefetch individual cache lines. If this
      // access is known to be within one cache line of some other one that
      // has already been prefetched, then don't prefetch this one as well.
      bool DupPref = false;
      for (auto &Pref : Prefetches) {
        const SCEV *PtrDiff = SE->getMinusSCEV(LSCEVAddRec, Pref.LSCEVAddRec);
        if (const SCEVConstant *ConstPtrDiff =
            dyn_cast<SCEVConstant>(PtrDiff)) {
          int64_t PD = std::abs(ConstPtrDiff->getValue()->getSExtValue());
          int64_t CacheLineSize =
              TTI->getCacheLineSize() ? TTI->getCacheLineSize() : CachelineSize;
          if (PD < (int64_t)CacheLineSize) {
            Pref.addInstruction(MemI, DT, PD);
            DupPref = true;
            break;
          }
        }
      }
      if (!DupPref && !DisableDirectLoadPrefetch)
        Prefetches.push_back(Prefetch(LSCEVAddRec, MemI));
    }
  }

  unsigned TargetMinStride =
    getMinPrefetchStride(NumMemAccesses, NumStridedMemAccesses,
                         Prefetches.size(), HasCall);

  LLVM_DEBUG(dbgs() << "Prefetching " << ItersAhead
             << " iterations ahead (loop size: " << LoopSize << ") in "
             << L->getHeader()->getParent()->getName() << ": " << *L);
  LLVM_DEBUG(dbgs() << "Loop has: "
             << NumMemAccesses << " memory accesses, "
             << NumStridedMemAccesses << " strided memory accesses, "
             << Prefetches.size() << " potential prefetch(es), "
             << "a minimum stride of " << TargetMinStride << ", "
             << (HasCall ? "calls" : "no calls") << ".\n");

  for (auto &P : Prefetches) {
    // Check if the stride of the accesses is large enough to warrant a
    // prefetch. If MinPrefetchStride <= 1, no need to check if any stride
    // goes.
    const SCEV *StrideExpr = P.LSCEVAddRec->getStepRecurrence(*SE);
    if (!isStrideLargeEnough(P.LSCEVAddRec, TargetMinStride))
      continue;

    BasicBlock *BB = P.InsertPt->getParent();
    SCEVExpander SCEVE(*SE, BB->getModule()->getDataLayout(), "prefaddr");
    const SCEV *NextLSCEV = SE->getAddExpr(
        P.LSCEVAddRec,
        SE->getMulExpr(SE->getConstant(P.LSCEVAddRec->getType(), ItersAhead),
                       StrideExpr));
    if (!SCEVE.isSafeToExpand(NextLSCEV))
      continue;

    unsigned PtrAddrSpace = NextLSCEV->getType()->getPointerAddressSpace();
    Type *I8Ptr = Type::getInt8PtrTy(BB->getContext(), PtrAddrSpace);
    Value *PrefPtrValue = SCEVE.expandCodeFor(NextLSCEV, I8Ptr, P.InsertPt);

    IRBuilder<> Builder(P.InsertPt);
    Module *M = BB->getParent()->getParent();
    Type *I32 = Type::getInt32Ty(BB->getContext());
    Function *PrefetchFunc = Intrinsic::getDeclaration(
        M, Intrinsic::prefetch, PrefPtrValue->getType());
    Builder.CreateCall(PrefetchFunc,
                       {PrefPtrValue, ConstantInt::get(I32, P.Writes),
                        ConstantInt::get(I32, IsInnerMost ? 3 : 0),
                        ConstantInt::get(I32, 1)});
    ++NumPrefetches;
    LLVM_DEBUG(dbgs() << "  Access: "
               << *P.MemI->getOperand(isa<LoadInst>(P.MemI) ? 0 : 1)
               << ", SCEV: " << *P.LSCEVAddRec << "\n");
    ORE->emit([&]() {
        return OptimizationRemark(DEBUG_TYPE, "Prefetched", P.MemI)
          << "prefetched memory access";
      });

    MadeChange = true;
  }

  if (!IndirectLoadPrefetch)
    return MadeChange;

  // List of valid phi nodes that indirect loads can depend on.
  SmallPtrSet<Instruction *, 4> LoopAuxIndPHINodes;
  // Map of valid phi node to its bound value in the preheader.
  ValueMap<PHINode *, Value *> AuxIndBounds;
  // Candidate memory loads in the loop.
  SmallVector<Instruction *, 4> CandidateMemoryLoads;
  // List of instruction from phi to load.
  std::vector<SmallSetVector<Instruction *, 8>> DependentInstList;
  // List of store instr in the loop.
  SmallVector<Value *, 4> LoopStorePtrs;

  // Get loop induction and auxiliary induction phis. (Thye will be candidates
  // for phi node matching during construction of the candidate instructions.)
  // And we use the phi nodes to determine the loop upperbound.
  Value *NumIterations =
      getLoopIterationNumber(L, LoopAuxIndPHINodes, AuxIndBounds);
  bool PrefetchInOuterLoop = false;
  if (NumIterations == nullptr) {
    if (!L->isOutermost()) {
      NumIterations = getLoopIterationNumber(L->getParentLoop(),
                                             LoopAuxIndPHINodes, AuxIndBounds);
      if (NumIterations == nullptr)
        return MadeChange;
      PrefetchInOuterLoop = true;
    } else
      return MadeChange;
  }

  if (!RandomAccessPrefetchOnly && !PrefetchInOuterLoop &&
      !canDoIndirectPrefetch(L)) {
    cleanLoopIterationNumber(NumIterations);
    return MadeChange;
  }

  // Find candidate auxiliary induction variables which could be a dependent for
  // the indirect load.
  BasicBlock *Header = nullptr;
  Loop *CurrentLoop = L;
  if (PrefetchInOuterLoop) {
    Header = L->getParentLoop()->getHeader();
    CurrentLoop = L->getParentLoop();
  } else {
    if (getBooleanLoopAttribute(L, "llvm.loop.isvectorized"))
      return false;
    Header = L->getHeader();
  }

  for (auto &I : *Header)
    if (PHINode *PHI = dyn_cast<PHINode>(&I)) {
      InductionDescriptor IndDesc;
      if (InductionDescriptor::isInductionPHI(PHI, CurrentLoop, SE, IndDesc) &&
          CurrentLoop->getInductionVariable(*SE) != PHI) {
        canGetAuxIndVarBound(CurrentLoop, PHI, LoopAuxIndPHINodes);
      }
    }

  // Will search for candidates in the parent loop of the current inner most
  // loop. This will capture more opportunities in the outer loop.
  SmallVector<BasicBlock *, 8> BBList;
  for (auto &BB : L->blocks())
    BBList.push_back(BB);
  if (L->getParentLoop())
    for (auto &BB : L->getParentLoop()->blocks()) {
      // We don't want to repeat blocks in the case of nested loops.
      if (L->contains(BB))
        continue;
      BBList.push_back(BB);
    }

  // Iterate through the loop and keep track of the memory loads and the
  // instruction list they depend on.
  for (const auto BB : BBList) {
    for (auto &I : *BB)
      if (LoadInst *LoadI = dyn_cast<LoadInst>(&I)) {
        SmallSetVector<Instruction *, 8> InstList;
        SmallSet<Instruction *, 8> InstSet;
        InstList.insert(LoadI);
        InstSet.insert(LoadI);
        if (findCandidateMemoryLoads(LoadI, InstList, InstSet,
                                     CandidateMemoryLoads, DependentInstList,
                                     LoopAuxIndPHINodes, PrefetchInOuterLoop,
                                     L)) {
          LLVM_DEBUG(dbgs() << "Found load candidate " << *LoadI << "\n");
          CandidateMemoryLoads.push_back(LoadI);
          DependentInstList.push_back(InstList);
        }
      } else if (StoreInst *StoreI = dyn_cast<StoreInst>(&I)) {
        // Keep track of store insts to avoid conflict.
        LoopStorePtrs.push_back(StoreI->getPointerOperand());
      }
  }

  // Keep track of previously transformed instrs for offset load and target
  // loads so we can reuse them.
  SmallVector<DenseMap<Value *, Value *>> Transforms;
  for (unsigned i = 0; i < CandidateMemoryLoads.size(); i++) {
    SmallSetVector<Instruction *, 8> DependentInsts = DependentInstList[i];
    unsigned NumLoads = 0;
    bool NoConflict = true;
    // Find candidate that contains indirect loads and check load for offset
    // doesn't alias with other stores.
    for (auto DependentInst : DependentInsts) {
      if (LoadInst *LoadI = dyn_cast<LoadInst>(DependentInst)) {
        NumLoads++;
        // For the load of target address offset, we avoid the load being
        // conflict with stores in the same loop.
        if (NumLoads == IndirectionLevel) {
          Value *LoadPtr = LoadI->getPointerOperand();
          for (Value *StorePtr : LoopStorePtrs)
            if (AA->isMustAlias(LoadPtr, StorePtr)) {
              NoConflict = false;
              break;
            }
        }
      }
    }

    // Prefetch all indirect loads without conflict to the offset load.
    if (NumLoads == IndirectionLevel && NoConflict) {
      if (PrefetchInOuterLoop) {
        MadeChange |= insertPrefetcherInOuterloopForIndirectLoad(
            L, i, NumIterations, CandidateMemoryLoads, DependentInsts,
            AuxIndBounds, Transforms, ItersAhead);
        break;
      } else {
        MadeChange |= insertPrefetcherForIndirectLoad(
            L, i, NumIterations, CandidateMemoryLoads, DependentInsts,
            AuxIndBounds, Transforms, ItersAhead);
      }
    }
  }

  cleanLoopIterationNumber(NumIterations);

  return MadeChange;
}
