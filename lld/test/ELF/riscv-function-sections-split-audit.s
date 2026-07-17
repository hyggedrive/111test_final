# REQUIRES: riscv

# RUN: llvm-mc -filetype=obj -triple=riscv32-unknown-elf %s -o %t.rv32.o
# RUN: ld.lld --riscv-function-sections-split --print-riscv-function-sections-split %t.rv32.o -o %t.rv32 2>&1 | FileCheck %s
# RUN: ld.lld --riscv-function-sections-split %t.rv32.o -o %t.no-print 2>&1 | count 0

# RUN: llvm-mc -filetype=obj -triple=riscv64-unknown-elf %s -o %t.rv64.o
# RUN: ld.lld --riscv-function-sections-split --print-riscv-function-sections-split %t.rv64.o -o %t.rv64 2>&1 | count 0

# CHECK:      riscv-function-sections-split: parent section: .text.safe
# CHECK:      riscv-function-sections-split: status: safe
# CHECK:      riscv-function-sections-split: block reasons:

# CHECK:      riscv-function-sections-split: parent section: .text.overlap
# CHECK:      riscv-function-sections-split: status: unsafe
# CHECK:      riscv-function-sections-split: block reasons: overlapping-functions

# CHECK:      riscv-function-sections-split: parent section: .text.zero
# CHECK:      riscv-function-sections-split: status: unsafe
# CHECK:      riscv-function-sections-split: block reasons: zero-sized-only

# CHECK:      riscv-function-sections-split: parent section: .text.nofunc
# CHECK:      riscv-function-sections-split: status: unsafe
# CHECK:      riscv-function-sections-split: block reasons: no-function-ranges

# CHECK:      riscv-function-sections-split: parent section: .text.jal
# CHECK:      riscv-function-sections-split: block reasons: no-reloc-cross-function-jal

# CHECK:      riscv-function-sections-split: parent section: .text.branch
# CHECK:      riscv-function-sections-split: block reasons: function-fallthrough,no-reloc-cross-function-branch

# CHECK:      riscv-function-sections-split: parent section: .text.rvc_zero
# CHECK:      riscv-function-sections-split: block reasons: no-reloc-cross-function-jal

# CHECK:      riscv-function-sections-split: parent section: .text.fallthrough
# CHECK:      riscv-function-sections-split: block reasons: function-fallthrough

# CHECK:      riscv-function-sections-split: parent section: .text.jalr
# CHECK:      riscv-function-sections-split: block reasons: computed-jump,function-fallthrough

# CHECK:      riscv-function-sections-split: parent section: .text.call
# CHECK:      riscv-function-sections-split: status: safe

# CHECK:      riscv-function-sections-split: parent section: .text.call_norelax
# CHECK:      riscv-function-sections-split: status: safe

# CHECK:      riscv-function-sections-split: parent section: .text.align
# CHECK:      riscv-function-sections-split: block reasons: align-crosses-function,source-relocation-unowned

# CHECK:      riscv-function-sections-split: parent section: .text.pcrel
# CHECK:      riscv-function-sections-split: block reasons: pcrel-pair-crosses-function

# CHECK:      riscv-function-sections-split: parent section: .text.source_sec
# CHECK:      riscv-function-sections-split: block reasons: source-section-symbol

# CHECK:      riscv-function-sections-split: parent section: .text.source_addend
# CHECK:      riscv-function-sections-split: block reasons: source-addend-crosses-piece

# CHECK:      riscv-function-sections-split: parent section: .text.incoming_sec
# CHECK:      riscv-function-sections-split: block reasons: incoming-section-symbol

# CHECK:      riscv-function-sections-split: parent section: .text.incoming_addend
# CHECK:      riscv-function-sections-split: block reasons: incoming-addend-crosses-piece

# CHECK:      riscv-function-sections-split: parent section: .text.eh
# CHECK:      riscv-function-sections-split: block reasons: incoming-eh-frame

# CHECK:      riscv-function-sections-split: parent section: .text.debug_target
# CHECK:      riscv-function-sections-split: block reasons: incoming-debug-relocation

# CHECK:      riscv-function-sections-split: parent section: .text.gap
# CHECK:      riscv-function-sections-split: block reasons: unexplained-gap

# CHECK:      riscv-function-sections-split: parent section: .text.symbol_cross
# CHECK:      riscv-function-sections-split: block reasons: symbol-range-crosses-piece

# CHECK:      riscv-function-sections-split: summary: candidate parents:
# CHECK-DAG:  riscv-function-sections-split: summary: safe parents:
# CHECK-DAG:  riscv-function-sections-split: summary: unsafe parents:
# CHECK-DAG:  riscv-function-sections-split: summary: overlapping-functions: 1
# CHECK-DAG:  riscv-function-sections-split: summary: zero-sized-only: 1
# CHECK-DAG:  riscv-function-sections-split: summary: no-function-ranges: 1
# CHECK-DAG:  riscv-function-sections-split: summary: no-reloc-cross-function-jal: 2
# CHECK-DAG:  riscv-function-sections-split: summary: no-reloc-cross-function-branch: 1
# CHECK-DAG:  riscv-function-sections-split: summary: function-fallthrough: 3
# CHECK-DAG:  riscv-function-sections-split: summary: computed-jump: 1
# CHECK-DAG:  riscv-function-sections-split: summary: align-crosses-function: 1
# CHECK-DAG:  riscv-function-sections-split: summary: pcrel-pair-crosses-function: 1
# CHECK-DAG:  riscv-function-sections-split: summary: source-section-symbol: 1
# CHECK-DAG:  riscv-function-sections-split: summary: source-addend-crosses-piece: 1
# CHECK-DAG:  riscv-function-sections-split: summary: incoming-section-symbol: 1
# CHECK-DAG:  riscv-function-sections-split: summary: incoming-addend-crosses-piece: 1
# CHECK-DAG:  riscv-function-sections-split: summary: incoming-eh-frame: 1
# CHECK-DAG:  riscv-function-sections-split: summary: incoming-debug-relocation: 1
# CHECK-DAG:  riscv-function-sections-split: summary: unexplained-gap: 1
# CHECK-DAG:  riscv-function-sections-split: summary: symbol-range-crosses-piece: 1

.globl _start
.section .text.safe,"ax",@progbits
_start:
.type safe0,@function
safe0:
  ret
.size safe0, .-safe0
.type safe1,@function
safe1:
  ret
.size safe1, .-safe1

.section .rodata.ptr,"a",@progbits
  .word safe0

.section .text.overlap,"ax",@progbits
.type overlap0,@function
overlap0:
  ret
.type overlap1,@function
overlap1:
  ret
.size overlap1, .-overlap1
.size overlap0, .-overlap0

.section .text.zero,"ax",@progbits
.type zero0,@function
zero0:
.size zero0, 0
  ret

.section .text.nofunc,"ax",@progbits
  ret

.section .text.jal,"ax",@progbits
.type jal0,@function
jal0:
  .reloc jal0, R_RISCV_RELAX
  .word 0x0040006f
.size jal0, .-jal0
.type jal1,@function
jal1:
  ret
.size jal1, .-jal1

.section .text.branch,"ax",@progbits
.type branch0,@function
branch0:
  .word 0x00000263
.size branch0, .-branch0
.type branch1,@function
branch1:
  ret
.size branch1, .-branch1

.section .text.rvc_zero,"ax",@progbits
.option push
.option rvc
.type rvc_zero0,@function
rvc_zero0:
1:
  ret
.size rvc_zero0, .-rvc_zero0
.type rvc_zero1,@function
rvc_zero1:
  c.j 1b
.size rvc_zero1, .-rvc_zero1
.option pop

.section .text.fallthrough,"ax",@progbits
.type fallthrough0,@function
fallthrough0:
  addi a0, a0, 0
.size fallthrough0, .-fallthrough0

.section .text.jalr,"ax",@progbits
.type jalr0,@function
jalr0:
  jalr x0, 0(a0)
.size jalr0, .-jalr0

.section .text.call,"ax",@progbits
.type call0,@function
call0:
  call call1
  ret
.size call0, .-call0
.type call1,@function
call1:
  ret
.size call1, .-call1

.section .text.call_norelax,"ax",@progbits
.option push
.option norelax
.type call_norelax0,@function
call_norelax0:
  call call_norelax1
  ret
.size call_norelax0, .-call_norelax0
.option pop
.type call_norelax1,@function
call_norelax1:
  ret
.size call_norelax1, .-call_norelax1

.section .text.align,"ax",@progbits
.type align0,@function
align0:
  ret
.size align0, .-align0
  .balign 8
.type align1,@function
align1:
  ret
.size align1, .-align1

.section .text.pcrel,"ax",@progbits
.type pcrel0,@function
pcrel0:
.Lpcrel_hi:
  auipc a0, %pcrel_hi(pcrel_sym)
  ret
.size pcrel0, .-pcrel0
.type pcrel1,@function
pcrel1:
  addi a0, a0, %pcrel_lo(.Lpcrel_hi)
  ret
.size pcrel1, .-pcrel1
pcrel_sym:
  .word 0

.section .text.source_sec,"ax",@progbits
.type source_sec0,@function
source_sec0:
  .reloc source_sec0, R_RISCV_32, .text.source_sec + 4
  ret
.size source_sec0, .-source_sec0
.type source_sec1,@function
source_sec1:
  ret
.size source_sec1, .-source_sec1

.section .text.source_addend,"ax",@progbits
.type source_addend0,@function
source_addend0:
  .reloc source_addend0, R_RISCV_32, source_addend0 + 4
  ret
.size source_addend0, .-source_addend0
.type source_addend1,@function
source_addend1:
  ret
.size source_addend1, .-source_addend1

.section .text.incoming_sec,"ax",@progbits
.type incoming_sec0,@function
incoming_sec0:
  ret
.size incoming_sec0, .-incoming_sec0
.type incoming_sec1,@function
incoming_sec1:
  ret
.size incoming_sec1, .-incoming_sec1
.section .rodata.incoming_sec,"a",@progbits
  .word .text.incoming_sec + 4

.section .text.incoming_addend,"ax",@progbits
.type incoming_addend0,@function
incoming_addend0:
  ret
.size incoming_addend0, .-incoming_addend0
.type incoming_addend1,@function
incoming_addend1:
  ret
.size incoming_addend1, .-incoming_addend1
.section .rodata.incoming_addend,"a",@progbits
  .word incoming_addend0 + 4

.section .text.eh,"ax",@progbits
.type eh0,@function
eh0:
  .cfi_startproc
  ret
  .cfi_endproc
.size eh0, .-eh0

.section .text.debug_target,"ax",@progbits
.type debug_target0,@function
debug_target0:
  ret
.size debug_target0, .-debug_target0
.section .debug_info,"",@progbits
  .word debug_target0

.section .text.gap,"ax",@progbits
.type gap0,@function
gap0:
  ret
.size gap0, .-gap0
  .word 0xdeadbeef
.type gap1,@function
gap1:
  ret
.size gap1, .-gap1

.section .text.symbol_cross,"ax",@progbits
.type symbol_cross_obj,@object
symbol_cross_obj:
.type symbol_cross0,@function
symbol_cross0:
  ret
.size symbol_cross0, .-symbol_cross0
.type symbol_cross1,@function
symbol_cross1:
  ret
.size symbol_cross1, .-symbol_cross1
.size symbol_cross_obj, .-symbol_cross_obj
