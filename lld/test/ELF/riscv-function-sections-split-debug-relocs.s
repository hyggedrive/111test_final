# REQUIRES: riscv

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=riscv32-unknown-elf %t/main.s -o %t/main.o
# RUN: ld.lld --gc-sections --riscv-function-sections-split-gc \
# RUN:   --print-riscv-function-sections-split %t/main.o -o %t/off 2>&1 \
# RUN:   | FileCheck %s --check-prefix=OFF
# RUN: ld.lld --gc-sections --riscv-function-sections-split-debug-relocs \
# RUN:   --print-riscv-function-sections-split %t/main.o -o %t/on 2>&1 \
# RUN:   | FileCheck %s --check-prefix=ON
# RUN: llvm-nm %t/on | FileCheck %s --check-prefix=NM \
# RUN:   --implicit-check-not=accept_dead --implicit-check-not=end_live

# OFF: riscv-function-sections-split: parent section: .text.accept
# OFF: riscv-function-sections-split: block reasons: incoming-debug-relocation

# ON: riscv-function-sections-split: parent section: .text.accept
# ON: riscv-function-sections-split: status: safe
# ON: riscv-function-sections-split: parent section: .text.end
# ON: riscv-function-sections-split: status: safe
# ON: riscv-function-sections-split: parent section: .text.addend_ok
# ON: riscv-function-sections-split: status: safe
# ON: riscv-function-sections-split: parent section: .text.addend_cross
# ON: riscv-function-sections-split: block reasons: incoming-debug-relocation
# ON: riscv-function-sections-split: parent section: .text.gap
# ON: riscv-function-sections-split: block reasons: incoming-debug-relocation
# ON: riscv-function-sections-split: parent section: .text.secsym
# ON: riscv-function-sections-split: block reasons: incoming-debug-relocation
# ON: riscv-function-sections-split: parent section: .text.alloc
# ON: riscv-function-sections-split: block reasons: incoming-addend-crosses-piece
# ON: riscv-function-sections-split: parent section: .text.mixed
# ON: riscv-function-sections-split: block reasons: incoming-debug-relocation

# ON: riscv-function-sections-split: phase1a: split parent: {{.*}}:(.text.accept)
# ON: riscv-function-sections-split: phase1a: split parent: {{.*}}:(.text.end)
# ON: riscv-function-sections-split: phase1a: split parent: {{.*}}:(.text.addend_ok)
# ON: riscv-function-sections-split: phase2b-debug: accepted debug relocation count: 3
# ON: riscv-function-sections-split: phase2b-debug: accepted interior relocation count: 2
# ON: riscv-function-sections-split: phase2b-debug: accepted parent-end relocation count: 1
# ON: riscv-function-sections-split: phase2b-debug: newly safe parent count: 3
# ON: riscv-function-sections-split: phase2b-debug: newly split parent count: 3
# ON-DAG: riscv-function-sections-split: phase2b-debug: fallback: addend-crosses-child: parents 1
# ON-DAG: riscv-function-sections-split: phase2b-debug: fallback: alloc-source: parents 1
# ON-DAG: riscv-function-sections-split: phase2b-debug: fallback: section-symbol: parents 1
# ON-DAG: riscv-function-sections-split: phase2b-debug: fallback: target-gap: parents 2

# NM-DAG: T accept_live
# NM-DAG: T end_dead

#--- main.s
.globl _start
.section .text.start,"ax",@progbits
.type _start,@function
_start:
  call accept_live
  call end_dead
  call addend_ok0
  ret
.size _start, .-_start

.section .text.accept,"ax",@progbits
.globl accept_live
.type accept_live,@function
accept_live:
accept_label:
  ret
.size accept_live, .-accept_live
.globl accept_dead
.type accept_dead,@function
accept_dead:
  ret
.size accept_dead, .-accept_dead

.section .debug_zgq_accept,"",@progbits
  .word accept_label

.section .text.end,"ax",@progbits
.globl end_live
.type end_live,@function
end_live:
  ret
.size end_live, .-end_live
.globl end_dead
.type end_dead,@function
end_dead:
  ret
.size end_dead, .-end_dead
end_parent_end:

.section .debug_zgq_end,"",@progbits
  .word end_parent_end

.section .text.addend_ok,"ax",@progbits
.type addend_ok0,@function
addend_ok0:
addend_ok_label:
  ret
  nop
.size addend_ok0, .-addend_ok0
.type addend_ok1,@function
addend_ok1:
  ret
.size addend_ok1, .-addend_ok1

.section .debug_zgq_addend_ok,"",@progbits
  .word addend_ok_label + 2

.section .text.addend_cross,"ax",@progbits
.type addend_cross0,@function
addend_cross0:
addend_cross_label:
  ret
.size addend_cross0, .-addend_cross0
.type addend_cross1,@function
addend_cross1:
  ret
.size addend_cross1, .-addend_cross1

.section .debug_zgq_addend_cross,"",@progbits
  .word addend_cross_label + 4

.section .text.gap,"ax",@progbits
.type gap0,@function
gap0:
  ret
.size gap0, .-gap0
gap_label:
  .word 0
.type gap1,@function
gap1:
  ret
.size gap1, .-gap1

.section .debug_zgq_gap,"",@progbits
  .word gap_label

.section .text.secsym,"ax",@progbits
.type secsym0,@function
secsym0:
  ret
.size secsym0, .-secsym0
.type secsym1,@function
secsym1:
  ret
.size secsym1, .-secsym1

.section .debug_zgq_secsym,"",@progbits
  .word .text.secsym + 4

.section .text.alloc,"ax",@progbits
.type alloc0,@function
alloc0:
  ret
.size alloc0, .-alloc0
.type alloc1,@function
alloc1:
  ret
.size alloc1, .-alloc1

.section .rodata.alloc_ref,"a",@progbits
  .word alloc0 + 4

.section .text.mixed,"ax",@progbits
.type mixed0,@function
mixed0:
mixed_label:
  ret
.size mixed0, .-mixed0
mixed_gap:
  .word 0
.type mixed1,@function
mixed1:
  ret
.size mixed1, .-mixed1

.section .debug_zgq_mixed,"",@progbits
  .word mixed_label
  .word mixed_gap
