; RUN: opt < %s -passes='mem2reg,simplifycfg,loop-simplify,lcssa,loop-mssa(licm)' -licm-skip-no-memory-inline-asm=true -S | FileCheck %s

@p = dso_local global ptr null, align 8
define dso_local void @foo() {
entry:
  %i = alloca i32, align 4
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, ptr %i, align 4
  %1 = load ptr, ptr @p, align 8
  %arrayidx = getelementptr inbounds i32, ptr %1, i64 0
  %2 = load i32, ptr %arrayidx, align 4
  %cmp = icmp slt i32 %0, %2
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  call void asm sideeffect "nop\0A\09", "~{dirflag},~{fpsr},~{flags}"()
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %3 = load i32, ptr %i, align 4
  %inc = add nsw i32 %3, 1
  store i32 %inc, ptr %i, align 4
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}
; CHECK-LABEL: @foo(
; CHECK: entry:
; CHECK-NEXT:  [[P:%.*]] = load ptr, ptr @p
; CHECK-NEXT:  [[GEP:%.*]] = getelementptr inbounds i32, ptr [[P]], i64 0
; CHECK-NEXT:  [[BOUND:%.*]] = load i32, ptr [[GEP]]
; CHECK-NEXT:  br label [[COND:%.*]]
