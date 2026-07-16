; RUN: opt < %s -S -passes=inline  | FileCheck %s
; RUN: opt < %s -S -passes=inline -enable-aggressive-inline  | FileCheck %s -check-prefix=AGG

define internal i32 @tester(i32 %in1, i32 %in2) noinline {
  %res = add i32 %in1, %in2
  ret i32 %res
}

; CHECK-LABEL: foo
; CHECK: add
; CHECK: call
; AGG-NOT: call
; CHECK: ret

define i32  @foo(i32 %in1, i32 %in2, i32 %in3)  {
  %tmp = add i32 %in2, %in3
  %res = call i32 @tester(i32 %in1, i32 %tmp)
  ret i32 %res
}
