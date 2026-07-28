# REQUIRES: riscv

# RUN: rm -rf %t && split-file %s %t && cd %t
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+c,+relax branch.s -o branch.o
# RUN: llvm-readelf -r branch.o | FileCheck %s --check-prefix=RELOC
# RUN: ld.lld branch.o -o normal 2>&1 | count 0
# RUN: ld.lld --print-riscv-branch-rvc-audit branch.o -o audited 2>&1 | FileCheck %s --check-prefix=AUDIT
# RUN: cmp normal audited

# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+relax branch.s -o noc.o
# RUN: ld.lld --print-riscv-branch-rvc-audit noc.o -o noc 2>&1 | FileCheck %s --check-prefix=NOC

# RELOC: R_RISCV_BRANCH
# RELOC: R_RISCV_RELAX

# AUDIT: riscv branch-rvc audit: section={{.*}}:(.text) total=12 valid-opcode=12 beq-bne=10 zero=9 compact-reg=7 range=5 marker=1 no-marker=4 c-beqz=2 c-bnez=3 estimated-bytes=10
# NOC:   riscv branch-rvc audit: section={{.*}}:(.text) total=12 valid-opcode=12 beq-bne=10 zero=9 compact-reg=7 range=0 marker=0 no-marker=0 c-beqz=0 c-bnez=0 estimated-bytes=0

#--- branch.s
.globl _start
_start:
marker_beq:
  .word 0x00040063 # beq s0, zero, near_target
  .reloc marker_beq, R_RISCV_BRANCH, near_target
  .reloc marker_beq, R_RISCV_RELAX

beq_rs2:
  .word 0x00f00063 # beq zero, a5, near_target
  .reloc beq_rs2, R_RISCV_BRANCH, near_target

bne_rs1:
  .word 0x00049063 # bne s1, zero, near_target
  .reloc bne_rs1, R_RISCV_BRANCH, near_target

bne_rs2:
  .word 0x00e01063 # bne zero, a4, near_target
  .reloc bne_rs2, R_RISCV_BRANCH, near_target

bad_x7:
  .word 0x00038063 # beq t2, zero, near_target
  .reloc bad_x7, R_RISCV_BRANCH, near_target

bad_x16:
  .word 0x00080063 # beq a6, zero, near_target
  .reloc bad_x16, R_RISCV_BRANCH, near_target

bad_two_regs:
  .word 0x00940063 # beq s0, s1, near_target
  .reloc bad_two_regs, R_RISCV_BRANCH, near_target

bad_blt:
  .word 0x00044063 # blt s0, zero, near_target
  .reloc bad_blt, R_RISCV_BRANCH, near_target

bad_bge:
  .word 0x00045063 # bge s0, zero, near_target
  .reloc bad_bge, R_RISCV_BRANCH, near_target

near_target:
  nop

pos_oob:
  .word 0x00040063 # beq s0, zero, pos_oob_target
  .reloc pos_oob, R_RISCV_BRANCH, pos_oob_target
  .space 252
pos_oob_target:
  nop

neg_target:
  .space 256
neg_in_range:
  .word 0x00041063 # bne s0, zero, neg_target
  .reloc neg_in_range, R_RISCV_BRANCH, neg_target

neg_oob_target:
  .space 258
neg_oob:
  .word 0x00041063 # bne s0, zero, neg_oob_target
  .reloc neg_oob, R_RISCV_BRANCH, neg_oob_target
