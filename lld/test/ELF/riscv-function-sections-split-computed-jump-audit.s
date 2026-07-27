# REQUIRES: riscv

# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+c %s -o %t.o
# RUN: ld.lld --riscv-function-sections-split \
# RUN:   --print-riscv-function-sections-split %t.o -o %t 2>&1 \
# RUN:   | FileCheck %s

# CHECK: riscv-function-sections-split: parent section: .text.jalr
# CHECK: riscv-function-sections-split: block reasons: computed-jump
# CHECK-DAG: riscv-function-sections-split: computed-jump audit: object file {{.*}} parent .text.jalr function ret_func offset 0 raw 0x8067 width 32 rd 0 rs1 1 imm 0 has relocation no relocations none class return
# CHECK-DAG: riscv-function-sections-split: computed-jump audit: object file {{.*}} parent .text.jalr function indirect_call offset 4 raw 0x500e7 width 32 rd 1 rs1 10 imm 0 has relocation no relocations none class indirect-call
# CHECK-DAG: riscv-function-sections-split: computed-jump audit: object file {{.*}} parent .text.jalr function indirect_tail offset 12 raw 0x50067 width 32 rd 0 rs1 10 imm 0 has relocation no relocations none class indirect-tail-jump
# CHECK-DAG: riscv-function-sections-split: computed-jump audit: object file {{.*}} parent .text.jalr function unusual_jalr offset 16 raw 0x502e7 width 32 rd 5 rs1 10 imm 0 has relocation no relocations none class unusual-jalr
# CHECK-DAG: riscv-function-sections-split: computed-jump audit: object file {{.*}} parent .text.jalr function reloc_jalr offset 24 raw 0x500e7 width 32 rd 1 rs1 10 imm 0 has relocation yes relocations R_RISCV_NONE:reloc_target,R_RISCV_RELAX:<none> class indirect-call
# CHECK-DAG: riscv-function-sections-split: computed-jump audit: object file {{.*}} parent .text.rvc function rvc_ret offset 0 raw 0x{{[0-9a-f]+}} width 16 rd 0 rs1 1 imm 0 has relocation no relocations none class return
# CHECK-DAG: riscv-function-sections-split: computed-jump audit: object file {{.*}} parent .text.rvc function rvc_call offset 2 raw 0x{{[0-9a-f]+}} width 16 rd 1 rs1 10 imm 0 has relocation no relocations none class indirect-call
# CHECK-DAG: riscv-function-sections-split: computed-jump audit: object file {{.*}} parent .text.rvc function rvc_tail offset 4 raw 0x{{[0-9a-f]+}} width 16 rd 0 rs1 10 imm 0 has relocation no relocations none class indirect-tail-jump
# CHECK-DAG: riscv-function-sections-split: computed-jump audit: object file {{.*}} parent .text.rvc function rvc_ebreak offset 6 raw 0x{{[0-9a-f]+}} width 16 class compressed-ebreak
# CHECK-DAG: riscv-function-sections-split: computed-jump audit: object file {{.*}} parent .text.rvc function rvc_reserved offset 8 raw 0x8002 width 16 class compressed-reserved
# CHECK-DAG: riscv-function-sections-split: computed-jump summary: blocking jalr instructions: 6
# CHECK-DAG: riscv-function-sections-split: computed-jump summary: compressed non-jalr control instructions: 2
# CHECK-DAG: riscv-function-sections-split: computed-jump summary: computed-jump blocker instructions: 8
# CHECK-DAG: riscv-function-sections-split: computed-jump summary: indirect-call count: 3
# CHECK-DAG: riscv-function-sections-split: computed-jump summary: indirect-tail-jump count: 2
# CHECK-DAG: riscv-function-sections-split: computed-jump summary: unusual-jalr count: 1
# CHECK-DAG: riscv-function-sections-split: computed-jump summary: compressed-ebreak count: 1
# CHECK-DAG: riscv-function-sections-split: computed-jump summary: compressed-reserved count: 1
# CHECK-DAG: riscv-function-sections-split: computed-jump summary: affected parent count: 2
# CHECK-DAG: riscv-function-sections-split: computed-jump summary: affected multi-function parent count: 2
# CHECK-DAG: riscv-function-sections-split: computed-jump summary: affected-parent candidate bytes indirect-call:

.option norvc

.globl _start
.section .text.start,"ax",@progbits
.type _start,@function
_start:
  call ret_func
  ret
.size _start, .-_start

.section .text.jalr,"ax",@progbits
.type ret_func,@function
ret_func:
  ret
.size ret_func, .-ret_func

.type indirect_call,@function
indirect_call:
  jalr ra, 0(a0)
  ret
.size indirect_call, .-indirect_call

.type indirect_tail,@function
indirect_tail:
  jalr zero, 0(a0)
.size indirect_tail, .-indirect_tail

.type unusual_jalr,@function
unusual_jalr:
  jalr t0, 0(a0)
  ret
.size unusual_jalr, .-unusual_jalr

.type reloc_jalr,@function
reloc_jalr:
reloc_site:
  .reloc reloc_site, R_RISCV_NONE, reloc_target
  .reloc reloc_site, R_RISCV_RELAX
  jalr ra, 0(a0)
  ret
.size reloc_jalr, .-reloc_jalr

.option push
.option rvc
.section .text.rvc,"ax",@progbits
.type rvc_ret,@function
rvc_ret:
  c.jr ra
.size rvc_ret, .-rvc_ret

.type rvc_call,@function
rvc_call:
  c.jalr a0
.size rvc_call, .-rvc_call

.type rvc_tail,@function
rvc_tail:
  c.jr a0
.size rvc_tail, .-rvc_tail

.type rvc_ebreak,@function
rvc_ebreak:
  c.ebreak
.size rvc_ebreak, .-rvc_ebreak

.type rvc_reserved,@function
rvc_reserved:
  .2byte 0x8002
.size rvc_reserved, .-rvc_reserved
.option pop

.section .text.target,"ax",@progbits
.type reloc_target,@function
reloc_target:
  ret
.size reloc_target, .-reloc_target
