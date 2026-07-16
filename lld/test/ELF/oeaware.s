# REQUIRES: target=aarch64{{.*}}
# RUN: llvm-mc -filetype=obj -triple=aarch64-none-linux-gnu %s -o %t.o
# RUN: ld.lld %t.o -o %ta -z oeaware-policy=1
# RUN: llvm-objdump -j .LLVM4OE_oeAware %ta -d | FileCheck %s

# CHECK: <.LLVM4OE_oeAware>:
# CHECK-NEXT: 00000001

# RUN: ld.lld %t.o -o %tb -z oeaware-policy=2
# RUN: llvm-objdump -j .LLVM4OE_oeAware %tb -d | FileCheck %s --check-prefix=OPTION

# OPTION: <.LLVM4OE_oeAware>:
# OPTION-NEXT: 00000002

.globl _start
_start: nop
.bss; .byte 0
