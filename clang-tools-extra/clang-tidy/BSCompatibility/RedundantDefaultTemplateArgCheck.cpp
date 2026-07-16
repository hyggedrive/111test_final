//===--- RedundantDefaultTemplateArgCheck.cpp - clang-tidy ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RedundantDefaultTemplateArgCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang::tidy::BSCompatibility {

void RedundantDefaultTemplateArgCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(functionTemplateDecl().bind("funcTmpl"), this);
}

void RedundantDefaultTemplateArgCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *FuncTmpl =
      Result.Nodes.getNodeAs<FunctionTemplateDecl>("funcTmpl");
  if (!FuncTmpl ||
      Result.SourceManager->isInSystemHeader(FuncTmpl->getLocation()))
    return;

  auto &Recorded = FirstDefaultMap[FuncTmpl->getCanonicalDecl()];
  bool IsFirstSeen = !Recorded;
  for (unsigned i = 0; i < FuncTmpl->getTemplateParameters()->size(); ++i) {
    const auto *Param = dyn_cast<TemplateTypeParmDecl>(
        FuncTmpl->getTemplateParameters()->getParam(i));
    if (!Param || !Param->hasDefaultArgument())
      continue;
    if (IsFirstSeen) {
      Recorded = true;
      continue;
    }
    TypeSourceInfo *DefaultArgInfo = Param->getDefaultArgumentInfo();
    if (!DefaultArgInfo)
      continue;
    TemplateArgument DefaultArg(DefaultArgInfo->getType());
    TemplateArgumentLoc DefaultArgLoc(DefaultArg, DefaultArgInfo);
    if (DefaultArgLoc.getArgument().isNull())
      continue;
    SourceRange DefaultRange = DefaultArgLoc.getSourceRange();
    if (!DefaultRange.isValid())
      continue;
    SourceManager &SM = *Result.SourceManager;
    LangOptions LangOpts = Result.Context->getLangOpts();

    SourceLocation EndLoc = DefaultRange.getEnd();
    SourceLocation StartLoc = DefaultRange.getBegin();

    // search forward for `=`
    SourceLocation EqualLoc = StartLoc;
    Token Tok;
    SourceLocation SearchLoc = StartLoc;

    while (SearchLoc.isValid() && SM.isWrittenInSameFile(SearchLoc, StartLoc)) {
      SearchLoc = SearchLoc.getLocWithOffset(-1);
      if (Lexer::getRawToken(SearchLoc, Tok, SM, LangOpts, true))
        break;
      if (Tok.is(tok::equal)) {
        EqualLoc = Tok.getLocation();
        break;
      }
    }

    // scan forward for blank space
    SourceLocation RemovalBegin = EqualLoc;
    while (RemovalBegin.getRawEncoding() >
           Param->getLocation().getRawEncoding()) {
      char C = *SM.getCharacterData(RemovalBegin.getLocWithOffset(-1));
      if (C != ' ' && C != '\t')
        break;
      RemovalBegin = RemovalBegin.getLocWithOffset(-1);
    }

    // remove from RemovalBegin to end of DefaultRange
    CharSourceRange RemovalRange = CharSourceRange::getCharRange(
        RemovalBegin, Lexer::getLocForEndOfToken(EndLoc, 0, SM, LangOpts));

    diag(Param->getLocation(), "default template argument redefined; only the "
                               "first declaration should specify it")
        << FixItHint::CreateRemoval(RemovalRange);
  }
}

} // namespace clang::tidy::BSCompatibility