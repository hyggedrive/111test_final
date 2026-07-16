// RUN: %check_clang_tidy %s BSCompatibility-forbidden-builtin-exit %t -- -fix-errors

void catchEx() {
  __builtin_exit(0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: __builtin_exit is not supported by Clang; you may use std::exit() [BSCompatibility-forbidden-builtin-exit]
  // CHECK-MESSAGES: :[[@LINE-2]]:3: error: use of undeclared identifier '__builtin_exit' [clang-diagnostic-error]
  try {
  } catch (int) {
  }
}

int main() { 
  __builtin_exit(1);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: __builtin_exit is not supported by Clang; you may use std::exit() [BSCompatibility-forbidden-builtin-exit]
  // CHECK-MESSAGES: :[[@LINE-2]]:3: error: use of undeclared identifier '__builtin_exit' [clang-diagnostic-error]
  }