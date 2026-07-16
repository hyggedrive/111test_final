.. title:: clang-tidy - BSCompatibility-forbidden-builtin-exit

BSCompatibility-forbidden-builtin-exit
======================================

Detect the use of __builtin_exit, which is not supported by clang.and 
propose warning.
e.g.
void catchEx() {
  __builtin_exit(0);
  try {
  } catch (int) {
  }
}

int main() { __builtin_exit(1); }
warning:__builtin_exit is not supported by Clang; you may use std::exit()