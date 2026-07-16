// The content of this file is sw64-only:
#if defined(__sw_64__)

#include "sanitizer_common/sanitizer_asm.h"

.section .text
.set noreorder

ASM_HIDDEN(__tsan_setjmp)
.comm _ZN14__interception11real_setjmpE,8,8
.globl ASM_SYMBOL_INTERCEPTOR(setjmp)
ASM_TYPE_FUNCTION(ASM_SYMBOL_INTERCEPTOR(setjmp))
ASM_SYMBOL_INTERCEPTOR(setjmp):
  ldih $r29, 0($r27)
  ldi $r29, 0($r29)
  CFI_STARTPROC

  // Save frame/link register
  ldi $sp, -32($sp)
  stl $r26, 0($sp)
  stl $fp, 8($sp)
  CFI_DEF_CFA_OFFSET (32)
  CFI_OFFSET (26, -32)
  CFI_OFFSET (15, -24)

  // Adjust the SP for previous frame
  ldi $fp,0($sp)
  CFI_DEF_CFA_REGISTER (15)

  // Save env parameter
  stl $r16, 16($sp)
  CFI_OFFSET (0, -16)

  // Obtain SP, first argument to `void __tsan_setjmp(uptr sp)`
  ldi $r16, 32($sp)

  // call tsan interceptor
  //ldih $r27, ASM_SYMBOL(__tsan_setjmp)($r29) !gprelhigh
  //ldi $r27, ASM_SYMBOL(__tsan_setjmp)($r29) !gprellow
  ldl $r27, ASM_SYMBOL(__tsan_setjmp)($r29) !literal
  call $r26, ($r27), 0
  ldih $r29, 0($r26)
  ldi $r29, 0($r29)

  // Restore env parameter
  ldl $r16, 16($sp)
  CFI_RESTORE (0)

  // Restore frame/link register
  ldl $fp, 8($sp)
  ldl $r26, 0($sp)
  CFI_RESTORE (15)
  CFI_RESTORE (26)
  CFI_DEF_CFA (31, 0)
  ldi $sp, 32($sp)

  // tail jump to libc setjmp
  ldl $r27, _ZN14__interception11real_setjmpE($r29) !literal
  ldl $r27, 0($r27)
  jmp $r31, ($r27), 0

  CFI_ENDPROC
ASM_SIZE(ASM_SYMBOL_INTERCEPTOR(setjmp))

ASM_HIDDEN(__tsan_setjmp)
.comm _ZN14__interception12real__setjmpE,8,8
.globl ASM_SYMBOL_INTERCEPTOR(_setjmp)
ASM_TYPE_FUNCTION(ASM_SYMBOL_INTERCEPTOR(_setjmp))
ASM_SYMBOL_INTERCEPTOR(_setjmp):
  ldih $r29, 0($r27)
  ldi $r29, 0($r29)
  CFI_STARTPROC

  // Save frame/link register
  ldi $sp, -32($sp)
  stl $r26, 0($sp)
  stl $fp, 8($sp)
  CFI_DEF_CFA_OFFSET (32)
  CFI_OFFSET (26, -32)
  CFI_OFFSET (15, -24)

  // Adjust the SP for previous frame
  ldi $fp,0($sp)
  CFI_DEF_CFA_REGISTER (15)

  // Save env parameter
  stl $r16, 16($sp)
  CFI_OFFSET (0, -16)

  // Obtain SP, first argument to `void __tsan_setjmp(uptr sp)`
  ldi $r16, 32($sp)

  // call tsan interceptor
  //ldih $r27, ASM_SYMBOL(__tsan_setjmp)($r29) !gprelhigh
  //ldi $r27, ASM_SYMBOL(__tsan_setjmp)($r29) !gprellow
  ldl $r27, ASM_SYMBOL(__tsan_setjmp)($r29) !literal
  call $r26, ($r27), 0
  ldih $r29, 0($r26)
  ldi $r29, 0($r29)

  // Restore env parameter
  ldl $r16, 16($sp)
  CFI_RESTORE (0)

  // Restore frame/link register
  ldl $fp, 8($sp)
  ldl $r26, 0($sp)
  CFI_RESTORE (15)
  CFI_RESTORE (26)
  CFI_DEF_CFA (31, 0)
  ldi $sp, 32($sp)

  // tail jump to libc setjmp
  ldl $r27, _ZN14__interception12real__setjmpE($r29) !literal
  ldl $r27, 0($r27)
  jmp $r31, ($r27), 0

  CFI_ENDPROC
ASM_SIZE(ASM_SYMBOL_INTERCEPTOR(_setjmp))

ASM_HIDDEN(__tsan_setjmp)
.comm _ZN14__interception14real_sigsetjmpE,8,8
.globl ASM_SYMBOL_INTERCEPTOR(sigsetjmp)
ASM_TYPE_FUNCTION(ASM_SYMBOL_INTERCEPTOR(sigsetjmp))
ASM_SYMBOL_INTERCEPTOR(sigsetjmp):
  ldih $r29, 0($r27)
  ldi $r29, 0($r29)
  CFI_STARTPROC

  // Save frame/link register
  ldi $sp, -32($sp)
  stl $r26, 0($sp)
  stl $fp, 8($sp)
  CFI_DEF_CFA_OFFSET (32)
  CFI_OFFSET (26, -32)
  CFI_OFFSET (15, -24)

  // Adjust the SP for previous frame
  ldi $fp,0($sp)
  CFI_DEF_CFA_REGISTER (15)

  // Save env parameter
  stl $r16, 16($sp)
  stl $r17, 24($sp)
  CFI_OFFSET (16, -16)
  CFI_OFFSET (17, -8)

  // Obtain SP, first argument to `void __tsan_setjmp(uptr sp)`
  ldi $r16, 32($sp)

  // call tsan interceptor
  //ldih $r27, ASM_SYMBOL(__tsan_setjmp)($r29) !gprelhigh
  //ldi $r27, ASM_SYMBOL(__tsan_setjmp)($r29) !gprellow
  ldl $r27, ASM_SYMBOL(__tsan_setjmp)($r29) !literal
  call $r26, ($r27), 0
  ldih $r29, 0($r26)
  ldi $r29, 0($r29)

  // Restore env parameter
  ldl $r16, 16($sp)
  ldl $r17, 24($sp)
  CFI_RESTORE (0)
  CFI_RESTORE (1)

  // Restore frame/link register
  ldl $fp, 8($sp)
  ldl $r26, 0($sp)
  CFI_RESTORE (15)
  CFI_RESTORE (26)
  CFI_DEF_CFA (31, 0)
  ldi $sp, 32($sp)

  // tail jump to libc setjmp
  ldl $r27, _ZN14__interception14real_sigsetjmpE($r29) !literal
  ldl $r27, 0($r27)
  jmp $r31, ($r27), 0

  CFI_ENDPROC
ASM_SIZE(ASM_SYMBOL_INTERCEPTOR(sigsetjmp))

ASM_HIDDEN(__tsan_setjmp)
.comm _ZN14__interception16real___sigsetjmpE,8,8
.globl ASM_SYMBOL_INTERCEPTOR(__sigsetjmp)
ASM_TYPE_FUNCTION(ASM_SYMBOL_INTERCEPTOR(__sigsetjmp))
ASM_SYMBOL_INTERCEPTOR(__sigsetjmp):
  ldih $r29, 0($r27)
  ldi $r29, 0($r29)
  CFI_STARTPROC

  // Save frame/link register
  ldi $sp, -32($sp)
  stl $r26, 0($sp)
  stl $fp, 8($sp)
  CFI_DEF_CFA_OFFSET (32)
  CFI_OFFSET (26, -32)
  CFI_OFFSET (15, -24)

  // Adjust the SP for previous frame
  ldi $fp,0($sp)
  CFI_DEF_CFA_REGISTER (15)

  // Save env parameter
  stl $r16, 16($sp)
  stl $r17, 24($sp)
  CFI_OFFSET (16, -16)
  CFI_OFFSET (17, -8)

  // Obtain SP, first argument to `void __tsan_setjmp(uptr sp)`
  ldi $r16, 32($sp)

  // call tsan interceptor
  //ldih $r27, ASM_SYMBOL(__tsan_setjmp)($r29) !gprelhigh
  //ldi $r27, ASM_SYMBOL(__tsan_setjmp)($r29) !gprellow
  ldl $r27, ASM_SYMBOL(__tsan_setjmp)($r29) !literal
  call $r26, ($r27), 0
  ldih $r29, 0($r26)
  ldi $r29, 0($r29)

  // Restore env parameter
  ldl $r16, 16($sp)
  ldl $r17, 24($sp)
  CFI_RESTORE (0)
  CFI_RESTORE (1)

  // Restore frame/link register
  ldl $fp, 8($sp)
  ldl $r26, 0($sp)
  CFI_RESTORE (15)
  CFI_RESTORE (26)
  CFI_DEF_CFA (31, 0)
  ldi $sp, 32($sp)

  // tail jump to libc setjmp
  ldl $r27, _ZN14__interception16real___sigsetjmpE($r29) !literal
  ldl $r27, 0($r27)
  jmp $r31, ($r27), 0

  CFI_ENDPROC
ASM_SIZE(ASM_SYMBOL_INTERCEPTOR(__sigsetjmp))

#endif
