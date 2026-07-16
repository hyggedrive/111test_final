// RUN: %check_clang_tidy %s BSCompatibility-non-void-function-return-void %t -- -fix-errors

// triggers the check here.
int f(){}
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: Non-void function does not return a val. [BSCompatibility-non-void-function-return-void]
// CHECK-FIXES: void f(){return something;}

// doesn't trigger the check here.
void f1();
int f2(){ return 0; }
int main(){}
