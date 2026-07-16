//===--- ThreadStorageUnifyCheck.h - clang-tidy -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_THREAD_STORAGE_UNIFY_CHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_THREAD_STORAGE_UNIFY_CHECK_H

#include "../ClangTidyCheck.h"

namespace clang {

class DiagnosticBuilder;
namespace tidy {
namespace BSCompatibility {

class ThreadStorageUnifyCheck : public ClangTidyCheck {
public:
  ThreadStorageUnifyCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  enum class TargetKind { Unknown, TL_thread_local, TL___thread };
  struct DeclInfo {
    const VarDecl *VD = nullptr;
    bool IsExtern = false;
    bool IsDefinition = false;
    bool SpelledThreadLocal = false;
    bool SpelledUnderMacro = false;
    SourceLocation TLTokenLoc;
    bool SpelledGNUThread = false;
  };

  // the reason to distinct the warning message
  enum class UnifyReason {
    MatchRedecls,
    LastExtern,
    NonExternOrDefinition,
    Unknown
  };

  static void findTLSKeywordToken(const VarDecl *VD, const SourceManager &SM,
                                  const LangOptions &LangOpts,
                                  bool &HasThreadLocal, bool &HasGNUThread,
                                  SourceLocation &TokLoc, bool &UnderMacro);

  static TargetKind decideTarget(ArrayRef<DeclInfo> Infos, bool InC,
                                 const SourceManager &SM);

  void applyFixes(ArrayRef<DeclInfo> Infos, TargetKind TK,
                  const SourceManager &SM, UnifyReason Reason,
                  DiagnosticBuilder *AttachTo = nullptr);
};

} // namespace BSCompatibility
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_THREADSTORAGEUNIFYCHECK_H