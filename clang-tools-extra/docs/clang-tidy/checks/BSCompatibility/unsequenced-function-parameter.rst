.. title:: clang-tidy - BSCompatibility-unsequenced-function-parameter

BSCompatibility-unsequenced-function-parameter
==============================================

Function calls as arguments are unsequenced and may cause dependency
issues. e.g.

```c
int global_value = 0;

int f1() {
  global_value += 10;
  std::cout << "f1 called, global_value=" << global_value << std::endl;
  return 1;
}

int f2() {
  global_value *= 2;
  std::cout << "f2 called, global_value=" << global_value << std::endl;
  return 2;
}

int add(int a, int b) {
  return b;
}

int main() {
  int b = add(f1(), f2());
  return 0;
}
```
in this case, the results of gcc and clang differ.

```
clang++ test.cpp && ./a.out
f1 called, global_value=10
f2 called, global_value=20

g++ test.cpp && ./a.out  # on x86
f2 called, global_value=0
f1 called, global_value=10
```

According to the C++ standard, the order of execution of function
parameters is not guaranteed, and multiple function parameters with
side effects result in undefined behavior.

This check detect multiple function parameters and provides
suggestions for extracting parameters from the function call.
