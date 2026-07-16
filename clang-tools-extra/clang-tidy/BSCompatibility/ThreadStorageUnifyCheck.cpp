//===--- ThreadStorageUnifyCheck.cpp - clang-tidy -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "ThreadStorageUnifyCheck.h"

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/SmallVector.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace BSCompatibility {

void ThreadStorageUnifyCheck::registerMatchers(MatchFinder *Finder) {
  // Broadly matches variable declarations stored in global storage;
  // whether it is TLS is filtered in check()
  Finder->addMatcher(varDecl(hasGlobalStorage(), hasThreadStorageDuration(),
                             unless(isExpansionInSystemHeader()))
                         .bind("vardecl"),
                     this);
}

void ThreadStorageUnifyCheck::findTLSKeywordToken(
    const VarDecl *VD, const SourceManager &SM, const LangOptions &LangOpts,
    bool &HasThreadLocal, bool &HasGNUThread, SourceLocation &TokLoc,
    bool &UnderMacro) {
  HasThreadLocal = false;
  HasGNUThread = false;
  TokLoc = VD->getBeginLoc();
  UnderMacro = VD->getBeginLoc().isMacroID();

  // Replacement in macros is riskier: diagnostics are still allowed, but the
  // token position is not returned for fix-it
  if (UnderMacro)
    return;

  SourceLocation Begin = VD->getBeginLoc();
  SourceLocation Name = VD->getLocation();
  if (Begin.isInvalid() || Name.isInvalid())
    return;

  CharSourceRange Range = CharSourceRange::getCharRange(Begin, Name);
  if (Range.isInvalid())
    return;

  StringRef Text = Lexer::getSourceText(Range, SM, LangOpts);

  // Directly search for the literal value to locate the
  // starting position of the keyword
  size_t pos = Text.find("thread_local");
  if (pos != StringRef::npos) {
    HasThreadLocal = true;
    TokLoc = Begin.getLocWithOffset(static_cast<int>(pos));
    return;
  }

  pos = Text.find("__thread");
  if (pos != StringRef::npos) {
    HasGNUThread = true;
    TokLoc = Begin.getLocWithOffset(static_cast<int>(pos));
    return;
  }
}

ThreadStorageUnifyCheck::TargetKind
ThreadStorageUnifyCheck::decideTarget(ArrayRef<DeclInfo> Infos, bool InC,
                                      const SourceManager &SM) {
  if (InC)
    return TargetKind::TL___thread;

  // Whenever non-extern (or defined) occurs, unify to __thread (GNU caliber)
  for (const auto &I : Infos) {
    if (!I.IsExtern || I.IsDefinition)
      return TargetKind::TL___thread;
  }

  // All are extern: the actual keyword written in the
  // "last extern declaration in the source order" shall prevail
  const DeclInfo *Last = nullptr;
  for (const auto &I : Infos) {
    if (!I.IsExtern)
      continue;
    if (!Last || SM.isBeforeInTranslationUnit(Last->VD->getBeginLoc(),
                                              I.VD->getBeginLoc()))
      Last = &I;
  }
  if (!Last)
    return TargetKind::Unknown;

  if (Last->SpelledGNUThread)
    return TargetKind::TL___thread;
  if (Last->SpelledThreadLocal)
    return TargetKind::TL_thread_local;

  // Fallback strategy: prefer __thread when unrecognizable (closer to GNU)
  return TargetKind::TL___thread;
}

static StringRef reasonToText(ThreadStorageUnifyCheck::UnifyReason R,
                              ThreadStorageUnifyCheck::TargetKind TK) {
  switch (R) {
  case ThreadStorageUnifyCheck::UnifyReason::NonExternOrDefinition:
    return (TK == ThreadStorageUnifyCheck::TargetKind::TL___thread)
               ? "unify to GNU '__thread' for non-extern or defined "
                 "thread-local variable"
               : "unify to 'thread_local' for non-extern or defined "
                 "thread-local variable";
  case ThreadStorageUnifyCheck::UnifyReason::LastExtern:
    return (TK == ThreadStorageUnifyCheck::TargetKind::TL___thread)
               ? "unify to GNU '__thread' to match the last extern declaration"
               : "unify to 'thread_local' to match the last extern declaration";
  case ThreadStorageUnifyCheck::UnifyReason::MatchRedecls:
    return (TK == ThreadStorageUnifyCheck::TargetKind::TL___thread)
               ? "unify to GNU '__thread' to match redeclarations"
               : "unify to 'thread_local' to match redeclarations";
  default:
    return (TK == ThreadStorageUnifyCheck::TargetKind::TL___thread)
               ? "unify to GNU '__thread'"
               : "unify to 'thread_local'";
  }
}

void ThreadStorageUnifyCheck::applyFixes(ArrayRef<DeclInfo> Infos,
                                         TargetKind TK,
                                         const SourceManager & /*SM*/,
                                         UnifyReason Reason,
                                         DiagnosticBuilder *AttachTo) {
  StringRef Want =
      (TK == TargetKind::TL___thread) ? "__thread" : "thread_local";
  StringRef ReasonText = reasonToText(Reason, TK);

  for (const auto &I : Infos) {
    if (I.SpelledUnderMacro)
      continue; // no fix in macro

    auto needReplace =
        (I.SpelledThreadLocal && TK == TargetKind::TL___thread) ||
        (I.SpelledGNUThread && TK == TargetKind::TL_thread_local);
    if (!needReplace)
      continue;

    // Calculate replacement interval
    const int Len =
        I.SpelledThreadLocal ? 12 : 8; // "thread_local" / "__thread"
    auto SR = CharSourceRange::getCharRange(I.TLTokenLoc,
                                            I.TLTokenLoc.getLocWithOffset(Len));
    auto Hint = FixItHint::CreateReplacement(SR, Want);

    if (AttachTo) {
      // Non-mixed use scenarios: attach the fix to
      // the "existing summary" warning
      (*AttachTo) << Hint;
    } else {
      // Maintaining old behavior in mixed scenarios
      diag(I.TLTokenLoc, ReasonText) << Hint;
    }
  }
}

void ThreadStorageUnifyCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *VD = Result.Nodes.getNodeAs<VarDecl>("vardecl");
  if (!VD)
    return;

  // Only execute once on the specification declaration to avoid duplication
  const VarDecl *Canon = VD->getCanonicalDecl();
  if (VD != Canon)
    return;

  const SourceManager &SM = *Result.SourceManager;
  const LangOptions &LO = Result.Context->getLangOpts();

  llvm::SmallVector<DeclInfo> Infos;
  for (const VarDecl *D : Canon->redecls()) {
    DeclInfo I;
    I.VD = D;
    I.IsExtern = (D->getStorageClass() == SC_Extern);
    I.IsDefinition = D->isThisDeclarationADefinition();
    findTLSKeywordToken(D, SM, LO, I.SpelledThreadLocal, I.SpelledGNUThread,
                        I.TLTokenLoc, I.SpelledUnderMacro);
    Infos.push_back(I);
  }
  if (Infos.empty())
    return;

  const bool InC = !LO.CPlusPlus;
  TargetKind TK = decideTarget(Infos, InC, SM);
  if (TK == TargetKind::Unknown)
    return;

  // Determine if already consistent with the target spelling
  bool AllOk = true;
  bool SeenTL = false, SeenGNU = false;
  bool HasNonExternOrDef = false;
  for (const auto &I : Infos) {
    SeenTL |= I.SpelledThreadLocal;
    SeenGNU |= I.SpelledGNUThread;
    HasNonExternOrDef |= (!I.IsExtern || I.IsDefinition);
    if (TK == TargetKind::TL___thread && !I.SpelledGNUThread) {
      AllOk = false;
    }
    if (TK == TargetKind::TL_thread_local && !I.SpelledThreadLocal) {
      AllOk = false;
    }
  }
  if (AllOk)
    return;

  // Decide reason for unification (affects wording)
  UnifyReason Reason = UnifyReason::Unknown;
  if (SeenTL && SeenGNU) {
    // Mixed spellings across redeclarations.
    // If non-extern/def exists and target is __thread, make that explicit.
    if (HasNonExternOrDef && TK == TargetKind::TL___thread)
      Reason = UnifyReason::NonExternOrDefinition;
    else
      Reason = (TK == TargetKind::TL_thread_local) ? UnifyReason::LastExtern
                                                   : UnifyReason::MatchRedecls;

    // 1) summary
    if (TK == TargetKind::TL___thread) {
      diag(Infos.front().VD->getLocation(),
           "mixed use of '__thread' and 'thread_local' for the same variable; "
           "unifying to GNU '__thread'");
    } else {
      diag(Infos.front().VD->getLocation(),
           "mixed use of '__thread' and 'thread_local' for the same variable; "
           "unifying to 'thread_local' per the last extern declaration");
    }
    // 2) Send unify everywhere
    applyFixes(Infos, TK, SM, Reason, /*AttachTo=*/nullptr);
  } else {
    // Not mixed, but still needs change (e.g. single non-extern 'thread_local')
    if (HasNonExternOrDef && TK == TargetKind::TL___thread)
      Reason = UnifyReason::NonExternOrDefinition;
    else
      Reason = UnifyReason::LastExtern; // safe default for extern-only cases

    // Non-mixed: only one summary is created and all the FixIts
    // above are attached to it.
    StringRef Head = reasonToText(Reason, TK);
    const DeclInfo *Anchor = nullptr;
    for (const auto &I : Infos) {
      if ((TK == TargetKind::TL___thread && I.SpelledThreadLocal) ||
          (TK == TargetKind::TL_thread_local && I.SpelledGNUThread)) {
        Anchor = &I;
        break;
      }
    }
    if (!Anchor)
      Anchor = &Infos.front();

    auto DB = diag(Anchor->VD->getLocation(), Head);
    applyFixes(Infos, TK, SM, Reason, /*AttachTo=*/&DB);
  }
}

} // namespace BSCompatibility
} // namespace tidy
} // namespace clang