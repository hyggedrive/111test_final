//===-- RegisterContext_sw_64.h --------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_RegisterContext_sw_64_H_
#define liblldb_RegisterContext_sw_64_H_

#include <cstddef>
#include <cstdint>

// eh_frame and DWARF Register numbers (eRegisterKindEHFrame &
// eRegisterKindDWARF)
enum {
  dwarf_r0_sw_64 = 0,
  dwarf_r1_sw_64,
  dwarf_r2_sw_64,
  dwarf_r3_sw_64,
  dwarf_r4_sw_64,
  dwarf_r5_sw_64,
  dwarf_r6_sw_64,
  dwarf_r7_sw_64,
  dwarf_r8_sw_64,
  dwarf_r9_sw_64,
  dwarf_r10_sw_64,
  dwarf_r11_sw_64,
  dwarf_r12_sw_64,
  dwarf_r13_sw_64,
  dwarf_r14_sw_64,
  dwarf_fp_sw_64, //15
  dwarf_r16_sw_64,
  dwarf_r17_sw_64,
  dwarf_r18_sw_64,
  dwarf_r19_sw_64,
  dwarf_r20_sw_64,
  dwarf_r21_sw_64,
  dwarf_r22_sw_64,
  dwarf_r23_sw_64,
  dwarf_r24_sw_64,
  dwarf_r25_sw_64,
  dwarf_ra_sw_64,
  dwarf_r27_sw_64,
  dwarf_r28_sw_64,
  dwarf_gp_sw_64, //29
  dwarf_sp_sw_64, //30
  dwarf_zero_sw_64, //31
  dwarf_pc_sw_64,
  dwarf_f0_sw_64,
  dwarf_f1_sw_64,
  dwarf_f2_sw_64,
  dwarf_f3_sw_64,
  dwarf_f4_sw_64,
  dwarf_f5_sw_64,
  dwarf_f6_sw_64,
  dwarf_f7_sw_64,
  dwarf_f8_sw_64,
  dwarf_f9_sw_64,
  dwarf_f10_sw_64,
  dwarf_f11_sw_64,
  dwarf_f12_sw_64,
  dwarf_f13_sw_64,
  dwarf_f14_sw_64,
  dwarf_f15_sw_64,
  dwarf_f16_sw_64,
  dwarf_f17_sw_64,
  dwarf_f18_sw_64,
  dwarf_f19_sw_64,
  dwarf_f20_sw_64,
  dwarf_f21_sw_64,
  dwarf_f22_sw_64,
  dwarf_f23_sw_64,
  dwarf_f24_sw_64,
  dwarf_f25_sw_64,
  dwarf_f26_sw_64,
  dwarf_f27_sw_64,
  dwarf_f28_sw_64,
  dwarf_f29_sw_64,
  dwarf_f30_sw_64,
  dwarf_f31_sw_64,

  dwarf_v0_sw_64,
  dwarf_v1_sw_64,
  dwarf_v2_sw_64,
  dwarf_v3_sw_64,
  dwarf_v4_sw_64,
  dwarf_v5_sw_64,
  dwarf_v6_sw_64,
  dwarf_v7_sw_64,
  dwarf_v8_sw_64,
  dwarf_v9_sw_64,
  dwarf_v10_sw_64,
  dwarf_v11_sw_64,
  dwarf_v12_sw_64,
  dwarf_v13_sw_64,
  dwarf_v14_sw_64,
  dwarf_v15_sw_64,
  dwarf_v16_sw_64,
  dwarf_v17_sw_64,
  dwarf_v18_sw_64,
  dwarf_v19_sw_64,
  dwarf_v20_sw_64,
  dwarf_v21_sw_64,
  dwarf_v22_sw_64,
  dwarf_v23_sw_64,
  dwarf_v24_sw_64,
  dwarf_v25_sw_64,
  dwarf_v26_sw_64,
  dwarf_v27_sw_64,
  dwarf_v28_sw_64,
  dwarf_v29_sw_64,
  dwarf_v30_sw_64,
  dwarf_v31_sw_64,

};

// GP registers
struct GPR_linux_sw_64 {
  uint64_t r0;
  uint64_t r1;
  uint64_t r2;
  uint64_t r3;
  uint64_t r4;
  uint64_t r5;
  uint64_t r6;
  uint64_t r7;
  uint64_t r8;
  uint64_t r9;
  uint64_t r10;
  uint64_t r11;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t fp;
  uint64_t r16;
  uint64_t r17;
  uint64_t r18;
  uint64_t r19;
  uint64_t r20;
  uint64_t r21;
  uint64_t r22;
  uint64_t r23;
  uint64_t r24;
  uint64_t r25;
  uint64_t ra;
  uint64_t r27;
  uint64_t r28;
  uint64_t gp;
  uint64_t sp;
  uint64_t zero;
  uint64_t pc;
};

// Floating Point Registers
struct FPR_linux_sw_64 {
  uint64_t f0;
  uint64_t f1;
  uint64_t f2;
  uint64_t f3;
  uint64_t f4;
  uint64_t f5;
  uint64_t f6;
  uint64_t f7;
  uint64_t f8;
  uint64_t f9;
  uint64_t f10;
  uint64_t f11;
  uint64_t f12;
  uint64_t f13;
  uint64_t f14;
  uint64_t f15;
  uint64_t f16;
  uint64_t f17;
  uint64_t f18;
  uint64_t f19;
  uint64_t f20;
  uint64_t f21;
  uint64_t f22;
  uint64_t f23;
  uint64_t f24;
  uint64_t f25;
  uint64_t f26;
  uint64_t f27;
  uint64_t f28;
  uint64_t f29;
  uint64_t f30;
  uint64_t f31;
};


struct SIMDReg_sw_64 {
  uint8_t byte[16]; // 128-bits for each SIMD register
};

struct SIMD_linux_sw_64 {
  SIMDReg_sw_64 v0;
  SIMDReg_sw_64 v1;
  SIMDReg_sw_64 v2;
  SIMDReg_sw_64 v3;
  SIMDReg_sw_64 v4;
  SIMDReg_sw_64 v5;
  SIMDReg_sw_64 v6;
  SIMDReg_sw_64 v7;
  SIMDReg_sw_64 v8;
  SIMDReg_sw_64 v9;
  SIMDReg_sw_64 v10;
  SIMDReg_sw_64 v11;
  SIMDReg_sw_64 v12;
  SIMDReg_sw_64 v13;
  SIMDReg_sw_64 v14;
  SIMDReg_sw_64 v15;
  SIMDReg_sw_64 v16;
  SIMDReg_sw_64 v17;
  SIMDReg_sw_64 v18;
  SIMDReg_sw_64 v19;
  SIMDReg_sw_64 v20;
  SIMDReg_sw_64 v21;
  SIMDReg_sw_64 v22;
  SIMDReg_sw_64 v23;
  SIMDReg_sw_64 v24;
  SIMDReg_sw_64 v25;
  SIMDReg_sw_64 v26;
  SIMDReg_sw_64 v27;
  SIMDReg_sw_64 v28;
  SIMDReg_sw_64 v29;
  SIMDReg_sw_64 v30;
  SIMDReg_sw_64 v31;
//  uint32_t fcsr;    /* FPU control status register */
};

struct UserArea_sw_64 {
  GPR_linux_sw_64 gpr; // General purpose registers.
  FPR_linux_sw_64 fpr; // Floating point registers.
  SIMD_linux_sw_64 simd; // SIMD registers.
};

#endif // liblldb_RegisterContext_sw_64_H_
