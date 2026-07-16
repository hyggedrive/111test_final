.. title:: clang-tidy - BSCompatibility-non-void-function-return-void

BSCompatibility-non-void-function-return-void
=============================================

This check detects the issue that a nonvoid function return void.
e.g.
int f(){}
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: Non-void function does not return a val. [BSCompatibility-non-void-function-return-void]

A warning will be proposed.