set(LLVM_LINK_COMPONENTS
  support
  FrontendOpenMP
  )

add_clang_library(clangTidyBSCompatibilityModule
  BSCompatibilityTidyModule.cpp
  ForbiddenBuiltinExitCheck.cpp
  DependentTemplateKeywordCheck.cpp
  MoveExplicitInstantiationAfterDefsCheck.cpp
  NonVoidFunctionReturnVoidCheck.cpp
  RedundantDefaultTemplateArgCheck.cpp
  ThreadStorageUnifyCheck.cpp
  UnsequencedFunctionParameterCheck.cpp

  LINK_LIBS
  clangTidy
  clangTidyCppCoreGuidelinesModule
  clangTidyUtils

  DEPENDS
  omp_gen
  )

clang_target_link_libraries(clangTidyBSCompatibilityModule
  PRIVATE
  clangAnalysis
  clangAnalysisFlowSensitive
  clangAnalysisFlowSensitiveModels
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTooling
  clangTransformer
  )
