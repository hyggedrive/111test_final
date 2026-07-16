//===--- MoveExplicitInstantiationAfterDefsCheck.h - clang-tidy -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_MOVEEXPLICITINSTANTIATIONAFTERDEFSCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_MOVEEXPLICITINSTANTIATIONAFTERDEFSCHECK_H

#include "../ClangTidyCheck.h"

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/ADT/DenseMap.h"
#include <string>

namespace clang {
namespace tidy {
namespace BSCompatibility {

class MoveExplicitInstantiationAfterDefsCheck : public ClangTidyCheck {
public:
  using ClangTidyCheck::ClangTidyCheck;

  // Register matchers for:
  //  - class/var/function template explicit instantiations, and
  //  - out-of-line function/method definitions.
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;

  // Collect matches and record their source locations for later rewrite.
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  // At TU end, decide whether any explicit instantiation must be moved
  // and emit FixIts (remove + re-insert after the last definition).
  void onEndOfTranslationUnit() override;

  struct Key {
    const NamedDecl *Primary = nullptr;
    std::string Mangle;
    bool operator==(const Key &O) const {
      return Primary == O.Primary && Mangle == O.Mangle;
    }
  };

  struct KeyInfo {
    static inline Key getEmptyKey() {
      return {reinterpret_cast<const NamedDecl *>(1), ""};
    }
    static inline Key getTombstoneKey() {
      return {reinterpret_cast<const NamedDecl *>(2), ""};
    }
    static unsigned getHashValue(const Key &K) {
      return llvm::hash_combine(K.Primary, K.Mangle);
    }
    static bool isEqual(const Key &A, const Key &B) { return A == B; }
  };

  struct Info {
    const Decl *InstDecl = nullptr; // the decl to move
    SourceLocation InstBegin;       // begin of explicit instantiation
    SourceLocation InstEndToken;    // end-of-token
    SourceLocation
        LastDefEndToken; // end-of-token of the last related out-of-line def
  };

  // Build the Key for a given declaration (instantiation or definition).
  Key buildKey(const Decl *D,
               const ast_matchers::MatchFinder::MatchResult &R) const;

  llvm::DenseMap<Key, Info, KeyInfo> Map;
  // Cache ASTContext pointer because older tidy bases don't
  // expose getASTContext().
  clang::ASTContext *Ctx = nullptr;
};

} // namespace BSCompatibility
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BSCOMPATIBILITY_MOVEEXPLICITINSTANTIATIONAFTERDEFSCHECK_H