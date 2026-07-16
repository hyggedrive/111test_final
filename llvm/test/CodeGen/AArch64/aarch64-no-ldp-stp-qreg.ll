; RUN: llc < %s -mtriple=arm64-eabi -verify-machineinstrs -mcpu=tsv110 | FileCheck %s
; RUN: llc < %s -mtriple=arm64-eabi -verify-machineinstrs -aarch64-ldp-stp-noq=false -mcpu=tsv110 | FileCheck %s --check-prefix=CHECK-NEGATIVE

define <2 x double> @ldp_doublex2(<2 x double>* %a) {
; CHECK-LABEL: ldp_doublex2
; CHECK: ldr    q[[DST1:[0-9]+]], [x0]
; CHECK: ldr    q[[DST2:[0-9]+]], [x0, #16]
; CHECK-NEGATIVE: ldp    q[[DST2:[0-9]+]], q[[DST1:[0-9]+]], [x0]
; CHECK-NEXT: fadd    v{{[0-9]+}}.2d, v[[DST1]].2d, v[[DST2]].2d
; CHECK-NEXT: ret
  %p1 = getelementptr inbounds <2 x double>, <2 x double>* %a, i64 0
  %tmp1 = load <2 x double>, <2 x double>* %p1, align 2
  %p2 = getelementptr inbounds <2 x double>, <2 x double>* %a, i64 1
  %tmp2 = load <2 x double>, <2 x double>* %p2, align 2
  %tmp3 = fadd <2 x double> %tmp1, %tmp2
  ret <2 x double> %tmp3 
}

; CHECK-LABEL: stp_doublex2
; CHECK: str q0, [x0]
; CHECK: str q1, [x0, #16]
; CHECK-NEGATIVE: stp q0, q1, [x0]
define void @stp_doublex2(<2 x double> %a, <2 x double> %b, <2 x double>* nocapture %p) nounwind {
  store <2 x double> %a, <2 x double>* %p, align 16
  %add.ptr = getelementptr inbounds <2 x double>, <2 x double>* %p, i64 1
  store <2 x double> %b, <2 x double>* %add.ptr, align 16
  ret void
}
