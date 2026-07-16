//===- bolt/Core/BinaryBasicBlockFeature.cpp - Low-level basic block
//-------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the BinaryBasicBlock class.
//
//===----------------------------------------------------------------------===//

#include "bolt/Core/BinaryBasicBlock.h"
#include "bolt/Core/BinaryBasicBlockFeature.h"

#define DEBUG_TYPE "bolt"

namespace llvm {
namespace bolt {} // namespace bolt
} // namespace llvm