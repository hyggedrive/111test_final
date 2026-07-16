//===-- xray_Sw64.cpp ----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of XRay, a dynamic runtime instrumentation system.
//
// Implementation of Sw64-specific routines (64-bit).
//
//===----------------------------------------------------------------------===//
#include "sanitizer_common/sanitizer_common.h"
#include "xray_defs.h"
#include "xray_interface_internal.h"
#include <atomic>
#include <cassert>

extern "C" void __clear_cache(void *start, void *end);

namespace __xray {

// The machine codes for some instructions used in runtime patching.
enum class PatchOpcodes : uint32_t {
  PO_BR = 0x13e00012, // br 0x90
};

inline static bool patchSled(const bool Enable, const uint32_t FuncId,
                             const XRaySledEntry &Sled,
                             void (*TracingHook)()) XRAY_NEVER_INSTRUMENT {
  // When |Enable| == true,
  // We replace the following compile-time stub (sled):
  //
  // xray_sled_n:
  //   ldi $r27,76($r27)
  //   br $r31,68
  //   17 NOPs (68 bytes)
  //
  // With the following runtime patch:
  //
  // xray_sled_n:
  //	ldi      sp,-24(sp)
  //	stl      ra,0(sp)
  //	stl      fp,8(sp)
  //	stl      $r27,16(sp)
  //	clr      $r27
  //	ldih     $r27,0($r27)
  //	ldi      $r27,0($r27)
  //	slll     $r27,0x20,$r27
  //	ldih     $r27,0($r27)
  //	ldi      $r27,0($r27)
  //	clr      $r16
  //	ldih     $r16,0($r16)
  //	ldi      $r16,0($r16)
  //	call     ra,($r27),0x2c
  //	ldl      $r27,16(sp)
  //	ldl      fp,8(sp)
  //	ldl      ra,0(sp)
  //	ldi      sp,24(sp)
  //	ldi      $r27,76($r27)
  //
  // Replacement of the first 4-byte instruction should be the last and atomic
  // operation, so that the user code which reaches the sled concurrently
  // either jumps over the whole sled, or executes the whole sled when the
  // latter is ready.
  //
  // When |Enable|==false, we set back the first instruction in the sled to be
  //   br $r31,72

  uint32_t *FirstAddress = reinterpret_cast<uint32_t *>(Sled.address());
  uint32_t *CurAddress = FirstAddress;
  if (Enable) {
    uint32_t LoTracingHookAddr =
        reinterpret_cast<uint64_t>(TracingHook) & 0xffff;
    uint32_t HiTracingHookAddr =
        (reinterpret_cast<uint64_t>(TracingHook + 0x8000) >> 16) & 0xffff;
    uint32_t HigherTracingHookAddr =
        (reinterpret_cast<uint64_t>(TracingHook + 0x80008000) >> 32) & 0xffff;
    uint32_t HighestTracingHookAddr =
        (reinterpret_cast<uint64_t>(TracingHook + 0x800080008000) >> 48) &
        0xffff;
    uint32_t LoFunctionID = FuncId & 0xffff;
    uint32_t HiFunctionID = ((FuncId + 0x8000) >> 16) & 0xffff;

    // CurAddress[0]  =  0xfbdeffe8;                     // ldi	sp,-24(sp)
    CurAddress[1] = 0xaf5e0000;                          // stl	ra,0(sp)
    CurAddress[2] = 0xadfe0008;                          // stl	fp,8(sp)
    CurAddress[3] = 0xaf7e0010;                          // stl	$r27,16(sp)
    CurAddress[4] = 0x43ff075b;                          // clr	$r27
    CurAddress[5] = 0xff7b0000 | HighestTracingHookAddr; // ldih $r27,0($r27)
    CurAddress[6] = 0xfb7b0000 | HigherTracingHookAddr;  // ldi $r27,0($r27)
    CurAddress[7] = 0x4b64091b;                          // slll $r27,0x20,$r27
    CurAddress[8] = 0xff7b0000 | HiTracingHookAddr;      // ldih $r27,0($r27)
    CurAddress[9] = 0xfb7b0000 | LoTracingHookAddr;      // ldi $r27,0($r27)
    CurAddress[10] = 0x43ff0750;                         // clr	$r16
    CurAddress[11] = 0xfe100000 | HiFunctionID;          // ldih $r16,0($r16)
    CurAddress[12] = 0xfa100000 | LoFunctionID;          // ldi $r16,0($r16)
    CurAddress[13] = 0x075b0000;                         // call ra,($r27),0x2c
    CurAddress[14] = 0x8f7e0010;                         // ldl	$r27,16(sp)
    CurAddress[15] = 0x8dfe0008;                         // ldl	fp,8(sp)
    CurAddress[16] = 0x8f5e0000;                         // ldl	ra,0(sp)
    CurAddress[17] = 0xfbde0018;                         // ldi	sp,24(sp)
    CurAddress[18] = 0xfb7b004c;                         // ldi $r27,76($r27)
    uint32_t CreateStackSpace = 0xfbdeffe8;

    std::atomic_store_explicit(
        reinterpret_cast<std::atomic<uint32_t> *>(FirstAddress),
        CreateStackSpace, std::memory_order_release);
  } else {
    std::atomic_store_explicit(
        reinterpret_cast<std::atomic<uint32_t> *>(FirstAddress),
        uint32_t(PatchOpcodes::PO_BR), std::memory_order_release);
  }
  __clear_cache(reinterpret_cast<char *>(FirstAddress),
                reinterpret_cast<char *>(CurAddress));
  return true;
}

bool patchFunctionEntry(const bool Enable, const uint32_t FuncId,
                        const XRaySledEntry &Sled,
                        void (*Trampoline)()) XRAY_NEVER_INSTRUMENT {
  return patchSled(Enable, FuncId, Sled, Trampoline);
}

bool patchFunctionExit(const bool Enable, const uint32_t FuncId,
                       const XRaySledEntry &Sled) XRAY_NEVER_INSTRUMENT {
  return patchSled(Enable, FuncId, Sled, __xray_FunctionExit);
}

bool patchFunctionTailExit(const bool Enable, const uint32_t FuncId,
                           const XRaySledEntry &Sled) XRAY_NEVER_INSTRUMENT {
  return patchSled(Enable, FuncId, Sled, __xray_FunctionExit);
}

bool patchCustomEvent(const bool Enable, const uint32_t FuncId,
                      const XRaySledEntry &Sled) XRAY_NEVER_INSTRUMENT {
  // FIXME
  return false;
}

bool patchTypedEvent(const bool Enable, const uint32_t FuncId,
                     const XRaySledEntry &Sled) XRAY_NEVER_INSTRUMENT {
  // FIXME
  return false;
}

} // namespace __xray

extern "C" void __xray_ArgLoggerEntry() XRAY_NEVER_INSTRUMENT {
  // FIXME: this will have to be implemented in the trampoline assembly file
}
