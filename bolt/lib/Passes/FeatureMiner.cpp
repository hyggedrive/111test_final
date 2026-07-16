//===--- Passes/FeatureMiner.cpp ------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// A very simple feature extractor based on Calder's paper
// Evidence-based static branch prediction using machine learning
// https://dl.acm.org/doi/10.1145/239912.239923
//===----------------------------------------------------------------------===//

#include "bolt/Passes/DataflowInfoManager.h"
#include "bolt/Passes/FeatureMiner.h"
#include "bolt/Passes/StaticBranchInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"

#undef DEBUG_TYPE
#define DEBUG_TYPE "bolt-feature-miner"

using namespace llvm;
using namespace bolt;

namespace opts {
extern cl::opt<bool> BlockCorrection;

} // namespace opts

namespace llvm {
namespace bolt {

class BinaryFunction;

int8_t FeatureMiner::getProcedureType(BinaryFunction &Function,
                                      BinaryContext &BC) {
  int8_t ProcedureType = 1;
  for (auto &BB : Function) {
    for (auto &Inst : BB) {
      if (BC.MIB->isCall(Inst)) {
        ProcedureType = 0; // non-leaf type
        if (const auto *CalleeSymbol = BC.MIB->getTargetSymbol(Inst)) {
          const auto *Callee = BC.getFunctionForSymbol(CalleeSymbol);
          if (Callee &&
              Callee->getFunctionNumber() == Function.getFunctionNumber()) {
            return 2; // call self type
          }
        }
      }
    }
  }
  return ProcedureType; // leaf type
}

void FeatureMiner::addSuccessorInfo(BFIPtr const &BFI, BinaryFunction &Function,
                                    BinaryContext &BC, BinaryBasicBlock &BB,
                                    bool SuccType) {

  BinaryBasicBlock *Successor = BB.getConditionalSuccessor(SuccType);

  if (!Successor)
    return;

  unsigned NumCalls{0};

  for (auto &Inst : BB) {
    if (BC.MIB->isCall(Inst)) {
      ++NumCalls;
    }
  }

  BBIPtr SuccBBInfo = std::make_unique<struct BasicBlockInfo>();

  // Check if the successor basic block is a loop header and store it.
  SuccBBInfo->LoopHeader = SBI->isLoopHeader(Successor);

  SuccBBInfo->BasicBlockSize = Successor->size();

  // Check if the edge getting to the successor basic block is a loop
  // exit edge and store it.
  SuccBBInfo->Exit = SBI->isExitEdge(&BB, Successor);

  // Check if the edge getting to the successor basic block is a loop
  // back edge and store it.
  SuccBBInfo->Backedge = SBI->isBackEdge(&BB, Successor);

  MCInst *SuccInst = Successor->getTerminatorBefore(nullptr);

  // Store information about the branch type ending sucessor basic block
  SuccBBInfo->EndOpcode = (SuccInst && BC.MIA->isBranch(*SuccInst))
                              ? SuccInst->getOpcode()
                              : 0; // 0 = NOTHING

  // Check if the successor basic block contains
  // a procedure call and store it.
  SuccBBInfo->Call = (NumCalls > 0) ? 1  // Contains a call instruction
                                    : 0; // Does not contain a call instruction

  uint32_t Offset = BB.getEndOffset();

  if (SuccType) {
    BFI->TrueSuccessor = std::move(SuccBBInfo);
    // Check if the taken branch is a forward
    // or a backwards branch and store it
    BFI->Direction = (Function.isForwardBranch(&BB, Successor) == true)
                         ? 1  // Forward branch
                         : 0; // Backwards branch

    auto OnlyBranchInfo = BB.getOnlyBranchInfo();
    BFI->Count = OnlyBranchInfo.Count;

    if (Offset) {
      uint32_t TargetOffset = Successor->getInputOffset();
      uint32_t BranchOffset = Offset;
      if (BranchOffset != UINT32_MAX && TargetOffset != UINT32_MAX) {
        int64_t Delta = static_cast<int64_t>(TargetOffset) -
                        static_cast<int64_t>(BranchOffset);
        BFI->DeltaTaken = std::abs(Delta);
      }
    }
  } else {
    if (BB.succ_size() == 2) {
      auto FallthroughBranchInfo = BB.getFallthroughBranchInfo();
      BFI->FallthroughCount = FallthroughBranchInfo.Count;
    } else {
      auto OnlyBranchInfo = BB.getOnlyBranchInfo();
      BFI->FallthroughCount = OnlyBranchInfo.Count;
    }
    BFI->FalseSuccessor = std::move(SuccBBInfo);
  }
}

void FeatureMiner::extractFeatures(BinaryFunction &Function,
                                   BinaryContext &BC) {
  int8_t ProcedureType = getProcedureType(Function, BC);
  auto Info = DataflowInfoManager(Function, nullptr, nullptr);
  const BinaryLoopInfo &LoopsInfo = Function.getLoopInfo();

  bool Simple = Function.isSimple();
  const auto &Order = Function.dfs();
  std::string Function_name = Function.getPrintName();

  for (auto *BBA : Order) {

    auto &BB = *BBA;

    BinaryBasicBlockFeature BBF = BB.getFeatures();

    unsigned TotalLoops{0};
    unsigned LoopDepth{0};
    unsigned LoopNumBlocks{0};

    bool LocalExitingBlock{false};
    bool LocalLatchBlock{false};
    bool LocalLoopHeader{false};

    generateProfileFeatures(&BB, &BBF);

    BinaryLoop *Loop = LoopsInfo.getLoopFor(&BB);
    if (Loop) {
      SmallVector<BinaryBasicBlock *, 1> ExitingBlocks;
      Loop->getExitingBlocks(ExitingBlocks);

      SmallVector<BinaryBasicBlock *, 1> ExitBlocks;
      Loop->getExitBlocks(ExitBlocks);

      SmallVector<BinaryLoop::Edge, 1> ExitEdges;
      Loop->getExitEdges(ExitEdges);

      SmallVector<BinaryBasicBlock *, 1> Latches;
      Loop->getLoopLatches(Latches);

      TotalLoops = LoopsInfo.TotalLoops;
      LoopDepth = Loop->getLoopDepth();
      LoopNumBlocks = Loop->getNumBlocks();
      LocalExitingBlock = Loop->isLoopExiting(&BB);
      LocalLatchBlock = Loop->isLoopLatch(&BB);
      LocalLoopHeader = ((Loop->getHeader() == (&BB)) ? 1 : 0);
    }

    unsigned NumLoads{0};
    unsigned NumCalls{0};
    unsigned NumIndirectCalls{0};

    for (auto &Inst : BB) {
      if (BC.MIB->mayLoad(Inst)) {
        ++NumLoads;
      } else if (BC.MIB->isCall(Inst)) {
        ++NumCalls;
        if (BC.MIB->isIndirectCall(Inst))
          ++NumIndirectCalls;
      }
    }

    int Index = -2;
    bool LoopHeader = SBI->isLoopHeader(&BB);

    BFIPtr BFI = std::make_unique<struct BranchFeaturesInfo>();

    BFI->TotalLoops = TotalLoops;
    BFI->LoopDepth = LoopDepth;
    BFI->LoopNumBlocks = LoopNumBlocks;
    BFI->LocalExitingBlock = LocalExitingBlock;
    BFI->LocalLatchBlock = LocalLatchBlock;
    BFI->LocalLoopHeader = LocalLoopHeader;
    BFI->NumCalls = NumCalls;
    BFI->BasicBlockSize = BB.size();
    BFI->NumBasicBlocks = Function.size();

    BFI->NumLoads = NumLoads;
    BFI->NumIndirectCalls = NumIndirectCalls;
    BFI->LoopHeader = LoopHeader;
    BFI->ProcedureType = ProcedureType;

    // Adding taken successor info.
    addSuccessorInfo(BFI, Function, BC, BB, true);
    // Adding fall through successor info.
    addSuccessorInfo(BFI, Function, BC, BB, false);

    MCInst ConditionalInst;
    bool hasConditionalBranch = false;
    MCInst UnconditionalInst;
    bool hasUnconditionalBranch = false;

    for (auto &Inst : BB) {
      ++Index;
      if (!BC.MIA->isConditionalBranch(Inst) &&
          !BC.MIA->isUnconditionalBranch(Inst))
        continue;

      generateInstFeatures(BC, BB, BFI, Index);

      if (BC.MIA->isConditionalBranch(Inst)) {
        ConditionalInst = Inst;
        hasConditionalBranch = true;
      }

      if (BC.MIA->isUnconditionalBranch(Inst)) {
        UnconditionalInst = Inst;
        hasUnconditionalBranch = true;
      }
    }

    if (hasConditionalBranch) {
      BFI->Opcode = ConditionalInst.getOpcode();

    } else {
      if (hasUnconditionalBranch) {
        BFI->Opcode = UnconditionalInst.getOpcode();

      } else {
        auto Inst = BB.getLastNonPseudoInstr();
        BFI->Opcode = Inst->getOpcode();
        generateInstFeatures(BC, BB, BFI, Index);
      }
    }

    auto &FalseSuccessor = BFI->FalseSuccessor;
    auto &TrueSuccessor = BFI->TrueSuccessor;

    int16_t ProcedureType = (BFI->ProcedureType.has_value())
                                ? static_cast<int16_t>(*(BFI->ProcedureType))
                                : -1;

    int64_t Count =
        (BFI->Count.has_value()) ? static_cast<int64_t>(*(BFI->Count)) : -1;

    int64_t FallthroughCount =
        (BFI->FallthroughCount.has_value())
            ? static_cast<int64_t>(*(BFI->FallthroughCount))
            : -1;

    int16_t LoopHeaderValid = (BFI->LoopHeader.has_value())
                                  ? static_cast<bool>(*(BFI->LoopHeader))
                                  : -1;

    int64_t TotalLoopsValid = (BFI->TotalLoops.has_value())
                                  ? static_cast<int64_t>(*(BFI->TotalLoops))
                                  : -1;
    int64_t LoopDepthValid = (BFI->LoopDepth.has_value())
                                 ? static_cast<int64_t>(*(BFI->LoopDepth))
                                 : -1;
    int64_t LoopNumBlocksValid =
        (BFI->LoopNumBlocks.has_value())
            ? static_cast<int64_t>(*(BFI->LoopNumBlocks))
            : -1;
    int64_t LocalExitingBlockValid =
        (BFI->LocalExitingBlock.has_value())
            ? static_cast<bool>(*(BFI->LocalExitingBlock))
            : -1;

    int64_t LocalLatchBlockValid =
        (BFI->LocalLatchBlock.has_value())
            ? static_cast<bool>(*(BFI->LocalLatchBlock))
            : -1;

    int64_t LocalLoopHeaderValid =
        (BFI->LocalLoopHeader.has_value())
            ? static_cast<bool>(*(BFI->LocalLoopHeader))
            : -1;

    int32_t CmpOpcode = (BFI->CmpOpcode.has_value())
                            ? static_cast<int32_t>(*(BFI->CmpOpcode))
                            : -1;

    int64_t OperandRAType = (BFI->OperandRAType.has_value())
                                ? static_cast<int32_t>(*(BFI->OperandRAType))
                                : 10;

    int64_t OperandRBType = (BFI->OperandRBType.has_value())
                                ? static_cast<int32_t>(*(BFI->OperandRBType))
                                : 10;
    int16_t Direction = (BFI->Direction.has_value())
                            ? static_cast<bool>(*(BFI->Direction))
                            : -1;

    int64_t DeltaTaken = (BFI->DeltaTaken.has_value())
                             ? static_cast<int64_t>(*(BFI->DeltaTaken))
                             : -1;

    int64_t NumLoadsValid = (BFI->NumLoads.has_value())
                                ? static_cast<int64_t>(*(BFI->NumLoads))
                                : -1;

    int64_t BasicBlockSize = (BFI->BasicBlockSize.has_value())
                                 ? static_cast<int64_t>(*(BFI->BasicBlockSize))
                                 : -1;

    int64_t NumBasicBlocks = (BFI->NumBasicBlocks.has_value())
                                 ? static_cast<int64_t>(*(BFI->NumBasicBlocks))
                                 : -1;

    int64_t NumCallsValid = (BFI->NumCalls.has_value())
                                ? static_cast<int64_t>(*(BFI->NumCalls))
                                : -1;

    int64_t NumIndirectCallsValid =
        (BFI->NumIndirectCalls.has_value())
            ? static_cast<int64_t>(*(BFI->NumIndirectCalls))
            : -1;

    int64_t HasIndirectCalls = (NumIndirectCallsValid > 0) ? 1 : 0;

    int32_t Opcode =
        (BFI->Opcode.has_value()) ? static_cast<int32_t>(*(BFI->Opcode)) : -1;

    uint64_t fun_exec = Function.getExecutionCount();
    fun_exec = (fun_exec != UINT64_MAX) ? fun_exec : 0;

    BBF.setDirection(Direction);
    BBF.setDeltaTaken(DeltaTaken);
    BBF.setOpcode(Opcode);
    BBF.setCmpOpcode(CmpOpcode);
    BBF.setOperandRAType(OperandRAType);
    BBF.setOperandRBType(OperandRBType);
    BBF.setFunExec(fun_exec);
    BBF.setTotalLoops(TotalLoopsValid);
    BBF.setLoopDepth(LoopDepthValid);
    BBF.setLoopNumBlocks(LoopNumBlocksValid);
    BBF.setLocalExitingBlock(LocalExitingBlockValid);
    BBF.setLocalLatchBlock(LocalLatchBlockValid);
    BBF.setLocalLoopHeader(LocalLoopHeaderValid);
    BBF.setNumCalls(NumCallsValid);
    BBF.setBasicBlockSize(BasicBlockSize);
    BBF.setNumBasicBlocks(NumBasicBlocks);
    BBF.setNumLoads(NumLoadsValid);
    BBF.setHasIndirectCalls(HasIndirectCalls);
    BBF.setLoopHeader(LoopHeaderValid);
    BBF.setProcedureType(ProcedureType);
    BBF.setCount(Count);
    BBF.setFallthroughCount(FallthroughCount);

    generateSuccessorFeatures(TrueSuccessor, &BBF);
    generateSuccessorFeatures(FalseSuccessor, &BBF);

    FalseSuccessor.reset();
    TrueSuccessor.reset();

    BBF.setInferenceFeatures();
    BB.setFeatures(BBF);

    BFI.reset();
  }
}

void FeatureMiner::generateInstFeatures(BinaryContext &BC, BinaryBasicBlock &BB,
                                        BFIPtr const &BFI, int Index) {

  // Holds the branch opcode info.

  BFI->CmpOpcode = 0;
  if (Index > -1) {
    auto Cmp = BB.begin() + Index;
    if (BC.MII->get((*Cmp).getOpcode()).isCompare()) {
      // Holding the branch comparison opcode info.
      BFI->CmpOpcode = (*Cmp).getOpcode();
      auto getOperandType = [&](const MCOperand &Operand) -> int32_t {
        if (Operand.isReg())
          return 0;
        else if (Operand.isImm())
          return 1;
        else if (Operand.isSFPImm())
          return 2;
        else if (Operand.isExpr())
          return 3;
        else
          return -1;
      };

      const auto InstInfo = BC.MII->get((*Cmp).getOpcode());
      unsigned NumDefs = InstInfo.getNumDefs();
      int32_t NumPrimeOperands = MCPlus::getNumPrimeOperands(*Cmp) - NumDefs;
      switch (NumPrimeOperands) {
      case 6: {
        int32_t RBType = getOperandType((*Cmp).getOperand(NumDefs));
        int32_t RAType = getOperandType((*Cmp).getOperand(NumDefs + 1));

        if (RBType == 0 && RAType == 0) {
          BFI->OperandRBType = RBType;
          BFI->OperandRAType = RAType;
        } else if (RBType == 0 && (RAType == 1 || RAType == 2)) {
          RAType = getOperandType((*Cmp).getOperand(NumPrimeOperands - 1));

          if (RAType != 1 && RAType != 2) {
            RAType = -1;
          }

          BFI->OperandRBType = RBType;
          BFI->OperandRAType = RAType;
        } else {
          BFI->OperandRAType = -1;
          BFI->OperandRBType = -1;
        }
        break;
      }
      case 2:
        BFI->OperandRBType = getOperandType((*Cmp).getOperand(NumDefs));
        BFI->OperandRAType = getOperandType((*Cmp).getOperand(NumDefs + 1));
        break;
      case 3:
        BFI->OperandRBType = getOperandType((*Cmp).getOperand(NumDefs));
        BFI->OperandRAType = getOperandType((*Cmp).getOperand(NumDefs + 2));
        break;
      case 1:
        BFI->OperandRAType = getOperandType((*Cmp).getOperand(NumDefs));
        break;
      default:
        BFI->OperandRAType = -1;
        BFI->OperandRBType = -1;
        break;
      }

    } else {
      Index -= 1;
      for (int Idx = Index; Idx > -1; Idx--) {
        auto Cmp = BB.begin() + Idx;
        if (BC.MII->get((*Cmp).getOpcode()).isCompare()) {
          // Holding the branch comparison opcode info.
          BFI->CmpOpcode = (*Cmp).getOpcode();
          break;
        }
      }
    }
  }
}

void FeatureMiner::generateSuccessorFeatures(BBIPtr &Successor,
                                             BinaryBasicBlockFeature *BBF) {

  int16_t LoopHeader = (Successor->LoopHeader.has_value())
                           ? static_cast<bool>(*(Successor->LoopHeader))
                           : -1;

  int16_t Backedge = (Successor->Backedge.has_value())
                         ? static_cast<bool>(*(Successor->Backedge))
                         : -1;

  int16_t Exit = (Successor->Exit.has_value())
                     ? static_cast<bool>(*(Successor->Exit))
                     : -1;

  int16_t Call = (Successor->Call.has_value())
                     ? static_cast<bool>(*(Successor->Call))
                     : -1;

  int32_t EndOpcode = (Successor->EndOpcode.has_value())
                          ? static_cast<int32_t>(*(Successor->EndOpcode))
                          : -1;

  int64_t BasicBlockSize =
      (Successor->BasicBlockSize.has_value())
          ? static_cast<int64_t>(*(Successor->BasicBlockSize))
          : -1;

  BBF->setEndOpcodeVec(EndOpcode);
  BBF->setLoopHeaderVec(LoopHeader);
  BBF->setBackedgeVec(Backedge);
  BBF->setExitVec(Exit);
  BBF->setCallVec(Call);
  BBF->setBasicBlockSizeVec(BasicBlockSize);
}

void FeatureMiner::runOnFunctions(BinaryContext &BC) {}

void FeatureMiner::inferenceFeatures(BinaryFunction &Function) {

  SBI = std::make_unique<StaticBranchInfo>();

  if (Function.empty())
    return;

  if (!Function.isLoopFree()) {
    const BinaryLoopInfo &LoopsInfo = Function.getLoopInfo();
    SBI->findLoopEdgesInfo(LoopsInfo);
  }

  BinaryContext &BC = Function.getBinaryContext();
  extractFeatures(Function, BC);

  SBI->clear();
}

void FeatureMiner::generateProfileFeatures(BinaryBasicBlock *BB,
                                           BinaryBasicBlockFeature *BBF) {
  int32_t parentChildNum, parentCount, childParentNum, childCount;

  if (BB->getParentSet().size() == 0) {
    parentChildNum = -1;
    parentCount = -1;
  } else {
    parentChildNum = std::numeric_limits<int32_t>::max();
    parentCount = 0;
    for (BinaryBasicBlock *parent : BB->getParentSet()) {
      if (parent->getChildrenSet().size() < parentChildNum) {
        parentChildNum = parent->getChildrenSet().size();
        parentCount = parent->getExecutionCount();
      } else if (parent->getChildrenSet().size() == parentChildNum &&
                 parent->getExecutionCount() > parentCount) {
        parentCount = parent->getExecutionCount();
      }
    }
  }

  if (BB->getChildrenSet().size() == 0) {
    childParentNum = -1;
    childCount = -1;
  } else {
    childParentNum = std::numeric_limits<int32_t>::max();
    childCount = 0;
    for (BinaryBasicBlock *child : BB->getChildrenSet()) {
      if (child->getParentSet().size() < childParentNum) {
        childParentNum = child->getParentSet().size();
        childCount = child->getExecutionCount();
      } else if (child->getParentSet().size() == childParentNum &&
                 child->getExecutionCount() > childCount) {
        childCount = child->getExecutionCount();
      }
    }
  }

  int64_t parentCountCatch = parentCount > 0 ? 1 : 0;
  int64_t childCountCatch = childCount > 0 ? 1 : 0;

  BBF->setParentChildNum(parentChildNum);
  BBF->setParentCount(parentCountCatch);
  BBF->setChildParentNum(childParentNum);
  BBF->setChildCount(childCountCatch);
}

} // namespace bolt
} // namespace llvm