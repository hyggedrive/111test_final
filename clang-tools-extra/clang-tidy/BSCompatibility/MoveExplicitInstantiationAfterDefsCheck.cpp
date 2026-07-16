//===--- MoveExplicitInstantiationAfterDefsCheck.cpp - clang-tidy ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MoveExplicitInstantiationAfterDefsCheck.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TokenKinds.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/Hashing.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tidy;

namespace clang::tidy::BSCompatibility {

namespace {

static SourceLocation endOfToken(const SourceManager &SM, const LangOptions &LO,
                                 SourceLocation Loc) {
  if (!Loc.isValid())
    return Loc;
  SourceLocation End = Lexer::getLocForEndOfToken(Loc, 0, SM, LO);
  return End.isValid() ? End : Loc;
}

// Primary key = primary class template's canonical decl + printable name.
static const NamedDecl *
getPrimaryFromClassSpec(const ClassTemplateSpecializationDecl *Spec) {
  if (!Spec)
    return nullptr;
  if (const auto *CTD = Spec->getSpecializedTemplate())
    return llvm::cast<NamedDecl>(CTD->getTemplatedDecl()->getCanonicalDecl());
  return nullptr;
}

static const NamedDecl *getPrimaryFromMethod(const CXXMethodDecl *MD) {
  if (!MD)
    return nullptr;
  const auto *RD = MD->getParent();
  if (const auto *CTD = RD->getDescribedClassTemplate())
    return llvm::cast<NamedDecl>(CTD->getTemplatedDecl()->getCanonicalDecl());
  if (const auto *Spec = dyn_cast<ClassTemplateSpecializationDecl>(RD))
    if (const auto *CTD2 = Spec->getSpecializedTemplate())
      return llvm::cast<NamedDecl>(
          CTD2->getTemplatedDecl()->getCanonicalDecl());
  return nullptr;
}

} // namespace

MoveExplicitInstantiationAfterDefsCheck::Key
MoveExplicitInstantiationAfterDefsCheck::buildKey(
    const Decl *D, const MatchFinder::MatchResult &R) const {
  const NamedDecl *Primary = nullptr;

  if (const auto *Spec = dyn_cast<ClassTemplateSpecializationDecl>(D)) {
    Primary = getPrimaryFromClassSpec(Spec);
  } else if (const auto *FD = dyn_cast<FunctionDecl>(D)) {
    if (const auto *MD = dyn_cast<CXXMethodDecl>(FD))
      Primary = getPrimaryFromMethod(MD);
  } else if (const auto *ND = dyn_cast<NamedDecl>(D)) {
    Primary = llvm::cast<NamedDecl>(ND->getCanonicalDecl());
  }

  // Stable printable signature: fully qualified primary template name.
  std::string Sig;
  if (const auto *N = dyn_cast_or_null<NamedDecl>(Primary)) {
    PrintingPolicy PP(R.Context->getLangOpts());
    PP.SuppressTagKeyword = true;
    PP.FullyQualifiedName = true;
    llvm::raw_string_ostream OS(Sig);
    N->printQualifiedName(OS);
    OS.flush();
  }
  Key K;
  K.Primary = Primary;
  K.Mangle = std::move(Sig);
  return K;
}

void MoveExplicitInstantiationAfterDefsCheck::registerMatchers(MatchFinder *F) {
  // 1) Class template explicit instantiation (we'll filter to definition-form).
  F->addMatcher(classTemplateSpecializationDecl().bind("inst_cls"), this);
  // 2) Out-of-line member function definitions (filter by FD->isOutOfLine()).
  F->addMatcher(functionDecl(isDefinition()).bind("def"), this);
}

void MoveExplicitInstantiationAfterDefsCheck::check(
    const MatchFinder::MatchResult &R) {
  if (!Ctx)
    Ctx = R.Context;
  const SourceManager &SM = *R.SourceManager;
  const LangOptions &LO = R.Context->getLangOpts();

  // (A) Capture explicit instantiation definition.
  if (const auto *C =
          R.Nodes.getNodeAs<ClassTemplateSpecializationDecl>("inst_cls")) {
    if (!C->getBeginLoc().isValid() || SM.isInSystemHeader(C->getLocation()) ||
        // not explicit instantiation definition
        C->getSpecializationKind() != TSK_ExplicitInstantiationDefinition)
      return;

    Key K = buildKey(C, R);
    auto &I = Map[K];
    if (!I.InstDecl ||
        SM.isBeforeInTranslationUnit(C->getBeginLoc(), I.InstBegin)) {
      I.InstDecl = C;
      I.InstBegin = C->getBeginLoc();
      I.InstEndToken = endOfToken(SM, LO, C->getEndLoc());
    }
    return;
  }

  // (B) Capture out-of-line member definitions of class templates.
  if (const auto *FD = R.Nodes.getNodeAs<FunctionDecl>("def")) {
    if (!FD->getBeginLoc().isValid() ||
        SM.isInSystemHeader(FD->getLocation()) || !FD->isOutOfLine())
      return;

    const auto *MD = dyn_cast<CXXMethodDecl>(FD);
    if (!MD)
      return;

    // Only class templates (primary or specialization).
    if (!getPrimaryFromMethod(MD))
      return;

    Key K = buildKey(FD, R);
    auto &I = Map[K];

    SourceLocation End = endOfToken(SM, LO, FD->getEndLoc());
    if (!I.LastDefEndToken.isValid() ||
        SM.isBeforeInTranslationUnit(I.LastDefEndToken, End))
      I.LastDefEndToken = End;
    return;
  }
}

void MoveExplicitInstantiationAfterDefsCheck::onEndOfTranslationUnit() {
  if (Map.empty() || !Ctx)
    return;

  const SourceManager &SM = Ctx->getSourceManager();
  const LangOptions &LO = Ctx->getLangOpts();

  for (const auto &E : Map) {
    const Info &I = E.second;
    if (!I.InstBegin.isValid() || !I.InstEndToken.isValid() ||
        !I.LastDefEndToken.isValid() || !I.InstDecl)
      continue;

    // Already after the last definition, then leave it alone
    if (SM.isBeforeInTranslationUnit(I.LastDefEndToken, I.InstBegin))
      continue;

    // Calculate the deletion range from the beginning of the
    // entire line to after the semicolon
    SourceLocation B = I.InstBegin;
    SourceLocation EOT = I.InstEndToken;

    // Beginning of line (deleting the entire line is more stable)
    SourceLocation SpellB = SM.getSpellingLoc(B);
    FileID FID = SM.getFileID(SpellB);
    unsigned Line = SM.getSpellingLineNumber(SpellB);
    SourceLocation LineBegin = SM.translateLineCol(FID, Line, 1);
    if (LineBegin.isInvalid())
      LineBegin = B;

    // Find a position after the trailing semicolon
    SourceLocation AfterSemi = Lexer::findLocationAfterToken(
        EOT, tok::semi, SM, LO, /*SkipTrailingWhitespaceAndNewLine=*/true);

    // Backward scanning: Look forward at most 16 tokens until a ';'
    // is encountered
    if (!AfterSemi.isValid()) {
      SourceLocation Scan = EOT;
      for (int i = 0; i < 16 && Scan.isValid(); ++i) {
        Token T;
        SourceLocation Beg = SM.getSpellingLoc(Scan);
        if (Lexer::getRawToken(Beg, T, SM, LO))
          break;
        if (T.is(tok::semi)) {
          AfterSemi = T.getEndLoc().isValid() ? T.getEndLoc() : EOT;
          break;
        }
        Scan = T.getEndLoc();
      }
    }

    CharSourceRange Rng = CharSourceRange::getCharRange(LineBegin, AfterSemi);
    llvm::StringRef InstText = Lexer::getSourceText(Rng, SM, LO).ltrim();

    // Only move the lines that look like "template ..."
    if (!InstText.starts_with("template"))
      continue;

    // Construct the inserted text, making sure to end it with ';'
    std::string Insert;
    Insert.push_back('\n');
    Insert.append(InstText.data(), InstText.size());

    if (!llvm::StringRef(Insert).rtrim().ends_with(";"))
      Insert.push_back(';');
    Insert.push_back('\n');

    SourceLocation InstBeginFile = SM.getFileLoc(I.InstBegin);
    SourceLocation InsertAnchor =
        Lexer::getLocForEndOfToken(SM.getFileLoc(I.LastDefEndToken), 0, SM, LO);

    if (!SM.isWrittenInMainFile(InsertAnchor))
      InsertAnchor = SM.getFileLoc(I.LastDefEndToken);

    // make fixes
    {
      auto D =
          diag(InsertAnchor,
               "explicit instantiation should appear after the out-of-line "
               "member definition(s) in this translation unit");
      D << FixItHint::CreateRemoval(Rng);

      D << FixItHint::CreateInsertion(InsertAnchor, Insert);
    }
    // make notes
    diag(InstBeginFile, "suggest to remove this explicit instantiation here",
         DiagnosticIDs::Note);
    diag(InsertAnchor, "suggest to insert the explicit instantiation here",
         DiagnosticIDs::Note);
  }
}
} // namespace clang::tidy::BSCompatibility