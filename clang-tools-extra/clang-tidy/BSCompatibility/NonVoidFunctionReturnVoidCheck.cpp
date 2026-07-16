//===--- NonVoidFunctionReturnVoidCheck.cpp - clang-tidy ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "NonVoidFunctionReturnVoidCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang::tidy::BSCompatibility {

void NonVoidFunctionReturnVoidCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(functionDecl(unless(isMain()),
                                  isDefinition(),
                                  unless(returns(asString("void"))),
                                  unless(hasDescendant(returnStmt())))
                    .bind("x"), this);
}

void NonVoidFunctionReturnVoidCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<FunctionDecl>("x");
  diag(MatchedDecl->getEndLoc(), "Non-void function does not return a val.")
                                 << MatchedDecl->getNameInfo().getSourceRange()
                                 << FixItHint::CreateReplacement(MatchedDecl->getReturnTypeSourceRange(), "void")
                                 << FixItHint::CreateInsertion(MatchedDecl->getEndLoc(), "return something;");
}

} // namespace clang::tidy::BSCompatibility
