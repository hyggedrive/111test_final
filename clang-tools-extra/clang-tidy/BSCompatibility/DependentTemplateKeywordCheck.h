//===--- DependentTemplateKeywordCheck.h - clang-tidy -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_DEPENDENTTEMPLATEKEYWORDCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_DEPENDENTTEMPLATEKEYWORDCHECK_H

#include "../ClangTidyCheck.h"

namespace clang::tidy::BSCompatibility {
// Detect if template is missing when calling a dependent template function
// and add template keyword if so

class DependentTemplateKeywordCheck : public ClangTidyCheck {
public:
  DependentTemplateKeywordCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  // generate the unique variable name
  std::string generateTempVarName(SourceLocation Loc, ASTContext *Context);
};

} // namespace clang::tidy::BSCompatibility

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_DEPENDENTTEMPLATEKEYWORDCHECK_H