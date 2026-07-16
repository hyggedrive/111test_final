; RUN: opt < %s  -S --passes='loop-versioning-licm' --mcpu=tsv110 -mtriple aarch64-linux-gnu -loop-versioning-overlap -debug-only=loop-versioning-licm 2>&1 | FileCheck %s
; REQUIRES: asserts, aarch64-registered-target
;
; CHECK:    Do Loop Versioning Overlap transformation
;
; CHECK-LABEL: do.body.ph:
; CHECK-NEXT:    [[TMP1:%.*]] = load i64, ptr [[X:%.*]]
;
; CHECK-LABEL: do.body:
; CHECK-NEXT:    phi
; CHECK-NEXT:    phi
; CHECK-NEXT:    load
; CHECK-NEXT:    store i64 [[TMP1]], ptr [[Y:%.*]]

define void @test0(ptr %dstPtr, ptr %srcPtr, ptr %dstEnd) {
entry:
  br label %do.body

do.body:
  %s.0 = phi ptr [ %srcPtr, %entry ], [ %add.ptr1, %do.body ]
  %d.0 = phi ptr [ %dstPtr, %entry ], [ %add.ptr, %do.body ]
  %0 = load i64, ptr %s.0, align 1
  store i64 %0, ptr %d.0, align 1
  %add.ptr = getelementptr inbounds i8, ptr %d.0, i64 8
  %add.ptr1 = getelementptr inbounds i8, ptr %s.0, i64 8
  %cmp = icmp ult ptr %add.ptr, %dstEnd
  br i1 %cmp, label %do.body, label %do.end

do.end:
  ret void
}
