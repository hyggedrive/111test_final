//===--- BSCompatibilityTidyModule.cpp - clang-tidy --------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "../cppcoreguidelines/NarrowingConversionsCheck.h"
#include "DependentTemplateKeywordCheck.h"
#include "ForbiddenBuiltinExitCheck.h"
#include "MoveExplicitInstantiationAfterDefsCheck.h"
#include "NonVoidFunctionReturnVoidCheck.h"
#include "RedundantDefaultTemplateArgCheck.h"
#include "ThreadStorageUnifyCheck.h"
#include "UnsequencedFunctionParameterCheck.h"

namespace clang::tidy {
namespace BSCompatibility {

class BSCompatibilityModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<ForbiddenBuiltinExitCheck>(
        "BSCompatibility-forbidden-builtin-exit");
    CheckFactories.registerCheck<DependentTemplateKeywordCheck>(
        "BSCompatibility-dependent-template-keyword");
    CheckFactories.registerCheck<MoveExplicitInstantiationAfterDefsCheck>(
        "BSCompatibility-move-explicit-instantiation-after-defs");
    CheckFactories.registerCheck<NonVoidFunctionReturnVoidCheck>(
        "BSCompatibility-non-void-function-return-void");
    CheckFactories.registerCheck<RedundantDefaultTemplateArgCheck>(
        "BSCompatibility-redundant-default-template-arg");
    CheckFactories.registerCheck<ThreadStorageUnifyCheck>(
        "BSCompatibility-thread-storage-unify");
    CheckFactories.registerCheck<UnsequencedFunctionParameterCheck>(
        "BSCompatibility-unsequenced-function-parameter");
  }
};

} // namespace BSCompatibility

// Register the BSCompatibilityModuleRegistry using this statically initsialized variable.
static ClangTidyModuleRegistry::Add<BSCompatibility::BSCompatibilityModule>
    X("BSCompatibility-module", "Adds checks for BiSheng compatibility code constructs.");

// This anchor is used to force the linker to link in the generated object file
// and thus register the BSCompatibilityModule.
volatile int BSCompatibilityModuleAnchorSource = 0;

} // namespace clang::tidy
