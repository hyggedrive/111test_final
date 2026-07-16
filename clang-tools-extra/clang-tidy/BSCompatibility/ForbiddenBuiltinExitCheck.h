//===--- ForbiddenBuiltinExitCheck.h - clang-tidy ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_FORBIDDENBUILTINEXITCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_FORBIDDENBUILTINEXITCHECK_H

#include "../ClangTidyCheck.h"

namespace clang::tidy::BSCompatibility {

// Detect the use of __builtin_exit and propose warning.
class ForbiddenBuiltinExitCheck : public ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP,
                           Preprocessor *ModuleExpanderPP) override;
};

} // namespace clang::tidy::BSCompatibility

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_FORBIDDENBUILTINEXITCHECK_H