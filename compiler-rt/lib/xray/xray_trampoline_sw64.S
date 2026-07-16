#include "../builtins/assembly.h"
  .text
  .file "xray_trampoline_sw64.S"
  .global _ZN6__xray19XRayPatchedFunctionE
  .p2align 3
  .global __xray_FunctionEntry
  .type __xray_FunctionEntry,@function
  .ent __xray_FunctionEntry
__xray_FunctionEntry:
  ldih    $29,0($27)         !gpdisp!1
  ldi     $29,0($29)          !gpdisp!1
  ldi     $30,-112($30)
  stl     $26, 104($30)
  stl     $29, 96($30)
  stl     $21, 88($30)
  stl     $20, 80($30)
  stl     $19, 72($30)
  stl     $18, 64($30)
  stl     $17, 56($30)
  stl     $16, 48($30)
  fstd    $f21, 40($30)
  fstd    $f20, 32($30)
  fstd    $f19, 24($30)
  fstd    $f18, 16($30)
  fstd    $f17, 8($30)
  fstd    $f16, 0($30)
  ldl     $27, _ZN6__xray19XRayPatchedFunctionE($29)  !literal!2
  ldl     $27, 0($27)
  beq     $27, FunctionEntry_restore
  call    $26, ($27),_ZN6__xray19XRayPatchedFunctionE  !lituse_jsr!2
FunctionEntry_restore:
  fldd    $f16, 0($30)
  fldd    $f17, 8($30)
  fldd    $f18, 16($30)
  fldd    $f19, 24($30)
  fldd    $f20, 32($30)
  fldd    $f21, 40($30)
  ldl     $16, 48($30)
  ldl     $17, 56($30)
  ldl     $18, 64($30)
  ldl     $19, 72($30)
  ldl     $20, 80($30)
  ldl     $21, 88($30)
  ldl     $29, 96($30)
  ldl     $26, 104($30)
  addl    $30, 112, $30
  ret     $31,($26),1
  .end    __xray_FunctionEntry
.Lfunc_end0:
        .size   __xray_FunctionEntry, .Lfunc_end0-__xray_FunctionEntry

FunctionEntry_end:
  .size __xray_FunctionEntry, FunctionEntry_end-__xray_FunctionEntry

  .p2align 3
  .global __xray_FunctionExit
  .type __xray_FunctionExit,@function
__xray_FunctionExit:
  ldih    $29,0($27)         !gpdisp!3
  ldi     $29,0($29)          !gpdisp!3
  ldi     $30,-112($30)
  stl     $26, 104($30)
  stl     $29, 96($30)
  stl     $21, 88($30)
  stl     $20, 80($30)
  stl     $19, 72($30)
  stl     $18, 64($30)
  stl     $17, 56($30)
  stl     $16, 48($30)
  fstd    $f21, 40($30)
  fstd    $f20, 32($30)
  fstd    $f19, 24($30)
  fstd    $f18, 16($30)
  fstd    $f17, 8($30)
  fstd    $f16, 0($30)
  ldl     $27, _ZN6__xray19XRayPatchedFunctionE($29)  !literal!4
  ldl     $27, 0($27)
  beq     $27, FunctionExit_restore
  call    $26, ($27),_ZN6__xray19XRayPatchedFunctionE !lituse_jsr!4
FunctionExit_restore:
  fldd    $f16, 0($30)
  fldd    $f17, 8($30)
  fldd    $f18, 16($30)
  fldd    $f19, 24($30)
  fldd    $f20, 32($30)
  fldd    $f21, 40($30)
  ldl     $16, 48($30)
  ldl     $17, 56($30)
  ldl     $18, 64($30)
  ldl     $19, 72($30)
  ldl     $20, 80($30)
  ldl     $21, 88($30)
  ldl     $29, 96($30)
  ldl     $26, 104($30)
  addl    $30, 112, $30
  ret     $31,($26),1
  .end    __xray_FunctionExit
.Lfunc_end1:
        .size   __xray_FunctionExit, .Lfunc_end1-__xray_FunctionExit
