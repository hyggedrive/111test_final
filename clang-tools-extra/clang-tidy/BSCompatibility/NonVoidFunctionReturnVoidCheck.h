//===--- NonVoidFunctionReturnVoidCheck.h - clang-tidy ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_NONVOIDFUNCTIONRETURNVOIDCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_NONVOIDFUNCTIONRETURNVOIDCHECK_H

#include "../ClangTidyCheck.h"

namespace clang::tidy::BSCompatibility {

// Check if a non-void function hasn't return statement.
class NonVoidFunctionReturnVoidCheck : public ClangTidyCheck {
public:
  NonVoidFunctionReturnVoidCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace clang::tidy::BSCompatibility

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_NONVOIDFUNCTIONRETURNVOIDCHECK_H
