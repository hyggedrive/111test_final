//===- bolt/Core/BinaryBasicBlockFeature.h - Low-level basic block -----*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Features of BinaryBasicBlock
//
//===----------------------------------------------------------------------===//

#ifndef BOLT_CORE_BINARY_BASIC_BLOCK_FEATURE_H
#define BOLT_CORE_BINARY_BASIC_BLOCK_FEATURE_H

#include "bolt/Core/FunctionLayout.h"
#include "bolt/Core/MCPlus.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/raw_ostream.h"
#include <limits>
#include <utility>

namespace llvm {

namespace bolt {

class BinaryBasicBlockFeature {

public:
  int32_t Opcode;

  int16_t Direction;

  int32_t CmpOpcode;

  int16_t LoopHeader;

  int16_t ProcedureType;

  int64_t Count;

  int64_t FallthroughCount;

  int64_t TotalLoops;

  int64_t LoopDepth;

  int64_t LoopNumBlocks;

  int64_t LocalExitingBlock;

  int64_t LocalLatchBlock;

  int64_t LocalLoopHeader;

  int64_t Call;

  int64_t DeltaTaken;

  int64_t NumLoads;

  int64_t NumCalls;

  int64_t OperandRAType;

  int64_t OperandRBType;

  int64_t BasicBlockSize;

  int64_t NumBasicBlocks;

  int64_t HasIndirectCalls;

  std::vector<int32_t> EndOpcode_vec;

  std::vector<int16_t> LoopHeader_vec;

  std::vector<int16_t> Backedge_vec;

  std::vector<int16_t> Exit_vec;

  std::vector<int16_t> Call_vec;

  std::vector<int64_t> BasicBlockSize_vec;

  std::vector<int64_t> InferenceFeatures;

  uint64_t FuncExec;

  int32_t ParentChildNum;

  int32_t ParentCount;

  int32_t ChildParentNum;

  int32_t ChildCount;

public:
  void setOpcode(const int32_t &BlockOpcode) { Opcode = BlockOpcode; }

  void setDirection(const int16_t &BlockDirection) {
    Direction = BlockDirection;
  }

  void setCmpOpcode(const int32_t &BlockCmpOpcode) {
    CmpOpcode = BlockCmpOpcode;
  }

  void setLoopHeader(const int16_t &BlockLoopHeader) {
    LoopHeader = BlockLoopHeader;
  }

  void setProcedureType(const int16_t &BlockProcedureType) {
    ProcedureType = BlockProcedureType;
  }

  void setCount(const int64_t &BlockCount) { Count = BlockCount; }

  void setFallthroughCount(const int64_t &BlockFallthroughCount) {
    FallthroughCount = BlockFallthroughCount;
  }

  void setTotalLoops(const int64_t &BlockTotalLoops) {
    TotalLoops = BlockTotalLoops;
  }

  void setLoopDepth(const int64_t &BlockLoopDepth) {
    LoopDepth = BlockLoopDepth;
  }

  void setLoopNumBlocks(const int64_t &BlockLoopNumBlocks) {
    LoopNumBlocks = BlockLoopNumBlocks;
  }

  void setLocalExitingBlock(const int64_t &BlockLocalExitingBlock) {
    LocalExitingBlock = BlockLocalExitingBlock;
  }

  void setLocalLatchBlock(const int64_t &BlockLocalLatchBlock) {
    LocalLatchBlock = BlockLocalLatchBlock;
  }

  void setLocalLoopHeader(const int64_t &BlockLocalLoopHeader) {
    LocalLoopHeader = BlockLocalLoopHeader;
  }

  void setDeltaTaken(const int64_t &BlockDeltaTaken) {
    DeltaTaken = BlockDeltaTaken;
  }

  void setNumLoads(const int64_t &BlockNumLoads) { NumLoads = BlockNumLoads; }

  void setNumCalls(const int64_t &BlockNumCalls) { NumCalls = BlockNumCalls; }

  void setOperandRAType(const int64_t &BlockOperandRAType) {
    OperandRAType = BlockOperandRAType;
  }

  void setOperandRBType(const int64_t &BlockOperandRBType) {
    OperandRBType = BlockOperandRBType;
  }

  void setBasicBlockSize(const int64_t &BlockBasicBlockSize) {
    BasicBlockSize = BlockBasicBlockSize;
  }

  void setNumBasicBlocks(const int64_t &BlockNumBasicBlocks) {
    NumBasicBlocks = BlockNumBasicBlocks;
  }

  void setHasIndirectCalls(const int64_t &BlockHasIndirectCalls) {
    HasIndirectCalls = BlockHasIndirectCalls;
  }

  void setEndOpcodeVec(const int32_t &EndOpcode) {
    EndOpcode_vec.push_back(EndOpcode);
  }

  void setLoopHeaderVec(const int16_t &LoopHeader) {
    LoopHeader_vec.push_back(LoopHeader);
  }

  void setBackedgeVec(const int16_t &Backedge) {
    Backedge_vec.push_back(Backedge);
  }

  void setExitVec(const int16_t &Exit) { Exit_vec.push_back(Exit); }

  void setCallVec(const int16_t &Call) { Call_vec.push_back(Call); }

  void setBasicBlockSizeVec(const int64_t &BasicBlockSize) {
    BasicBlockSize_vec.push_back(BasicBlockSize);
  }

  void setFunExec(const uint64_t &BlockFuncExec) { FuncExec = BlockFuncExec; }

  void setParentChildNum(const int32_t &BlockParentChildNum) {
    ParentChildNum = BlockParentChildNum;
  }

  void setParentCount(const int32_t &BlockParentCount) {
    ParentCount = BlockParentCount;
  }

  void setChildParentNum(const int32_t &BlockChildParentNum) {
    ChildParentNum = BlockChildParentNum;
  }

  void setChildCount(const int32_t &BlockChildCount) {
    ChildCount = BlockChildCount;
  }

  void setInferenceFeatures() {

    if (Count == -1 || FallthroughCount == -1) {
      return;
    }
    if (ParentChildNum == -1 && ParentCount == -1 && ChildParentNum == -1 &&
        ChildCount == -1) {
      return;
    }

    InferenceFeatures.push_back(static_cast<int64_t>(Direction));
    InferenceFeatures.push_back(static_cast<int64_t>(LoopHeader));
    InferenceFeatures.push_back(static_cast<int64_t>(ProcedureType));
    InferenceFeatures.push_back(static_cast<int64_t>(OperandRAType));
    InferenceFeatures.push_back(static_cast<int64_t>(OperandRBType));
    InferenceFeatures.push_back(static_cast<int64_t>(LoopHeader_vec[0]));
    InferenceFeatures.push_back(static_cast<int64_t>(Backedge_vec[0]));
    InferenceFeatures.push_back(static_cast<int64_t>(Exit_vec[0]));
    InferenceFeatures.push_back(static_cast<int64_t>(LoopHeader_vec[1]));
    InferenceFeatures.push_back(static_cast<int64_t>(Call_vec[0]));
    InferenceFeatures.push_back(static_cast<int64_t>(LocalExitingBlock));
    InferenceFeatures.push_back(static_cast<int64_t>(HasIndirectCalls));
    InferenceFeatures.push_back(static_cast<int64_t>(LocalLatchBlock));
    InferenceFeatures.push_back(static_cast<int64_t>(LocalLoopHeader));
    InferenceFeatures.push_back(static_cast<int64_t>(Opcode));
    InferenceFeatures.push_back(static_cast<int64_t>(CmpOpcode));
    InferenceFeatures.push_back(static_cast<int64_t>(EndOpcode_vec[0]));
    InferenceFeatures.push_back(static_cast<int64_t>(EndOpcode_vec[1]));
    InferenceFeatures.push_back(static_cast<int64_t>(FuncExec));
    InferenceFeatures.push_back(static_cast<int64_t>(NumBasicBlocks));
    InferenceFeatures.push_back(static_cast<int64_t>(BasicBlockSize));
    InferenceFeatures.push_back(static_cast<int64_t>(BasicBlockSize_vec[0]));
    InferenceFeatures.push_back(static_cast<int64_t>(BasicBlockSize_vec[1]));
    InferenceFeatures.push_back(static_cast<int64_t>(LoopNumBlocks));
    InferenceFeatures.push_back(static_cast<int64_t>(NumLoads));
    InferenceFeatures.push_back(static_cast<int64_t>(NumCalls));
    InferenceFeatures.push_back(static_cast<int64_t>(TotalLoops));
    InferenceFeatures.push_back(static_cast<int64_t>(DeltaTaken));
    InferenceFeatures.push_back(static_cast<int64_t>(LoopDepth));
    InferenceFeatures.push_back(static_cast<int64_t>(ParentChildNum));
    InferenceFeatures.push_back(static_cast<int64_t>(ParentCount));
    InferenceFeatures.push_back(static_cast<int64_t>(ChildParentNum));
    InferenceFeatures.push_back(static_cast<int64_t>(ChildCount));
  }

  std::vector<int64_t> getInferenceFeatures() { return InferenceFeatures; }
};
} // namespace bolt
} // namespace llvm

#endif