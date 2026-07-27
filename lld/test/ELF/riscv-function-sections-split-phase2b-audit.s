# REQUIRES: riscv

# RUN: llvm-mc -filetype=obj -triple=riscv32-unknown-elf %s -o %t.o
# RUN: ld.lld --riscv-function-sections-split \
# RUN:   --print-riscv-function-sections-split %t.o -o %t 2>&1 \
# RUN:   | FileCheck %s --check-prefix=SUMMARY
# RUN: ld.lld --riscv-function-sections-split \
# RUN:   --print-riscv-function-sections-split \
# RUN:   --print-riscv-function-sections-split-relocs %t.o -o %t.relocs 2>&1 \
# RUN:   | FileCheck %s --check-prefix=RELOCS

# SUMMARY: riscv-function-sections-split: phase2b-audit: object: {{.*}} parent: .text.computed
# SUMMARY-SAME: blockers: computed-jump,function-fallthrough
# SUMMARY: riscv-function-sections-split: phase2b-audit: object: {{.*}} parent: .text.debug_target
# SUMMARY-SAME: incoming debug relocations: 1
# SUMMARY: riscv-function-sections-split: phase2b-audit: object: {{.*}} parent: .text.fallthrough
# SUMMARY-SAME: blockers: function-fallthrough
# SUMMARY: riscv-function-sections-split: phase2b-audit: object: {{.*}} parent: .text.gap_target
# SUMMARY-SAME: unmappable incoming relocations: 1
# SUMMARY: riscv-function-sections-split: phase2b-audit: object: {{.*}} parent: .text.secsym
# SUMMARY-SAME: incoming alloc relocations: 1
# SUMMARY-SAME: uniquely mappable incoming relocations: 1

# SUMMARY: riscv-function-sections-split: phase2b-audit: unsafe parent count:
# SUMMARY: riscv-function-sections-split: phase2b-audit: blocked function count:
# SUMMARY: riscv-function-sections-split: phase2b-audit: blocked candidate bytes:
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: blocker computed-jump: parents 1
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: blocker fallthrough: parents
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: blocker incoming-alloc-relocation: parents
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: blocker incoming-debug-relocation: parents 1
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: blocker incoming-section-symbol-relocation: parents 1
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: blocker incoming-symbol-addend: parents 1
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: incoming class alloc: relocations
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: incoming class debug: relocations 1 parents 1
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: incoming class nonalloc: relocations 1 parents 1
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: incoming class section-symbol-addend: relocations 1 parents 1
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: incoming class target-boundary: relocations
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: incoming class target-function: relocations
# SUMMARY-DAG: riscv-function-sections-split: phase2b-audit: incoming class target-gap: relocations 1 parents 1

# RELOCS: riscv-function-sections-split: phase2b-audit: incoming reloc: source object {{.*}} source section .debug_zgq_phase2b
# RELOCS-SAME: type R_RISCV_32 target debug0
# RELOCS: riscv-function-sections-split: phase2b-audit: incoming reloc: source object {{.*}} source section .rodata.gap_ref
# RELOCS-SAME: target gap_label
# RELOCS-SAME: unique 0
# RELOCS: riscv-function-sections-split: phase2b-audit: incoming reloc: source object {{.*}} source section .rodata.secsym_ref
# RELOCS-SAME: target .text.secsym
# RELOCS-SAME: target section 1

.globl _start
.section .text.start,"ax",@progbits
.type _start,@function
_start:
  ret
.size _start, .-_start

.section .text.safe,"ax",@progbits
.type safe0,@function
safe0:
  ret
.size safe0, .-safe0
.type safe1,@function
safe1:
  ret
.size safe1, .-safe1

.section .text.computed,"ax",@progbits
.type computed0,@function
computed0:
  jalr x0, 0(a0)
.size computed0, .-computed0
.type computed1,@function
computed1:
  ret
.size computed1, .-computed1

.section .text.fallthrough,"ax",@progbits
.type fallthrough0,@function
fallthrough0:
  addi a0, a0, 1
.size fallthrough0, .-fallthrough0
.type fallthrough1,@function
fallthrough1:
  ret
.size fallthrough1, .-fallthrough1

.section .text.debug_target,"ax",@progbits
.type debug0,@function
debug0:
  ret
.size debug0, .-debug0
.type debug1,@function
debug1:
  ret
.size debug1, .-debug1

.section .debug_zgq_phase2b,"",@progbits
  .word debug0

.section .text.secsym,"ax",@progbits
.type secsym0,@function
secsym0:
  ret
.size secsym0, .-secsym0
.type secsym1,@function
secsym1:
  ret
.size secsym1, .-secsym1

.section .rodata.secsym_ref,"a",@progbits
  .word .text.secsym + 4

.section .text.gap_target,"ax",@progbits
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

.section .rodata.gap_ref,"a",@progbits
  .word gap_label
