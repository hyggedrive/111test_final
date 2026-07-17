# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t.o
# RUN: ld.lld --riscv-function-sections-split --print-riscv-function-sections-split %t.o -o %t 2>&1 | count 0

.globl _start
.type _start,@function
_start:
  ret
.size _start, .-_start
