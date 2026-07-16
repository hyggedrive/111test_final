//===--- ForbiddenBuiltinExitCheck.cpp - clang-tidy -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ForbiddenBuiltinExitCheck.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Token.h"
#include "llvm/ADT/StringRef.h"

using namespace clang::ast_matchers;

namespace clang::tidy::BSCompatibility {
void ForbiddenBuiltinExitCheck::registerMatchers(MatchFinder *Finder) {}

void ForbiddenBuiltinExitCheck::check(const MatchFinder::MatchResult &Result) {}

void ForbiddenBuiltinExitCheck::registerPPCallbacks(
    const SourceManager &SM, Preprocessor *PP, Preprocessor *ModuleExpanderPP) {
  class ForbiddenBuiltinExitPPCallback : public PPCallbacks {
  public:
    ForbiddenBuiltinExitPPCallback(ClangTidyCheck &Check,
                                   const SourceManager &SM, Preprocessor &PP)
        : Check(Check), SM(SM), PP(PP) {}

    void FileChanged(SourceLocation Loc, FileChangeReason Reason,
                     SrcMgr::CharacteristicKind, FileID PrevFID) override {
      if (Reason != EnterFile)
        return;

      FileID FID = SM.getFileID(Loc);
      bool Invalid = false;
      StringRef Code = SM.getBufferData(FID, &Invalid);
      if (Invalid)
        return;

      LangOptions LangOpts;
      Lexer Lex(SM.getLocForStartOfFile(FID), LangOpts, Code.begin(),
                Code.begin(), Code.end());

      Token Tok;
      while (!Lex.LexFromRawLexer(Tok)) {
        if (Tok.is(tok::raw_identifier)) {
          IdentifierInfo &II =
              PP.getIdentifierTable().get(Tok.getRawIdentifier());
          Tok.setIdentifierInfo(&II);
          Tok.setKind(II.getTokenID());

          if (II.getName() == "__builtin_exit") {
            Check.diag(Tok.getLocation(), "__builtin_exit is not supported by "
                                          "Clang; you may use std::exit()");
          }
        }
      }
    }

  private:
    ClangTidyCheck &Check;
    const SourceManager &SM;
    Preprocessor &PP;
  };

  PP->addPPCallbacks(
      std::make_unique<ForbiddenBuiltinExitPPCallback>(*this, SM, *PP));
}

} // namespace clang::tidy::BSCompatibility