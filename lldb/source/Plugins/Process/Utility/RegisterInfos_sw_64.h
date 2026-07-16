//===-- RegisterInfos_sw_64.h ----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <stddef.h>

#include "lldb/Core/dwarf.h"
#include "llvm/Support/Compiler.h"


#ifdef DECLARE_REGISTER_INFOS_SW64_STRUCT

// Computes the offset of the given GPR in the user data area.
#ifdef LINUX_SW64
#define GPR_OFFSET(regname)                                                    \
   (LLVM_EXTENSION offsetof(UserArea_sw_64, gpr) +                                   \
   LLVM_EXTENSION offsetof(GPR_linux_sw_64, regname))
#endif

// Computes the offset of the given FPR in the extended data area.
#define FPR_OFFSET(regname)                                                    \
   (LLVM_EXTENSION offsetof(UserArea_sw_64, fpr) +                                    \
   LLVM_EXTENSION offsetof(FPR_linux_sw_64, regname))

// Computes the offset of the given SIMD in the extended data area.
#define SIMD_OFFSET(regname)                                                    \
  (LLVM_EXTENSION offsetof(UserArea_sw_64, simd) +                                    \
   LLVM_EXTENSION offsetof(SIMD_linux_sw_64, regname))

// RegisterKind: EHFrame, DWARF, Generic, Process Plugin, LLDB

// Note that the size and offset will be updated by platform-specific classes.
#ifdef LINUX_SW64
#define DEFINE_GPR(reg, alt, kind1, kind2, kind3)                              \
  {                                                                            \
    #reg, alt, sizeof(((GPR_linux_sw_64 *) 0)->reg),                            \
                      GPR_OFFSET(reg), eEncodingUint, eFormatHex,              \
                                 {kind1, kind2, kind3, ptrace_##reg##_sw_64,    \
                                  gpr_##reg##_sw_64 },                        \
                                  NULL, NULL, NULL,                          \
  }
#endif

#define DEFINE_GPR_INFO(reg, alt, kind1, kind2, kind3)                         \
  {                                                                            \
    #reg, alt, sizeof(((GPR_linux_sw_64 *) 0)->reg) / 2,                        \
                      GPR_OFFSET(reg), eEncodingUint, eFormatHex,              \
                                 {kind1, kind2, kind3, ptrace_##reg##_sw_64,    \
                                  gpr_##reg##_sw_64 },                        \
                                  NULL, NULL, NULL,                          \
  }
/*
const uint8_t dwarf_opcode_mips64[] = {
    llvm::dwarf::DW_OP_regx,  dwarf_sr_mips64,        llvm::dwarf::DW_OP_lit1,
    llvm::dwarf::DW_OP_lit26, llvm::dwarf::DW_OP_shl, llvm::dwarf::DW_OP_and,
    llvm::dwarf::DW_OP_lit26, llvm::dwarf::DW_OP_shr};
*/
#define DEFINE_FPR(reg, alt, kind1, kind2, kind3)                              \
  {                                                                            \
    #reg, alt, sizeof(((FPR_linux_sw_64 *) 0)->reg),                            \
                      FPR_OFFSET(reg), eEncodingIEEE754, eFormatFloat,         \
                                 {kind1, kind2, kind3, ptrace_##reg##_sw_64,    \
                                  fpr_##reg##_sw_64 },                        \
                                  NULL, NULL, NULL,                          \
  }

#define DEFINE_FPR_INFO(reg, alt, kind1, kind2, kind3)                         \
  {                                                                            \
    #reg, alt, sizeof(((FPR_linux_sw_64 *) 0)->reg),                            \
                      FPR_OFFSET(reg), eEncodingUint, eFormatHex,              \
                                 {kind1, kind2, kind3, ptrace_##reg##_sw_64,    \
                                  fpr_##reg##_sw_64 },                        \
                                  NULL, NULL, NULL,                          \
  }


#define DEFINE_SIMD(reg, alt, kind1, kind2, kind3, kind4)                       \
  {                                                                            \
    #reg, alt, sizeof(((SIMD_linux_sw_64 *) 0)->reg),                            \
                      SIMD_OFFSET(reg), eEncodingVector, eFormatVectorOfUInt8,  \
                                 {kind1, kind2, kind3, kind4,                  \
                                  simd_##reg##_sw_64 },                        \
                                  NULL, NULL, NULL,                          \
  }

#define DEFINE_SIMD_INFO(reg, alt, kind1, kind2, kind3, kind4)                  \
  {                                                                            \
    #reg, alt, sizeof(((SIMD_linux_sw_64 *) 0)->reg),                            \
                      SIMD_OFFSET(reg), eEncodingUint, eFormatHex,              \
                                 {kind1, kind2, kind3, kind4,                  \
                                  simd_##reg##_sw_64 },                        \
                                  NULL, NULL, NULL,                          \
  }

static RegisterInfo g_register_infos_sw_64[] = {
// General purpose registers.            EH_Frame,                  DWARF,
// Generic,    Process Plugin
#ifdef LINUX_SW64 
    DEFINE_GPR(r0, "v0", dwarf_r0_sw_64, dwarf_r0_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r1, "t0", dwarf_r1_sw_64, dwarf_r1_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r2, "t1", dwarf_r2_sw_64, dwarf_r2_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r3, "t2", dwarf_r3_sw_64, dwarf_r3_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r4, "t3", dwarf_r4_sw_64, dwarf_r4_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r5, "t4", dwarf_r5_sw_64, dwarf_r5_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r6, "t5", dwarf_r6_sw_64, dwarf_r6_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r7, "t6", dwarf_r7_sw_64, dwarf_r7_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r8, "t7", dwarf_r8_sw_64, dwarf_r8_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r9, "s0", dwarf_r9_sw_64, dwarf_r9_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r10, "s1", dwarf_r10_sw_64, dwarf_r10_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r11, "s2", dwarf_r11_sw_64, dwarf_r11_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r12, "s3", dwarf_r12_sw_64, dwarf_r12_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r13, "s4", dwarf_r13_sw_64, dwarf_r13_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r14, "s5", dwarf_r14_sw_64, dwarf_r14_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(fp, "fp", dwarf_fp_sw_64, dwarf_fp_sw_64,
               LLDB_REGNUM_GENERIC_FP),
    DEFINE_GPR(r16, "a0", dwarf_r16_sw_64, dwarf_r16_sw_64,
               LLDB_REGNUM_GENERIC_ARG1),
    DEFINE_GPR(r17, "a1", dwarf_r17_sw_64, dwarf_r17_sw_64,
               LLDB_REGNUM_GENERIC_ARG2),
    DEFINE_GPR(r18, "a2", dwarf_r18_sw_64, dwarf_r18_sw_64,
               LLDB_REGNUM_GENERIC_ARG3),
    DEFINE_GPR(r19, "a3", dwarf_r19_sw_64, dwarf_r19_sw_64,
               LLDB_REGNUM_GENERIC_ARG4),
    DEFINE_GPR(r20, "a4", dwarf_r20_sw_64, dwarf_r20_sw_64,
               LLDB_REGNUM_GENERIC_ARG5),
    DEFINE_GPR(r21, "a5", dwarf_r21_sw_64, dwarf_r21_sw_64,
               LLDB_REGNUM_GENERIC_ARG6),
    DEFINE_GPR(r22, "t8", dwarf_r22_sw_64, dwarf_r22_sw_64,
               LLDB_REGNUM_GENERIC_ARG7),
    DEFINE_GPR(r23, "t9", dwarf_r23_sw_64, dwarf_r23_sw_64,
               LLDB_REGNUM_GENERIC_ARG8),
    DEFINE_GPR(r24, "t10", dwarf_r24_sw_64, dwarf_r24_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r25, "t11", dwarf_r25_sw_64, dwarf_r25_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(ra, "ra", dwarf_ra_sw_64, dwarf_ra_sw_64,
               LLDB_REGNUM_GENERIC_RA),
    DEFINE_GPR(r27, "pv", dwarf_r27_sw_64, dwarf_r27_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(r28, "at", dwarf_r28_sw_64, dwarf_r28_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(gp, "gp", dwarf_gp_sw_64, dwarf_gp_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(sp, "sp", dwarf_sp_sw_64, dwarf_sp_sw_64,
               LLDB_REGNUM_GENERIC_SP),
    DEFINE_GPR(zero, "zero", dwarf_zero_sw_64, dwarf_zero_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_GPR(pc, "pc", dwarf_pc_sw_64, dwarf_pc_sw_64,
               LLDB_REGNUM_GENERIC_PC),
    DEFINE_FPR(f0, nullptr, dwarf_f0_sw_64, dwarf_f0_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f1, nullptr, dwarf_f1_sw_64, dwarf_f1_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f2, nullptr, dwarf_f2_sw_64, dwarf_f2_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f3, nullptr, dwarf_f3_sw_64, dwarf_f3_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f4, nullptr, dwarf_f4_sw_64, dwarf_f4_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f5, nullptr, dwarf_f5_sw_64, dwarf_f5_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f6, nullptr, dwarf_f6_sw_64, dwarf_f6_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f7, nullptr, dwarf_f7_sw_64, dwarf_f7_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f8, nullptr, dwarf_f8_sw_64, dwarf_f8_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f9, nullptr, dwarf_f9_sw_64, dwarf_f9_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f10, nullptr, dwarf_f10_sw_64, dwarf_f10_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f11, nullptr, dwarf_f11_sw_64, dwarf_f11_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f12, nullptr, dwarf_f12_sw_64, dwarf_f12_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f13, nullptr, dwarf_f13_sw_64, dwarf_f13_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f14, nullptr, dwarf_f14_sw_64, dwarf_f14_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f15, nullptr, dwarf_f15_sw_64, dwarf_f15_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f16, nullptr, dwarf_f16_sw_64, dwarf_f16_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f17, nullptr, dwarf_f17_sw_64, dwarf_f17_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f18, nullptr, dwarf_f18_sw_64, dwarf_f18_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f19, nullptr, dwarf_f19_sw_64, dwarf_f19_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f20, nullptr, dwarf_f20_sw_64, dwarf_f20_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f21, nullptr, dwarf_f21_sw_64, dwarf_f21_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f22, nullptr, dwarf_f22_sw_64, dwarf_f22_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f23, nullptr, dwarf_f23_sw_64, dwarf_f23_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f24, nullptr, dwarf_f24_sw_64, dwarf_f24_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f25, nullptr, dwarf_f25_sw_64, dwarf_f25_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f26, nullptr, dwarf_f26_sw_64, dwarf_f26_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f27, nullptr, dwarf_f27_sw_64, dwarf_f27_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f28, nullptr, dwarf_f28_sw_64, dwarf_f28_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f29, nullptr, dwarf_f29_sw_64, dwarf_f29_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f30, nullptr, dwarf_f30_sw_64, dwarf_f30_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_FPR(f31, nullptr, dwarf_f31_sw_64, dwarf_f31_sw_64,
               LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v0, nullptr, dwarf_v0_sw_64, dwarf_v0_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v1, nullptr, dwarf_v1_sw_64, dwarf_v1_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v2, nullptr, dwarf_v2_sw_64, dwarf_v2_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v3, nullptr, dwarf_v3_sw_64, dwarf_v3_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v4, nullptr, dwarf_v4_sw_64, dwarf_v4_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v5, nullptr, dwarf_v5_sw_64, dwarf_v5_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v6, nullptr, dwarf_v6_sw_64, dwarf_v6_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v7, nullptr, dwarf_v7_sw_64, dwarf_v7_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v8, nullptr, dwarf_v8_sw_64, dwarf_v8_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v9, nullptr, dwarf_v9_sw_64, dwarf_v9_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v10, nullptr, dwarf_v10_sw_64, dwarf_v10_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v11, nullptr, dwarf_v11_sw_64, dwarf_v11_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v12, nullptr, dwarf_v12_sw_64, dwarf_v12_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v13, nullptr, dwarf_v13_sw_64, dwarf_v13_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v14, nullptr, dwarf_v14_sw_64, dwarf_v14_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v15, nullptr, dwarf_v15_sw_64, dwarf_v15_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v16, nullptr, dwarf_v16_sw_64, dwarf_v16_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v17, nullptr, dwarf_v17_sw_64, dwarf_v17_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v18, nullptr, dwarf_v18_sw_64, dwarf_v18_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v19, nullptr, dwarf_v19_sw_64, dwarf_v19_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v20, nullptr, dwarf_v20_sw_64, dwarf_v20_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v21, nullptr, dwarf_v21_sw_64, dwarf_v21_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v22, nullptr, dwarf_v22_sw_64, dwarf_v22_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v23, nullptr, dwarf_v23_sw_64, dwarf_v23_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v24, nullptr, dwarf_v24_sw_64, dwarf_v24_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v25, nullptr, dwarf_v25_sw_64, dwarf_v25_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v26, nullptr, dwarf_v26_sw_64, dwarf_v26_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v27, nullptr, dwarf_v27_sw_64, dwarf_v27_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v28, nullptr, dwarf_v28_sw_64, dwarf_v28_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v29, nullptr, dwarf_v29_sw_64, dwarf_v29_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v30, nullptr, dwarf_v30_sw_64, dwarf_v30_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),
    DEFINE_SIMD(v31, nullptr, dwarf_v31_sw_64, dwarf_v31_sw_64,
               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM),

#endif
};

static_assert((sizeof(g_register_infos_sw_64) /
               sizeof(g_register_infos_sw_64[0])) == k_num_registers_sw_64,
              "g_register_infos_sw_64 has wrong number of register infos");

#undef DEFINE_GPR
#undef DEFINE_GPR_INFO
#undef DEFINE_FPR
#undef DEFINE_FPR_INFO
#undef GPR_OFFSET
#undef FPR_OFFSET

#endif // DECLARE_REGISTER_INFOS_SW64_STRUCT
