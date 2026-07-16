//===--- UnsequencedFunctionParameterCheck.h - clang-tidy -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_UNSEQUENCEDFUNCTIONPARAMETERCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_UNSEQUENCEDFUNCTIONPARAMETERCHECK_H

#include "../ClangTidyCheck.h"

namespace clang::tidy::BSCompatibility {

/// Detect multiple function parameters and provides suggestions for extracting
/// parameters from the function call.
class UnsequencedFunctionParameterCheck : public ClangTidyCheck {
public:
  UnsequencedFunctionParameterCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  int GlobalTempCounter = 0;
  SourceLocation findSafeInsertionPoint(const CallExpr *Callee,
                                        SourceManager &SM, ASTContext &Context);
  SourceLocation findPreviousLineEnd(SourceLocation Loc, SourceManager &SM);
};

} // namespace clang::tidy::BSCompatibility

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_UNSEQUENCEDFUNCTIONPARAMETERCHECK_H
