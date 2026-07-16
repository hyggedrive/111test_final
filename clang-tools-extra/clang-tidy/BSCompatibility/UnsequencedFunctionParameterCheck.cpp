//===--- NonVoidFunctionReturnVoidCheck.cpp - clang-tidy ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "UnsequencedFunctionParameterCheck.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang::tidy::BSCompatibility {

void UnsequencedFunctionParameterCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(callExpr(hasAnyArgument(callExpr())).bind("callee"), this);
}

void UnsequencedFunctionParameterCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *Callee = Result.Nodes.getNodeAs<clang::CallExpr>("callee");
  if (!Callee || Callee->getNumArgs() < 2)
    return;

  ASTContext *Context = Result.Context;
  SourceManager &SM = *Result.SourceManager;
  const LangOptions &LangOpts = Context->getLangOpts();

  std::vector<const CallExpr *> CallArgs;
  std::vector<FixItHint> FixIts;

  for (size_t i = 0; i < Callee->getNumArgs(); ++i) {
    if (const auto *CallArg = dyn_cast<CallExpr>(Callee->getArg(i))) {
      CallArgs.push_back(CallArg);
    }
  }

  if (CallArgs.size() < 2)
    return;

  std::string TempDecls = "\n";
  std::vector<std::string> TempVarNames;
  int TempCounter = 0;

  // Get the location for variable insertions.
  SourceLocation InsertLoc = findSafeInsertionPoint(Callee, SM, *Context);

  // Create insertions.
  for (const CallExpr *Call : CallArgs) {
    std::string VarName = (Twine("__temp_") + Twine(GlobalTempCounter) +
                           Twine("_") + Twine(TempCounter++))
                              .str();
    TempVarNames.push_back(VarName);

    QualType ReturnType = Call->getType();
    std::string TypeStr = ReturnType.getAsString();

    SourceRange CallRange = Call->getSourceRange();
    std::string CallText =
        Lexer::getSourceText(CharSourceRange::getTokenRange(CallRange), SM,
                             LangOpts)
            .str();

    TempDecls += "\t" + TypeStr + " " + VarName + " = " + CallText + ";\n";
  }

  if (!TempDecls.empty()) {
    FixIts.push_back(FixItHint::CreateInsertion(InsertLoc, TempDecls));
  }

  // Create replacements.
  TempCounter = 0;
  GlobalTempCounter++;
  for (size_t i = 0; i < Callee->getNumArgs(); ++i) {
    if (isa<CallExpr>(Callee->getArg(i))) {
      const Expr *Arg = Callee->getArg(i);
      FixIts.push_back(FixItHint::CreateReplacement(
          Arg->getSourceRange(), TempVarNames[TempCounter++]));
    }
  }

  // Print diaginfo.
  auto Diag =
      diag(Callee->getBeginLoc(), "Function calls as arguments are unsequenced "
                                  "and may cause dependency issues");

  for (const FixItHint &Fix : FixIts) {
    Diag << Fix;
  }
}

SourceLocation UnsequencedFunctionParameterCheck::findSafeInsertionPoint(
    const CallExpr *Callee, SourceManager &SM, ASTContext &Context) {
  if (!Callee) return SourceLocation();
  
  SourceLocation CalleeStart = Callee->getBeginLoc();
  if (CalleeStart.isInvalid()) return SourceLocation();

  FileID FID = SM.getFileID(CalleeStart);
  SourceLocation FileStart = SM.getLocForStartOfFile(FID);

  // Get source code before Callee.
  const char *CalleePtr = SM.getCharacterData(CalleeStart);
  const char *FilePtr = SM.getCharacterData(FileStart);
  if (!CalleePtr || !FilePtr) return SourceLocation();

  // Find the position after last ";", "{" or "}".
  const char *SearchPtr = CalleePtr - 1;
  while (SearchPtr >= FilePtr) {
    if (*SearchPtr == ';' || *SearchPtr == '{' || *SearchPtr == '}') {
      break;
    }
    SearchPtr--;
  }
  SourceLocation FoundLoc;
  if (SearchPtr >= FilePtr) {
    FoundLoc = FileStart.getLocWithOffset(SearchPtr - FilePtr);
  } else {
    FoundLoc = FileStart;
  }
  const char *AfterSemiBrace = SearchPtr + 1;
  while (*AfterSemiBrace && isspace(*AfterSemiBrace)) {
    AfterSemiBrace++;
  }

  SourceLocation LastStatementEnd =
      FileStart.getLocWithOffset(SearchPtr - FilePtr);
  SourceLocation StatementStart =
      FileStart.getLocWithOffset(AfterSemiBrace - FilePtr);
  // Check if last statment ends in the same line as this statment start.
  if (SM.getSpellingLineNumber(LastStatementEnd) !=
      SM.getSpellingLineNumber(StatementStart))
    return findPreviousLineEnd(StatementStart, SM);
  else
    return StatementStart;
}

SourceLocation
UnsequencedFunctionParameterCheck::findPreviousLineEnd(SourceLocation Loc,
                                                       SourceManager &SM) {
  if (Loc.isInvalid())
    return Loc;

  unsigned Line = SM.getSpellingLineNumber(Loc);
  if (Line <= 1) {
    return SM.getLocForStartOfFile(SM.getFileID(Loc));
  }

  FileID FID = SM.getFileID(Loc);
  SourceLocation PreviousLine = SM.translateLineCol(FID, Line - 1, 1);
  SourceLocation CurrentLine = SM.translateLineCol(FID, Line, 1);
  if (PreviousLine.isInvalid() || CurrentLine.isInvalid()) {
    return Loc;
  }

  SourceLocation Ret = CurrentLine.getLocWithOffset(-1);
  return Ret.isValid() ? Ret : Loc;
}

} // namespace clang::tidy::BSCompatibility
