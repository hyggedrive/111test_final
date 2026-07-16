//===- LoopVectorizePrepare.h - Loop Vectorize Prepare ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides the interface for the Loop Vectorize Prepare Pass.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_TRANSFORMS_VECTORIZE_LOOPVECTORIZEPREPARE_H
#define LLVM_TRANSFORMS_VECTORIZE_LOOPVECTORIZEPREPARE_H

#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Utils.h"

namespace llvm {

class LoopVectorizePreparePass
    : public PassInfoMixin<LoopVectorizePreparePass> {
public:
  LoopVectorizePreparePass() = default;

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_VECTORIZE_LOOPVECTORIZEPREPARE_H
