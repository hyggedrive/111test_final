# REQUIRES: riscv

# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+c %s -o %t.o
# RUN: ld.lld --riscv-function-sections-split %t.o -o %t.no-print
# RUN: ld.lld --riscv-function-sections-split \
# RUN:   --print-riscv-function-sections-split %t.o -o %t.print 2> %t.log
# RUN: cmp %t.no-print %t.print
# RUN: llvm-readelf -r %t.o | FileCheck %s --check-prefix=RELOCS
# RUN: FileCheck %s < %t.log
# RUN: not grep "noreturn audit: .*caller indirect_caller" %t.log

# RELOCS: R_RISCV_RVC_JUMP
# RELOCS: R_RISCV_BRANCH
# RELOCS: R_RISCV_JAL

# CHECK: riscv-function-sections-split: parent section: .text.callers
# CHECK: riscv-function-sections-split: block reasons: computed-jump,function-fallthrough
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_returns {{.*}} relocation target returns_target resolved target returns_target {{.*}} defined yes {{.*}} function yes {{.*}} target range found yes {{.*}} proof status reachable-return
# CHECK-DAG: noreturn audit: target range [{{[0-9]+}},{{[0-9]+}}) target return count 1
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_chain {{.*}} relocation target chain_a resolved target chain_a {{.*}} proof status unproven-direct-call
# CHECK-DAG: noreturn chain: depth 0 caller chain_a callee chain_b
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_known_exit {{.*}} relocation target exit resolved target exit {{.*}} known noreturn name candidate yes proof status reachable-return proof reason reachable-return
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_call_then_ret {{.*}} relocation target call_then_ret_target resolved target call_then_ret_target {{.*}} proof status reachable-return
# CHECK-DAG: noreturn audit: target range [{{[0-9]+}},{{[0-9]+}}) target return count 1 {{.*}} target final class return target final part of call pair no target final direct-call target none target final relocation none
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_weak_undef {{.*}} relocation target weak_undef resolved target weak_undef {{.*}} undefined yes {{.*}} weak yes {{.*}} proof status undefined-target
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_cycle {{.*}} relocation target cycle_a resolved target cycle_a {{.*}} proof status unproven-direct-call
# CHECK-DAG: noreturn chain stop: cycle
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_depth {{.*}} relocation target depth0 resolved target depth0 {{.*}} proof status unproven-direct-call
# CHECK-DAG: noreturn chain stop: depth-limit
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_known_chain {{.*}} relocation target known_chain resolved target known_chain {{.*}} candidate noreturn chain yes proof status unproven-direct-call
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_self_loop32 {{.*}} relocation target self_loop32 resolved target self_loop32 {{.*}} proof status conservatively-proven proof reason reachable-internal-self-loop-no-return
# CHECK-DAG: noreturn cfg: target self_loop32 {{.*}} reachable return count 0 {{.*}} self loop yes {{.*}} proof status conservatively-proven proof reason reachable-internal-self-loop-no-return
# CHECK-DAG: noreturn cfg: instruction target self_loop32 {{.*}} class unconditional-jump {{.*}} target valid boundary yes successor count 1
# CHECK-DAG: noreturn proof: caller call_self_loop32 callee self_loop32 callee proven yes eligibility changed no
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_self_loop16 {{.*}} relocation target self_loop16 resolved target self_loop16 {{.*}} proof status conservatively-proven
# CHECK-DAG: noreturn cfg: target self_loop16 {{.*}} self loop yes {{.*}} proof status conservatively-proven
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_ecall_loop {{.*}} relocation target ecall_loop resolved target ecall_loop {{.*}} proof status conservatively-proven
# CHECK-DAG: noreturn cfg: target ecall_loop {{.*}} proof status conservatively-proven
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_c_addi_loop {{.*}} relocation target c_addi_loop resolved target c_addi_loop {{.*}} proof status conservatively-proven
# CHECK-DAG: noreturn cfg: target c_addi_loop {{.*}} proof status conservatively-proven
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_c_mv_loop {{.*}} relocation target c_mv_loop resolved target c_mv_loop {{.*}} proof status conservatively-proven
# CHECK-DAG: noreturn cfg: target c_mv_loop {{.*}} proof status conservatively-proven
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_c_swsp_loop {{.*}} relocation target c_swsp_loop resolved target c_swsp_loop {{.*}} proof status conservatively-proven
# CHECK-DAG: noreturn cfg: target c_swsp_loop {{.*}} proof status conservatively-proven
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_branch_ret {{.*}} relocation target branch_ret resolved target branch_ret {{.*}} proof status reachable-return
# CHECK-DAG: noreturn cfg: target branch_ret {{.*}} proof status reachable-return
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_indirect_target {{.*}} relocation target indirect_target resolved target indirect_target {{.*}} proof status reachable-indirect-control-flow
# CHECK-DAG: noreturn cfg: target indirect_target {{.*}} reachable indirect count 1 {{.*}} proof status reachable-indirect-control-flow
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_out_of_range_target {{.*}} relocation target out_of_range_target resolved target out_of_range_target {{.*}} proof status reachable-out-of-range-target
# CHECK-DAG: noreturn cfg: target out_of_range_target {{.*}} proof status reachable-out-of-range-target
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_truncated_target {{.*}} relocation target truncated_target resolved target truncated_target {{.*}} proof status invalid-instruction-boundary
# CHECK-DAG: noreturn cfg: target truncated_target {{.*}} proof status invalid-instruction-boundary
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_reloc_cj_forward {{.*}} relocation target reloc_cj_forward resolved target reloc_cj_forward {{.*}} proof status conservatively-proven
# CHECK-DAG: noreturn cfg: instruction target reloc_cj_forward {{.*}} class unconditional-jump {{.*}} target valid boundary yes target source relocation relocation type R_RISCV_RVC_JUMP
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_reloc_branch_ret {{.*}} relocation target reloc_branch_ret resolved target reloc_branch_ret {{.*}} proof status reachable-return
# CHECK-DAG: noreturn cfg: instruction target reloc_branch_ret {{.*}} class conditional-branch {{.*}} target source relocation relocation type R_RISCV_BRANCH
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_reloc_jal_out {{.*}} relocation target reloc_jal_out resolved target reloc_jal_out {{.*}} proof status reachable-out-of-range-target
# CHECK-DAG: noreturn cfg: instruction target reloc_jal_out {{.*}} class unconditional-jump {{.*}} target source relocation relocation type R_RISCV_JAL
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_illegal16_target {{.*}} relocation target illegal16_target resolved target illegal16_target {{.*}} proof status reachable-unknown-instruction
# CHECK-DAG: noreturn cfg: instruction target illegal16_target {{.*}} raw 0x0 width 16 class unknown
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_reserved16_target {{.*}} relocation target reserved16_target resolved target reserved16_target {{.*}} proof status reachable-unknown-instruction
# CHECK-DAG: noreturn cfg: instruction target reserved16_target {{.*}} raw 0x4 width 16 class unknown
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_bad_c_addi_target {{.*}} relocation target bad_c_addi_target resolved target bad_c_addi_target {{.*}} proof status reachable-unknown-instruction
# CHECK-DAG: noreturn cfg: instruction target bad_c_addi_target {{.*}} raw 0x5 width 16 class unknown
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_bad_c_mv_target {{.*}} relocation target bad_c_mv_target resolved target bad_c_mv_target {{.*}} proof status reachable-unknown-instruction
# CHECK-DAG: noreturn cfg: instruction target bad_c_mv_target {{.*}} raw 0x8006 width 16 class unknown
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_mret_target {{.*}} relocation target mret_target resolved target mret_target {{.*}} proof status reachable-indirect-control-flow
# CHECK-DAG: noreturn cfg: instruction target mret_target {{.*}} class system-return
# CHECK-DAG: noreturn summary: noreturn direct-call audit count: {{[0-9]+}}
# CHECK-DAG: noreturn summary: affected parent count: {{[0-9]+}}
# CHECK-DAG: noreturn summary: defined target count: {{[0-9]+}}
# CHECK-DAG: noreturn summary: undefined target count: 1
# CHECK-DAG: noreturn summary: target range found count: {{[0-9]+}}
# CHECK-DAG: noreturn summary: known-name-only count: 0
# CHECK-DAG: noreturn summary: conservatively-proven count: {{[1-9][0-9]*}}
# CHECK-DAG: noreturn summary: candidate-chain count: {{[0-9]+}}
# CHECK-DAG: noreturn summary: not-proven count: {{[0-9]+}}

.globl _start
.section .text.start,"ax",@progbits
.type _start,@function
_start:
  call returns_target
  ret
.size _start, .-_start

.weak weak_undef

.section .text.callers,"ax",@progbits
.option norvc
.type call_returns,@function
call_returns:
  call returns_target
.size call_returns, .-call_returns

.type call_chain,@function
call_chain:
  call chain_a
.size call_chain, .-call_chain

.type call_known_exit,@function
call_known_exit:
  call exit
.size call_known_exit, .-call_known_exit

.type call_call_then_ret,@function
call_call_then_ret:
  call call_then_ret_target
.size call_call_then_ret, .-call_call_then_ret

.type call_weak_undef,@function
call_weak_undef:
  call weak_undef
.size call_weak_undef, .-call_weak_undef

.type call_cycle,@function
call_cycle:
  call cycle_a
.size call_cycle, .-call_cycle

.type call_depth,@function
call_depth:
  call depth0
.size call_depth, .-call_depth

.type call_known_chain,@function
call_known_chain:
  call known_chain
.size call_known_chain, .-call_known_chain

.type call_self_loop32,@function
call_self_loop32:
  call self_loop32
.size call_self_loop32, .-call_self_loop32

.type call_self_loop16,@function
call_self_loop16:
  call self_loop16
.size call_self_loop16, .-call_self_loop16

.type call_ecall_loop,@function
call_ecall_loop:
  call ecall_loop
.size call_ecall_loop, .-call_ecall_loop

.type call_c_addi_loop,@function
call_c_addi_loop:
  call c_addi_loop
.size call_c_addi_loop, .-call_c_addi_loop

.type call_c_mv_loop,@function
call_c_mv_loop:
  call c_mv_loop
.size call_c_mv_loop, .-call_c_mv_loop

.type call_c_swsp_loop,@function
call_c_swsp_loop:
  call c_swsp_loop
.size call_c_swsp_loop, .-call_c_swsp_loop

.type call_branch_ret,@function
call_branch_ret:
  call branch_ret
.size call_branch_ret, .-call_branch_ret

.type call_indirect_target,@function
call_indirect_target:
  call indirect_target
.size call_indirect_target, .-call_indirect_target

.type call_out_of_range_target,@function
call_out_of_range_target:
  call out_of_range_target
.size call_out_of_range_target, .-call_out_of_range_target

.type call_truncated_target,@function
call_truncated_target:
  call truncated_target
.size call_truncated_target, .-call_truncated_target

.type call_reloc_cj_forward,@function
call_reloc_cj_forward:
  call reloc_cj_forward
.size call_reloc_cj_forward, .-call_reloc_cj_forward

.type call_reloc_branch_ret,@function
call_reloc_branch_ret:
  call reloc_branch_ret
.size call_reloc_branch_ret, .-call_reloc_branch_ret

.type call_reloc_jal_out,@function
call_reloc_jal_out:
  call reloc_jal_out
.size call_reloc_jal_out, .-call_reloc_jal_out

.type call_illegal16_target,@function
call_illegal16_target:
  call illegal16_target
.size call_illegal16_target, .-call_illegal16_target

.type call_reserved16_target,@function
call_reserved16_target:
  call reserved16_target
.size call_reserved16_target, .-call_reserved16_target

.type call_bad_c_addi_target,@function
call_bad_c_addi_target:
  call bad_c_addi_target
.size call_bad_c_addi_target, .-call_bad_c_addi_target

.type call_bad_c_mv_target,@function
call_bad_c_mv_target:
  call bad_c_mv_target
.size call_bad_c_mv_target, .-call_bad_c_mv_target

.type call_mret_target,@function
call_mret_target:
  call mret_target
.size call_mret_target, .-call_mret_target

.type indirect_caller,@function
indirect_caller:
  .reloc indirect_caller, R_RISCV_RELAX
  jalr ra, 0(a0)
.size indirect_caller, .-indirect_caller

.section .text.targets,"ax",@progbits
.option norvc
.type returns_target,@function
returns_target:
  ret
.size returns_target, .-returns_target

.type chain_a,@function
chain_a:
  call chain_b
.size chain_a, .-chain_a

.type chain_b,@function
chain_b:
  ret
.size chain_b, .-chain_b

.type exit,@function
exit:
  ret
.size exit, .-exit

.type call_then_ret_target,@function
call_then_ret_target:
  call chain_b
  ret
.size call_then_ret_target, .-call_then_ret_target

.type cycle_a,@function
cycle_a:
  call cycle_b
.size cycle_a, .-cycle_a

.type cycle_b,@function
cycle_b:
  call cycle_a
.size cycle_b, .-cycle_b

.type depth0,@function
depth0:
  call depth1
.size depth0, .-depth0

.type depth1,@function
depth1:
  call depth2
.size depth1, .-depth1

.type depth2,@function
depth2:
  call depth3
.size depth2, .-depth2

.type depth3,@function
depth3:
  call depth4
.size depth3, .-depth3

.type depth4,@function
depth4:
  call depth5
.size depth4, .-depth4

.type depth5,@function
depth5:
  ret
.size depth5, .-depth5

.type known_chain,@function
known_chain:
  call exit
.size known_chain, .-known_chain

.type self_loop32,@function
self_loop32:
  jal zero, self_loop32
.size self_loop32, .-self_loop32

.option push
.option rvc
.type self_loop16,@function
self_loop16:
  .2byte 0xa001
  .reloc self_loop16, R_RISCV_RVC_JUMP, self_loop16
.size self_loop16, .-self_loop16

.type ecall_loop,@function
ecall_loop:
  ecall
1:
  c.j 1b
.size ecall_loop, .-ecall_loop

.type c_addi_loop,@function
c_addi_loop:
  .2byte 0x1141
1:
  .2byte 0xa001
  .reloc 1b, R_RISCV_RVC_JUMP, 1b
.size c_addi_loop, .-c_addi_loop

.type c_mv_loop,@function
c_mv_loop:
  .2byte 0x85aa
1:
  .2byte 0xa001
  .reloc 1b, R_RISCV_RVC_JUMP, 1b
.size c_mv_loop, .-c_mv_loop

.type c_swsp_loop,@function
c_swsp_loop:
  .2byte 0xc606
1:
  .2byte 0xa001
  .reloc 1b, R_RISCV_RVC_JUMP, 1b
.size c_swsp_loop, .-c_swsp_loop
.option pop

.option norvc
.type branch_ret,@function
branch_ret:
  beqz a0, 1f
  jal zero, 2f
1:
  ret
2:
  jal zero, 2b
.size branch_ret, .-branch_ret

.type indirect_target,@function
indirect_target:
  jalr zero, 0(a0)
.size indirect_target, .-indirect_target

.type out_of_range_target,@function
out_of_range_target:
  jal zero, . + 8
.size out_of_range_target, .-out_of_range_target
  nop
  ret

.type truncated_target,@function
truncated_target:
  .2byte 0x0013
.size truncated_target, .-truncated_target

.option push
.option rvc
.type reloc_cj_forward,@function
reloc_cj_forward:
  .2byte 0xa001
  .reloc reloc_cj_forward, R_RISCV_RVC_JUMP, reloc_cj_forward_loop
  ret
reloc_cj_forward_loop:
  .2byte 0xa001
  .reloc reloc_cj_forward_loop, R_RISCV_RVC_JUMP, reloc_cj_forward_loop
.size reloc_cj_forward, .-reloc_cj_forward
.option pop

.option norvc
.type reloc_branch_ret,@function
reloc_branch_ret:
  .word 0x00050063
  .reloc reloc_branch_ret, R_RISCV_BRANCH, reloc_branch_ret_return
  jal zero, reloc_branch_ret_loop
reloc_branch_ret_return:
  ret
reloc_branch_ret_loop:
  jal zero, reloc_branch_ret_loop
.size reloc_branch_ret, .-reloc_branch_ret

.type reloc_jal_out,@function
reloc_jal_out:
  .word 0x0000006f
  .reloc reloc_jal_out, R_RISCV_JAL, reloc_jal_out_landing
.size reloc_jal_out, .-reloc_jal_out
reloc_jal_out_landing:
  ret

.type illegal16_target,@function
illegal16_target:
  .2byte 0x0000
.size illegal16_target, .-illegal16_target

.type reserved16_target,@function
reserved16_target:
  .2byte 0x0004
.size reserved16_target, .-reserved16_target

.type bad_c_addi_target,@function
bad_c_addi_target:
  .2byte 0x0005
.size bad_c_addi_target, .-bad_c_addi_target

.type bad_c_mv_target,@function
bad_c_mv_target:
  .2byte 0x8006
.size bad_c_mv_target, .-bad_c_mv_target

.type mret_target,@function
mret_target:
  .word 0x30200073
.size mret_target, .-mret_target
