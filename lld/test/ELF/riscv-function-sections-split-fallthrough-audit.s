# REQUIRES: riscv

# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+c %s -o %t.o
# RUN: ld.lld --riscv-function-sections-split \
# RUN:   %t.o -o %t.no-print
# RUN: ld.lld --riscv-function-sections-split \
# RUN:   --print-riscv-function-sections-split %t.o -o %t.print 2> %t.log
# RUN: cmp %t.no-print %t.print
# RUN: FileCheck %s < %t.log

# CHECK: riscv-function-sections-split: parent section: .text.safe
# CHECK: riscv-function-sections-split: block reasons: none

# CHECK: riscv-function-sections-split: parent section: .text.fallthrough
# CHECK: riscv-function-sections-split: block reasons: computed-jump,function-fallthrough
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function arithmetic_end range [0,4) size 4 range start 0 range end 4 next function branch_end next start 4 adjacent-next yes last-function no range-end-parent-end no symbol boundary suspicious no symbol boundary detail none reason no-terminal
# CHECK-DAG: fallthrough audit: last instruction present yes offset 0 raw 0x{{[0-9a-f]+}} width 32 class non-terminal
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function branch_end range [4,8) size 4 {{.*}} reason conditional-branch-at-end
# CHECK-DAG: fallthrough audit: last instruction present yes offset 4 raw 0x{{[0-9a-f]+}} width 32 class conditional-branch {{.*}} direct target 4 target-current yes target-other no target-boundary yes
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function jal_call_end range [8,12) size 4 {{.*}} reason direct-call-at-end
# CHECK-DAG: fallthrough audit: last instruction present yes offset 8 raw 0x{{[0-9a-f]+}} width 32 class direct-call
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function call_pair_end range [12,20) size 8 {{.*}} reason direct-call-at-end
# CHECK-DAG: fallthrough audit: last instruction present yes offset 16 raw 0x{{[0-9a-f]+}} width 32 class direct-call rd 1 rs1 {{[0-9]+}} rs2 0 imm 0 {{.*}} part of call pair yes pair start offset 12 pair relocations R_RISCV_CALL{{(_PLT)?}}:ret32:0{{(,R_RISCV_RELAX:<none>:0)?}}
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function jalr_call_end range [20,24) size 4 {{.*}} reason indirect-call-at-end
# CHECK-DAG: fallthrough audit: last instruction present yes offset 20 raw 0x{{[0-9a-f]+}} width 32 class indirect-call
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function jalr_x5_end range [24,28) size 4 {{.*}} reason indirect-call-at-end
# CHECK-DAG: fallthrough audit: last instruction present yes offset 24 raw 0x{{[0-9a-f]+}} width 32 class indirect-call rd 5
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function ebreak32_end range [28,32) size 4 {{.*}} reason no-terminal
# CHECK-DAG: fallthrough audit: last instruction present yes offset 28 raw 0x100073 width 32 class ebreak
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function ecall32_end range [32,36) size 4 {{.*}} reason no-terminal
# CHECK-DAG: fallthrough audit: last instruction present yes offset 32 raw 0x73 width 32 class unknown-system
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function truncated_end range [36,38) size 2 {{.*}} reason truncated-instruction
# CHECK-DAG: fallthrough audit: last instruction present no
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function c_jalr_end range [38,40) size 2 {{.*}} reason indirect-call-at-end
# CHECK-DAG: fallthrough audit: last instruction present yes offset 38 raw 0x{{[0-9a-f]+}} width 16 class indirect-call
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function c_jr_tail_end range [40,42) size 2 {{.*}} reason indirect-jump-at-end
# CHECK-DAG: fallthrough audit: last instruction present yes offset 40 raw 0x{{[0-9a-f]+}} width 16 class indirect-jump
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function terminal_padding range [42,46) size 4 {{.*}} reason terminal-before-trailing-bytes
# CHECK-DAG: fallthrough audit: terminal exists yes terminal kind return trailing bytes after terminal 2 trailing zero no trailing nop yes trailing align no trailing known padding yes
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function c_ebreak_end range [46,48) size 2 {{.*}} reason compressed-ebreak
# CHECK-DAG: fallthrough audit: last instruction present yes offset 46 raw 0x{{[0-9a-f]+}} width 16 class compressed-ebreak
# CHECK-DAG: fallthrough audit: object file {{.*}} parent .text.fallthrough function c_reserved_end range [48,50) size 2 {{.*}} reason compressed-reserved
# CHECK-DAG: fallthrough audit: last instruction present yes offset 48 raw 0x8002 width 16 class compressed-reserved
# CHECK-DAG: fallthrough summary: fallthrough function range count: 14
# CHECK-DAG: fallthrough summary: affected parent count: 1
# CHECK-DAG: fallthrough summary: affected multi-function parent count: 1
# CHECK-DAG: fallthrough summary: symbol-boundary-suspicious function count: 0
# CHECK-DAG: fallthrough summary: no-terminal count: 3
# CHECK-DAG: fallthrough summary: terminal-before-trailing-bytes count: 1
# CHECK-DAG: fallthrough summary: conditional-branch-at-end count: 1
# CHECK-DAG: fallthrough summary: direct-call-at-end count: 2
# CHECK-DAG: fallthrough summary: indirect-call-at-end count: 3
# CHECK-DAG: fallthrough summary: indirect-jump-at-end count: 1
# CHECK-DAG: fallthrough summary: truncated-instruction count: 1
# CHECK-DAG: fallthrough summary: compressed-ebreak count: 1
# CHECK-DAG: fallthrough summary: compressed-reserved count: 1

.globl _start
.section .text.start,"ax",@progbits
.type _start,@function
_start:
  call ret32
  ret
.size _start, .-_start

.section .text.safe,"ax",@progbits
.type ret32,@function
ret32:
  ret
.size ret32, .-ret32

.option push
.option rvc
.type cret,@function
cret:
  c.jr ra
.size cret, .-cret
.option pop

.section .text.fallthrough,"ax",@progbits
.option push
.option norvc
.type arithmetic_end,@function
arithmetic_end:
  addi a0, a0, 1
.size arithmetic_end, .-arithmetic_end

.type branch_end,@function
branch_end:
  beqz a0, branch_end
.size branch_end, .-branch_end

.type jal_call_end,@function
jal_call_end:
  jal ra, ret32
.size jal_call_end, .-jal_call_end

.type call_pair_end,@function
call_pair_end:
  call ret32
.size call_pair_end, .-call_pair_end

.type jalr_call_end,@function
jalr_call_end:
  jalr ra, 0(a0)
.size jalr_call_end, .-jalr_call_end

.type jalr_x5_end,@function
jalr_x5_end:
  jalr t0, 0(a0)
.size jalr_x5_end, .-jalr_x5_end

.type ebreak32_end,@function
ebreak32_end:
  ebreak
.size ebreak32_end, .-ebreak32_end

.type ecall32_end,@function
ecall32_end:
  ecall
.size ecall32_end, .-ecall32_end

.type truncated_end,@function
truncated_end:
  .2byte 0x0013
.size truncated_end, .-truncated_end
.option pop

.option push
.option rvc
.type c_jalr_end,@function
c_jalr_end:
  c.jalr a0
.size c_jalr_end, .-c_jalr_end

.type c_jr_tail_end,@function
c_jr_tail_end:
  c.jr a0
.size c_jr_tail_end, .-c_jr_tail_end

.type terminal_padding,@function
terminal_padding:
  c.jr ra
  c.nop
.size terminal_padding, .-terminal_padding

.type c_ebreak_end,@function
c_ebreak_end:
  c.ebreak
.size c_ebreak_end, .-c_ebreak_end

.type c_reserved_end,@function
c_reserved_end:
  .2byte 0x8002
.size c_reserved_end, .-c_reserved_end
.option pop
