; RUN: llc < %s -mtriple=aarch64 -aarch64-endianness-opts | FileCheck %s --check-prefixes=CHECK

define i32 @test_read_3(ptr %b) {
; CHECK-LABEL: test_read_3:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    ldurh w8, [x0]
; CHECK-NEXT:    ldrb w9, [x0, #2]
; CHECK-NEXT:    rev16 w8, w8
; CHECK-NEXT:    orr w0, w9, w8, lsl #8
; CHECK-NEXT:    ret
entry:
  %0 = load i8, ptr %b, align 1
  %conv = zext i8 %0 to i32
  %shl = shl nuw nsw i32 %conv, 16
  %arrayidx1 = getelementptr inbounds i8, ptr %b, i64 1
  %1 = load i8, ptr %arrayidx1, align 1
  %conv2 = zext i8 %1 to i32
  %shl3 = shl nuw nsw i32 %conv2, 8
  %or = or i32 %shl3, %shl
  %arrayidx4 = getelementptr inbounds i8, ptr %b, i64 2
  %2 = load i8, ptr %arrayidx4, align 1
  %conv5 = zext i8 %2 to i32
  %or6 = or i32 %or, %conv5
  ret i32 %or6
}

;; Check for not crash.
define i64 @test_read_3_second(ptr %b) {
; CHECK-LABEL: test_read_3_second:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    ldrb    w8, [x0]
; CHECK-NEXT:    ldrb    w9, [x0, #1]
; CHECK-NEXT:    ldrb    w10, [x0, #2]
; CHECK-NEXT:    lsl     x8, x8, #16
; CHECK-NEXT:    orr     x8, x8, x9, lsl #8
; CHECK-NEXT:    orr     x0, x8, x10
; CHECK-NEXT:    ret
entry:
  %0 = load i8, ptr %b, align 1
  %conv = zext i8 %0 to i64
  %shl = shl nuw nsw i64 %conv, 16
  %arrayidx1 = getelementptr inbounds i8, ptr %b, i64 1
  %1 = load i8, ptr %arrayidx1, align 1
  %conv2 = zext i8 %1 to i64
  %shl3 = shl nuw nsw i64 %conv2, 8
  %or = or i64 %shl3, %shl
  %arrayidx4 = getelementptr inbounds i8, ptr %b, i64 2
  %2 = load i8, ptr %arrayidx4, align 1
  %conv5 = zext i8 %2 to i64
  %or6 = or i64 %or, %conv5
  ret i64 %or6
}

define i64 @test_read_6(ptr %b) {
; CHECK-LABEL: test_read_6:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    ldurh w8, [x0]
; CHECK-NEXT:    ldur w9, [x0, #2]
; CHECK-NEXT:    rev16 w8, w8
; CHECK-NEXT:    rev w9, w9
; CHECK-NEXT:    orr x0, x9, x8, lsl #32
; CHECK-NEXT:    ret
entry:
  %0 = load i8, ptr %b, align 1
  %conv.i = zext i8 %0 to i64
  %arrayidx1.i = getelementptr inbounds i8, ptr %b, i64 1
  %1 = load i8, ptr %arrayidx1.i, align 1
  %conv2.i = zext i8 %1 to i64
  %add.ptr = getelementptr inbounds i8, ptr %b, i64 2
  %2 = load i8, ptr %add.ptr, align 1
  %conv.i5 = zext i8 %2 to i64
  %shl.i6 = shl nuw nsw i64 %conv.i5, 24
  %arrayidx1.i7 = getelementptr inbounds i8, ptr %b, i64 3
  %3 = load i8, ptr %arrayidx1.i7, align 1
  %conv2.i8 = zext i8 %3 to i64
  %shl3.i = shl nuw nsw i64 %conv2.i8, 16
  %arrayidx4.i = getelementptr inbounds i8, ptr %b, i64 4
  %4 = load i8, ptr %arrayidx4.i, align 1
  %conv5.i = zext i8 %4 to i64
  %shl6.i = shl nuw nsw i64 %conv5.i, 8
  %arrayidx8.i = getelementptr inbounds i8, ptr %b, i64 5
  %5 = load i8, ptr %arrayidx8.i, align 1
  %conv9.i = zext i8 %5 to i64
  %6 = shl nuw nsw i64 %conv.i, 40
  %7 = shl nuw nsw i64 %conv2.i, 32
  %or.i9 = or i64 %7, %6
  %or7.i = or i64 %or.i9, %shl.i6
  %or10.i = or i64 %or7.i, %shl3.i
  %shl.i10 = or i64 %or10.i, %shl6.i
  %or.i11 = or i64 %shl.i10, %conv9.i
  ret i64 %or.i11
}

define i64 @test_read_7(ptr %b) {
; CHECK-LABEL: test_read_7:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    ldurh w8, [x0]
; CHECK-NEXT:    ldrb w9, [x0, #2]
; CHECK-NEXT:    ldur w10, [x0, #3]
; CHECK-NEXT:    rev16 w8, w8
; CHECK-NEXT:    orr w8, w9, w8, lsl #8
; CHECK-NEXT:    rev w9, w10
; CHECK-NEXT:    orr x0, x9, x8, lsl #32
; CHECK-NEXT:    ret
entry:
  %0 = load i8, ptr %b, align 1
  %conv.i = zext i8 %0 to i64
  %shl.i = shl nuw nsw i64 %conv.i, 16
  %arrayidx1.i = getelementptr inbounds i8, ptr %b, i64 1
  %1 = load i8, ptr %arrayidx1.i, align 1
  %conv2.i = zext i8 %1 to i64
  %shl3.i = shl nuw nsw i64 %conv2.i, 8
  %or.i = or i64 %shl3.i, %shl.i
  %arrayidx4.i = getelementptr inbounds i8, ptr %b, i64 2
  %2 = load i8, ptr %arrayidx4.i, align 1
  %conv5.i = zext i8 %2 to i64
  %or6.i = or i64 %or.i, %conv5.i
  %add.ptr = getelementptr inbounds i8, ptr %b, i64 3
  %3 = load i8, ptr %add.ptr, align 1
  %conv.i5 = zext i8 %3 to i64
  %shl.i6 = shl nuw nsw i64 %conv.i5, 24
  %arrayidx1.i7 = getelementptr inbounds i8, ptr %b, i64 4
  %4 = load i8, ptr %arrayidx1.i7, align 1
  %conv2.i8 = zext i8 %4 to i64
  %shl3.i9 = shl nuw nsw i64 %conv2.i8, 16
  %or.i10 = or i64 %shl3.i9, %shl.i6
  %arrayidx4.i11 = getelementptr inbounds i8, ptr %b, i64 5
  %5 = load i8, ptr %arrayidx4.i11, align 1
  %conv5.i12 = zext i8 %5 to i64
  %shl6.i = shl nuw nsw i64 %conv5.i12, 8
  %arrayidx8.i = getelementptr inbounds i8, ptr %b, i64 6
  %6 = load i8, ptr %arrayidx8.i, align 1
  %conv9.i = zext i8 %6 to i64
  %shl.i13 = shl nuw nsw i64 %or6.i, 32
  %or7.i = or i64 %or.i10, %shl.i13
  %or10.i = or i64 %or7.i, %shl6.i
  %or.i14 = or i64 %or10.i, %conv9.i
  ret i64 %or.i14
}

define void @test_write_3(ptr %b, i64 %n) {
; CHECK-LABEL: test_write_3:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    rev16 w8, w1
; CHECK-NEXT:    lsr x9, x1, #16
; CHECK-NEXT:    sturh w8, [x0, #1]
; CHECK-NEXT:    strb w9, [x0]
; CHECK-NEXT:    ret
entry:
  %shr = lshr i64 %n, 16
  %conv = trunc i64 %shr to i8
  store i8 %conv, ptr %b, align 1
  %shr1 = lshr i64 %n, 8
  %conv2 = trunc i64 %shr1 to i8
  %arrayidx3 = getelementptr inbounds i8, ptr %b, i64 1
  store i8 %conv2, ptr %arrayidx3, align 1
  %conv4 = trunc i64 %n to i8
  %arrayidx5 = getelementptr inbounds i8, ptr %b, i64 2
  store i8 %conv4, ptr %arrayidx5, align 1
  ret void
}

define void @test_write_6(ptr %b, i64 %n) {
; CHECK-LABEL: test_write_6:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    lsr x9, x1, #32
; CHECK-NEXT:    rev w8, w1
; CHECK-NEXT:    rev16 w9, w9
; CHECK-NEXT:    stur w8, [x0, #2]
; CHECK-NEXT:    sturh w9, [x0]
; CHECK-NEXT:    ret
entry:
  %shr = lshr i64 %n, 32
  %shr.i = lshr i64 %n, 40
  %conv.i = trunc i64 %shr.i to i8
  store i8 %conv.i, ptr %b, align 1
  %conv1.i = trunc i64 %shr to i8
  %arrayidx2.i = getelementptr inbounds i8, ptr %b, i64 1
  store i8 %conv1.i, ptr %arrayidx2.i, align 1
  %add.ptr = getelementptr inbounds i8, ptr %b, i64 2
  %shr.i3 = lshr i64 %n, 24
  %conv.i4 = trunc i64 %shr.i3 to i8
  store i8 %conv.i4, ptr %add.ptr, align 1
  %shr1.i = lshr i64 %n, 16
  %conv2.i = trunc i64 %shr1.i to i8
  %arrayidx3.i = getelementptr inbounds i8, ptr %b, i64 3
  store i8 %conv2.i, ptr %arrayidx3.i, align 1
  %shr4.i = lshr i64 %n, 8
  %conv5.i = trunc i64 %shr4.i to i8
  %arrayidx6.i = getelementptr inbounds i8, ptr %b, i64 4
  store i8 %conv5.i, ptr %arrayidx6.i, align 1
  %conv7.i = trunc i64 %n to i8
  %arrayidx8.i = getelementptr inbounds i8, ptr %b, i64 5
  store i8 %conv7.i, ptr %arrayidx8.i, align 1
  ret void
}

define void @test_write_7(ptr %b, i64 %n) {
; CHECK-LABEL: test_write_7:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    lsr x9, x1, #32
; CHECK-NEXT:    rev w8, w1
; CHECK-NEXT:    rev16 w9, w9
; CHECK-NEXT:    lsr w10, w9, #24
; CHECK-NEXT:    stur w8, [x0, #3]
; CHECK-NEXT:    sturh w9, [x0, #1]
; CHECK-NEXT:    strb w10, [x0]
; CHECK-NEXT:    ret
entry:
  %shr = lshr i64 %n, 32
  %shr.i = lshr i64 %n, 48
  %conv.i = trunc i64 %shr.i to i8
  store i8 %conv.i, ptr %b, align 1
  %shr1.i = lshr i64 %n, 40
  %conv2.i = trunc i64 %shr1.i to i8
  %arrayidx3.i = getelementptr inbounds i8, ptr %b, i64 1
  store i8 %conv2.i, ptr %arrayidx3.i, align 1
  %conv4.i = trunc i64 %shr to i8
  %arrayidx5.i = getelementptr inbounds i8, ptr %b, i64 2
  store i8 %conv4.i, ptr %arrayidx5.i, align 1
  %add.ptr = getelementptr inbounds i8, ptr %b, i64 3
  %shr.i3 = lshr i64 %n, 24
  %conv.i4 = trunc i64 %shr.i3 to i8
  store i8 %conv.i4, ptr %add.ptr, align 1
  %shr1.i5 = lshr i64 %n, 16
  %conv2.i6 = trunc i64 %shr1.i5 to i8
  %arrayidx3.i7 = getelementptr inbounds i8, ptr %b, i64 4
  store i8 %conv2.i6, ptr %arrayidx3.i7, align 1
  %shr4.i = lshr i64 %n, 8
  %conv5.i = trunc i64 %shr4.i to i8
  %arrayidx6.i = getelementptr inbounds i8, ptr %b, i64 5
  store i8 %conv5.i, ptr %arrayidx6.i, align 1
  %conv7.i = trunc i64 %n to i8
  %arrayidx8.i = getelementptr inbounds i8, ptr %b, i64 6
  store i8 %conv7.i, ptr %arrayidx8.i, align 1
  ret void
}

define void @test_write_with_different_chains(ptr noalias %b, i64 %n, ptr noalias %c) {
; CHECK-LABEL: test_write_with_different_chains:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    ldr x8, [x2]
; CHECK-NEXT:    lsr x9, x1, #16
; CHECK-NEXT:    rev16 w10, w1
; CHECK-NEXT:    rev16 w11, w8
; CHECK-NEXT:    lsr x8, x8, #16
; CHECK-NEXT:    strb w9, [x0]
; CHECK-NEXT:    sturh w10, [x0, #1]
; CHECK-NEXT:    sturh w11, [x2, #1]
; CHECK-NEXT:    strb w8, [x2]
; CHECK-NEXT:    ret
entry:
  %shr = lshr i64 %n, 16
  %conv = trunc i64 %shr to i8
  store i8 %conv, ptr %b, align 1
  %shr1 = lshr i64 %n, 8
  %conv2 = trunc i64 %shr1 to i8
  %arrayidx3 = getelementptr inbounds i8, ptr %b, i64 1
  store i8 %conv2, ptr %arrayidx3, align 1
  %conv4 = trunc i64 %n to i8
  %arrayidx5 = getelementptr inbounds i8, ptr %b, i64 2
  store i8 %conv4, ptr %arrayidx5, align 1
  %0 = load i64, ptr %c, align 1
  %shr10 = lshr i64 %0, 16
  %conv10 = trunc i64 %shr10 to i8
  store i8 %conv10, ptr %c, align 1
  %shr11 = lshr i64 %0, 8
  %conv12 = trunc i64 %shr11 to i8
  %arrayidx13 = getelementptr inbounds i8, ptr %c, i64 1
  store i8 %conv12, ptr %arrayidx13, align 1
  %conv14 = trunc i64 %0 to i8
  %arrayidx15 = getelementptr inbounds i8, ptr %c, i64 2
  store i8 %conv14, ptr %arrayidx15, align 1
  ret void
}

define void @test_write_multiple_to_same_address(ptr %b, i64 %n, i64 %m) {
; CHECK-LABEL: test_write_multiple_to_same_address:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    rev16 w8, w2
; CHECK-NEXT:    rev16 w9, w1
; CHECK-NEXT:    lsr x10, x1, #16
; CHECK-NEXT:    lsr x11, x2, #16
; CHECK-NEXT:    sturh w8, [x0, #8]
; CHECK-NEXT:    sturh w9, [x0, #1]
; CHECK-NEXT:    strb w10, [x0]
; CHECK-NEXT:    strb w11, [x0, #7]
; CHECK-NEXT:    ret
entry:
  %shr = lshr i64 %n, 16
  %conv = trunc i64 %shr to i8
  store i8 %conv, ptr %b, align 1
  %shr1 = lshr i64 %n, 8
  %conv2 = trunc i64 %shr1 to i8
  %arrayidx3 = getelementptr inbounds i8, ptr %b, i64 1
  store i8 %conv2, ptr %arrayidx3, align 1
  %conv4 = trunc i64 %n to i8
  %arrayidx5 = getelementptr inbounds i8, ptr %b, i64 2
  store i8 %conv4, ptr %arrayidx5, align 1
  %shr10 = lshr i64 %m, 16
  %conv10 = trunc i64 %shr10 to i8
  %arrayidx11 = getelementptr inbounds i8, ptr %b, i64 7
  store i8 %conv10, ptr %arrayidx11, align 1
  %shr11 = lshr i64 %m, 8
  %conv12 = trunc i64 %shr11 to i8
  %arrayidx13 = getelementptr inbounds i8, ptr %b, i64 8
  store i8 %conv12, ptr %arrayidx13, align 1
  %conv14 = trunc i64 %m to i8
  %arrayidx15 = getelementptr inbounds i8, ptr %b, i64 9
  store i8 %conv14, ptr %arrayidx15, align 1
  ret void
}