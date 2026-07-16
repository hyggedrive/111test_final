//===--- DependentTemplateKeywordCheck.cpp - clang-tidy -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "DependentTemplateKeywordCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/TokenKinds.h"
#include "clang/Lex/Lexer.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tidy;

static clang::SourceLocation getTemplateInsertLocAfterToken(
    clang::SourceLocation OpLoc, clang::tok::TokenKind TokKind,
    const clang::SourceManager &SM, const clang::LangOptions &LangOpts) {
  return clang::Lexer::findLocationAfterToken(OpLoc, TokKind, SM, LangOpts,
                                              /*SkipTrailingWhitespace=*/true);
}

namespace clang::tidy::BSCompatibility {

void DependentTemplateKeywordCheck::registerMatchers(
    ast_matchers::MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus)
    return;

  Finder->addMatcher(expr().bind("maybeExpr"), this);
}

static bool isUsedAsCallee(const Expr *E, ASTContext &Context) {
  const auto &Parents = Context.getParents(*E);
  if (Parents.empty())
    return false;

  const Stmt *ParentStmt = Parents[0].get<Stmt>();
  if (!ParentStmt)
    return false;

  if (const auto *Call = dyn_cast<CallExpr>(ParentStmt)) {
    const Expr *Callee = Call->getCallee()->IgnoreParenImpCasts();
    const Expr *Self = E->IgnoreParenImpCasts();
    return Callee == Self;
  }

  return false;
}

void DependentTemplateKeywordCheck::check(
    const MatchFinder::MatchResult &Result) {
  ASTContext &Ctx = *Result.Context;
  const SourceManager &SM = *Result.SourceManager;

  const auto *ExprNode = Result.Nodes.getNodeAs<clang::Expr>("maybeExpr");
  if (!ExprNode)
    return;

  // a->foo<T>() or a.foo<T>()
  if (const auto *CDSME = dyn_cast<CXXDependentScopeMemberExpr>(ExprNode)) {
    if (!CDSME->hasExplicitTemplateArgs() || (!isUsedAsCallee(CDSME, Ctx)) ||
        CDSME->getTemplateKeywordLoc().isValid())
      return;

    SourceLocation InsertLoc = getTemplateInsertLocAfterToken(
        CDSME->getOperatorLoc(), CDSME->isArrow() ? tok::arrow : tok::period,
        SM, Ctx.getLangOpts());

    if (InsertLoc.isInvalid())
      return;

    diag(CDSME->getBeginLoc(), "missing 'template' keyword before dependent "
                               "template member function call")
        << FixItHint::CreateInsertion(InsertLoc, "template ");
    return;
  }

  // A<T>::foo<T>()
  if (const auto *DSDR = dyn_cast<DependentScopeDeclRefExpr>(ExprNode)) {
    if (!DSDR->hasExplicitTemplateArgs() || (!isUsedAsCallee(DSDR, Ctx)) ||
        DSDR->getTemplateKeywordLoc().isValid())
      return;

    // find the last "::" location
    SourceLocation ColonColonLoc = DSDR->getQualifierLoc().getEndLoc();
    if (ColonColonLoc.isInvalid())
      return;

    SourceLocation InsertLoc = getTemplateInsertLocAfterToken(
        ColonColonLoc, tok::coloncolon, SM, Ctx.getLangOpts());

    if (InsertLoc.isInvalid())
      return;

    diag(DSDR->getNameInfo().getLoc(),
         "missing 'template' keyword before dependent template function")
        << FixItHint::CreateInsertion(InsertLoc, "template ");
    return;
  }
}
} // namespace clang::tidy::BSCompatibility