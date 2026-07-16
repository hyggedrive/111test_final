//===- LoopVectorizePrepare.cpp - Loop Vectorize Prepare Pass -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the Loop Vectorize Prepare Pass that before Loop
// Vectorize Pass. It does some pattern analysis and adds some hints (like
// metadatas) which used by LoopVectorizePass to select proper vectorization
// methods according to specific target machine features.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Vectorize/LoopVectorizePrepare.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

using namespace llvm;
using namespace PatternMatch;

#define DEBUG_TYPE "loop-vectorize-prepare"

static cl::opt<bool> EnableLoopVectorizePrepare("enable-loop-vectorize-prepare",
                                                cl::init(true), cl::Hidden);

static unsigned analyzeLoopPatternForURHADD(Loop *L) {
  unsigned NumURHADDPattern = 0;
  for (auto *Block : L->getBlocks())
    for (auto &Inst : *Block) {
      Value *A, *B;

      // Check (a+b+1)>>1 pattern, which can use URHADD instruction in AArch64.
      if (match(&Inst,
                m_LShr(m_Add(m_Add(m_Value(A), m_Value(B)), m_SpecificInt(1)),
                       m_SpecificInt(1))))
        NumURHADDPattern++;
      else if (match(&Inst, m_LShr(m_Add(m_Add(m_Value(A), m_SpecificInt(1)),
                                         m_Value(B)),
                                   m_SpecificInt(1))))
        NumURHADDPattern++;
    }

  return NumURHADDPattern;
}

static bool isSupportSVEOnly(Loop *L, const TargetTransformInfo &TTI) {
  Function *F = L->getHeader()->getParent();
  Triple TargetTriple = Triple(F->getParent()->getTargetTriple());
  if (!TargetTriple.isAArch64())
    return false;

  if (!TTI.enableScalableVectorization())
    return false;

  Attribute Attr = F->getFnAttribute("target-features");
  if (!Attr.isValid())
    return false;
  SmallVector<StringRef, 8> Features;
  bool hasSVE = false;
  bool hasSVE2 = false;
  Attr.getValueAsString().split(Features, ',');
  for (StringRef Feature : Features) {
    if (Feature == "+sve")
      hasSVE = true;
    else if (Feature == "+sve2") {
      hasSVE = true;
      hasSVE2 = true;
    }
  }

  return hasSVE && !hasSVE2;
}

static void addVectorizeMetadataForURHADD(Loop *L,
                                          const TargetTransformInfo &TTI,
                                          unsigned NumPattern) {
  // URHADD instruction is only supported by neon and sve2,
  // so if it only supports sve, prefer to use neon to
  // vectorize by adding metadatas to disable sve.
  if (!isSupportSVEOnly(L, TTI))
    return;

  unsigned NumInsns = 0;
  for (auto *Block : L->getBlocks())
    NumInsns += Block->sizeWithoutDebug();
  // Still try to use sve version if loop has many insns,
  // hope overall sve vectorization can compensate for the
  // benefits of URHADD.
  if (NumInsns / NumPattern >= 20)
    return;

  SmallVector<Metadata *, 8> MDs;
  MDs.push_back(nullptr);
  bool IsAlready = false;
  MDNode *LoopID = L->getLoopID();
  if (LoopID) {
    for (unsigned i = 1, ie = LoopID->getNumOperands(); i < ie; ++i) {
      auto *MD = dyn_cast<MDNode>(LoopID->getOperand(i));
      if (MD) {
        const auto *S = dyn_cast<MDString>(MD->getOperand(0));
        if (S &&
            (S->getString().startswith("llvm.loop.vectorize.scalable.enable") ||
             S->getString().startswith("llvm.loop.vectorize.enable") ||
             S->getString().startswith(
                 "llvm.loop.vectorize.predicate.enable") ||
             S->getString().startswith("llvm.loop.interleave.count")))
          IsAlready = true;
      }
      MDs.push_back(LoopID->getOperand(i));
    }
  }

  if (!IsAlready) {
    LLVMContext &Context = L->getHeader()->getContext();

    MDs.push_back(MDNode::get(
        Context, {MDString::get(Context, "llvm.loop.vectorize.scalable.enable"),
                  ConstantAsMetadata::get(
                      ConstantInt::get(Type::getInt1Ty(Context), 0))}));

    MDs.push_back(MDNode::get(
        Context, {MDString::get(Context, "llvm.loop.vectorize.enable"),
                  ConstantAsMetadata::get(
                      ConstantInt::get(Type::getInt1Ty(Context), 1))}));

    MDs.push_back(MDNode::get(
        Context,
        {MDString::get(Context, "llvm.loop.vectorize.predicate.enable"),
         ConstantAsMetadata::get(
             ConstantInt::get(Type::getInt1Ty(Context), 0))}));

    MDs.push_back(MDNode::get(
        Context, {MDString::get(Context, "llvm.loop.interleave.count"),
                  ConstantAsMetadata::get(
                      ConstantInt::get(Type::getInt32Ty(Context), 1))}));

    MDNode *NewLoopID = MDNode::get(Context, MDs);
    // Set operand 0 to refer to the loop id itself.
    NewLoopID->replaceOperandWith(0, NewLoopID);
    L->setLoopID(NewLoopID);

    LLVM_DEBUG(
        dbgs() << "LVPrepare: Add hint metadatas for URHADD vectorize\n");
  }
}

static bool processLoops(Loop *L, const TargetTransformInfo &TTI) {
  bool Changed = false;

  for (Loop *SubLoop : L->getSubLoops()) {
    Changed |= processLoops(SubLoop, TTI);
  }

  if (L->isInnermost()) {
    unsigned NumPattern = analyzeLoopPatternForURHADD(L);
    if (NumPattern > 0) {
      addVectorizeMetadataForURHADD(L, TTI, NumPattern);
      Changed = true;
    }
  }

  return Changed;
}

static PreservedAnalyses
LoopVectorizePrepareImpl(Function &F, LoopInfo &LI,
                         const TargetTransformInfo &TTI) {
  if (!EnableLoopVectorizePrepare)
    return PreservedAnalyses::all();

  if (LI.empty())
    return PreservedAnalyses::all();

  bool Changed = false;

  for (Loop *L : LI) {
    Changed |= processLoops(L, TTI);
  }

  if (Changed)
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}

PreservedAnalyses LoopVectorizePreparePass::run(Function &F,
                                                FunctionAnalysisManager &AM) {
  LLVM_DEBUG(dbgs() << "Do Loop Vectorize Prepare Pass\n");
  LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
  const TargetTransformInfo &TTI = AM.getResult<TargetIRAnalysis>(F);

  return LoopVectorizePrepareImpl(F, LI, TTI);
}
