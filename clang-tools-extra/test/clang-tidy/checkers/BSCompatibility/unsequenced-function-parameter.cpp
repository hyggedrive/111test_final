// RUN: %check_clang_tidy %s BSCompatibility-unsequenced-function-parameter %t -- -fix-errors

int foo() {return 1;}
double foo2() {return 1.0;}
int barbar(int a, int b, double c) {return 1;}

double bar(double a, double b) {return 2.0;}

#define foofoo(first, second)

int main() {
    foofoo(foo(), foo()); // Ignore the macro func.
    barbar(foo(), foo(), 1.0);
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Function calls as arguments are unsequenced and may cause dependency issues [BSCompatibility-unsequenced-function-parameter]
// CHECK-FIXES: int __temp_0_0 = foo();
// CHECK-FIXES: int __temp_0_1 = foo();
// CHECK-FIXES: barbar(__temp_0_0, __temp_0_1, 1.0);
    barbar(foo(), foo(), bar(foo2(), foo2()));
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Function calls as arguments are unsequenced and may cause dependency issues [BSCompatibility-unsequenced-function-parameter]
// CHECK-FIXES: int __temp_1_0 = foo(); 
// CHECK-FIXES: int __temp_1_1 = foo(); 
// CHECK-FIXES: double __temp_1_2 = bar(foo2(), foo2()); 
// CHECK-FIXES: barbar(__temp_1_0, __temp_1_1, __temp_1_2);
// CHECK-MESSAGES: :[[@LINE-6]]:26: warning: Function calls as arguments are unsequenced and may cause dependency issues [BSCompatibility-unsequenced-function-parameter]

// bar(foo(), foo()) is not fixed during first check
}

// CHECK-NOT: instantiatedFunction
// CHECK-NOT: classTemplateMethod
// CHECK-NOT: usedFunction
// CHECK-NOT: declaredButNotDefined
// CHECK-NOT: systemTemplateFunction