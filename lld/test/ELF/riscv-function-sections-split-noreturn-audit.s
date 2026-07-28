# REQUIRES: riscv

# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+c %s -o %t.o
# RUN: ld.lld --riscv-function-sections-split %t.o -o %t.no-print
# RUN: ld.lld --riscv-function-sections-split \
# RUN:   --print-riscv-function-sections-split %t.o -o %t.print 2> %t.log
# RUN: cmp %t.no-print %t.print
# RUN: FileCheck %s < %t.log
# RUN: not grep "noreturn audit: .*caller indirect_caller" %t.log

# CHECK: riscv-function-sections-split: parent section: .text.callers
# CHECK: riscv-function-sections-split: block reasons: computed-jump,function-fallthrough
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_returns {{.*}} relocation target returns_target resolved target returns_target {{.*}} defined yes {{.*}} function yes {{.*}} target range found yes {{.*}} proof status target-contains-return
# CHECK-DAG: noreturn audit: target range [{{[0-9]+}},{{[0-9]+}}) target return count 1
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_chain {{.*}} relocation target chain_a resolved target chain_a {{.*}} proof status target-ends-in-direct-call
# CHECK-DAG: noreturn chain: depth 0 caller chain_a callee chain_b
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_known_exit {{.*}} relocation target exit resolved target exit {{.*}} known noreturn name candidate yes proof status target-contains-return proof reason target-contains-return
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_call_then_ret {{.*}} relocation target call_then_ret_target resolved target call_then_ret_target {{.*}} proof status target-contains-return
# CHECK-DAG: noreturn audit: target range [{{[0-9]+}},{{[0-9]+}}) target return count 1 {{.*}} target final class return target final part of call pair no target final direct-call target none
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_weak_undef {{.*}} relocation target weak_undef resolved target weak_undef {{.*}} undefined yes {{.*}} weak yes {{.*}} proof status undefined-target
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_cycle {{.*}} relocation target cycle_a resolved target cycle_a {{.*}} proof status target-ends-in-direct-call
# CHECK-DAG: noreturn chain stop: cycle
# CHECK-DAG: noreturn audit: object file {{.*}} parent .text.callers caller call_depth {{.*}} relocation target depth0 resolved target depth0 {{.*}} proof status target-ends-in-direct-call
# CHECK-DAG: noreturn chain stop: depth-limit
# CHECK-DAG: noreturn summary: noreturn direct-call audit count: {{[0-9]+}}
# CHECK-DAG: noreturn summary: affected parent count: {{[0-9]+}}
# CHECK-DAG: noreturn summary: defined target count: {{[0-9]+}}
# CHECK-DAG: noreturn summary: undefined target count: 1
# CHECK-DAG: noreturn summary: target range found count: {{[0-9]+}}
# CHECK-DAG: noreturn summary: known-name-only count: 0
# CHECK-DAG: noreturn summary: conservatively-proven count: 0

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
