; Check if the ptradd is not sunk into the loop latch when the feature is enabled.
; RUN: opt < %s -loop-reduce -no-sink-ptradd-post-load -S | FileCheck -check-prefix=ENABLE %s
; RUN: opt < %s -loop-reduce -S | FileCheck -check-prefix=DISABLE %s

; ModuleID = 'foo.ll'
source_filename = "foo.cpp"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "aarch64-unknown-linux-gnu"

define dso_local noundef i16 @foo(ptr nocapture noundef readonly %ptr, i16 noundef %sum) local_unnamed_addr {
; ENABLE-LABEL: while.body:
; ENABLE-NEXT: [[PTR1:%.*]] = phi ptr [ [[GEP1:%.*]], %cleanup ], [ %{{.*}}, %while.body.preheader ]
; ENABLE: [[GEP1]] = getelementptr{{.*}}, ptr [[PTR1]], i64 8
; ENABLE-LABEL: cleanup:
; ENABLE-NOT: [[GEP1]] = {{.*}}
; DISABLE-LABEL: while.body:
; DISABLE-NEXT: [[PTR2:%.*]] = phi ptr [ [[GEP2:%.*]], %cleanup ], [ %{{.*}}, %while.body.preheader ]
; DISABLE-NOT: [[GEP2]] = getelementptr{{.*}}, ptr [[PTR2]], i64 8
; DISABLE-LABEL: cleanup:
; DISABLE: [[GEP2]] = getelementptr{{.*}}, ptr [[PTR2]], i64 8
;
entry:
  br label %while.body.preheader

while.body.preheader:                             ; preds = %entry
  br label %while.body
  
while.body:                                       ; preds = %while.body.preheader, %cleanup
  %ptr.addr.046 = phi ptr [ %incdec.ptr, %cleanup ], [ %ptr, %while.body.preheader ]
  %ret.045 = phi i16 [ %ret.1, %cleanup ], [ 0, %while.body.preheader ]
  %temp.044 = phi i16 [ %temp.1, %cleanup ], [ 0, %while.body.preheader ]
  %count.043 = phi i16 [ %inc, %cleanup ], [ 0, %while.body.preheader ]
  %0 = load i32, ptr %ptr.addr.046, align 4
  %y3 = getelementptr inbounds i8, ptr %ptr.addr.046, i64 4
  %1 = load i16, ptr %y3, align 4
  %z4 = getelementptr inbounds i8, ptr %ptr.addr.046, i64 6
  %2 = load i16, ptr %z4, align 2
  %incdec.ptr = getelementptr inbounds i8, ptr %ptr.addr.046, i64 8
  %inc = add nuw i16 %count.043, 1
  %cmp5 = icmp eq i32 %0, 0
  br i1 %cmp5, label %if.then, label %if.end19
  
if.then:                                          ; preds = %while.body
  %add = add i16 %temp.044, 3
  %add8 = add i16 %add, %1
  %add10 = add i16 %add8, %2
  br label %cleanup
  
if.end19:                                         ; preds = %while.body
  %add22 = add i16 %temp.044, -5
  %sub = add i16 %add22, %ret.045
  %add14 = add i16 %sub, %1
  %add24 = add i16 %add14, %2
  br label %cleanup
  
cleanup:                                          ; preds = %if.end19, %if.then
  %temp.1 = phi i16 [ %add10, %if.then ], [ 0, %if.end19 ]
  %ret.1 = phi i16 [ %ret.045, %if.then ], [ %add24, %if.end19 ]
  %cmp = icmp ult i16 %inc, %sum
  br i1 %cmp, label %while.body, label %while.end
  
while.end:                                        ; preds = %cleanup
  ret i16 %ret.1
}
