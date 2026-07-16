//===-- RegisterContextLinux_sw_64.cpp ------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===---------------------------------------------------------------------===//


#include <stddef.h>
#include <vector>

// For eh_frame and DWARF Register numbers
#include "RegisterContextLinux_sw_64.h"

// For GP and FP buffers
#include "RegisterContext_sw_64.h"

// Internal codes for all sw_64 registers
#include "lldb-sw_64-linux-register-enums.h"

using namespace lldb;
using namespace lldb_private;

// Include RegisterInfos_sw_64 to declare our g_register_infos_sw_64
// structure.
#define DECLARE_REGISTER_INFOS_SW64_STRUCT
#define LINUX_SW64
#include "RegisterInfos_sw_64.h"
#undef LINUX_SW64
#undef DECLARE_REGISTER_INFOS_SW64_STRUCT

// sw_64 general purpose registers.
const uint32_t g_gp_regnums_sw_64[] = {
    gpr_r0_sw_64,      gpr_r1_sw_64,    gpr_r2_sw_64,
    gpr_r3_sw_64,      gpr_r4_sw_64,    gpr_r5_sw_64,
    gpr_r6_sw_64,      gpr_r7_sw_64,    gpr_r8_sw_64,
    gpr_r9_sw_64,      gpr_r10_sw_64,   gpr_r11_sw_64,
    gpr_r12_sw_64,     gpr_r13_sw_64,   gpr_r14_sw_64,
    gpr_fp_sw_64,      gpr_r16_sw_64,   gpr_r17_sw_64,
    gpr_r18_sw_64,     gpr_r19_sw_64,   gpr_r20_sw_64,
    gpr_r21_sw_64,     gpr_r22_sw_64,   gpr_r23_sw_64,
    gpr_r24_sw_64,     gpr_r25_sw_64,   gpr_ra_sw_64,
    gpr_r27_sw_64,     gpr_r28_sw_64,   gpr_gp_sw_64,
    gpr_sp_sw_64,      gpr_zero_sw_64,	gpr_pc_sw_64,
    LLDB_INVALID_REGNUM // register sets need to end with this flag
};

static_assert((sizeof(g_gp_regnums_sw_64) / sizeof(g_gp_regnums_sw_64[0])) -
                      1 ==
                  k_num_gpr_registers_sw_64,
              "g_gp_regnums_sw_64 has wrong number of register infos");

// sw_64 floating point registers.
const uint32_t g_fp_regnums_sw_64[] = {
    fpr_f0_sw_64,      fpr_f1_sw_64,  fpr_f2_sw_64,      fpr_f3_sw_64,
    fpr_f4_sw_64,      fpr_f5_sw_64,  fpr_f6_sw_64,      fpr_f7_sw_64,
    fpr_f8_sw_64,      fpr_f9_sw_64,  fpr_f10_sw_64,     fpr_f11_sw_64,
    fpr_f12_sw_64,     fpr_f13_sw_64, fpr_f14_sw_64,     fpr_f15_sw_64,
    fpr_f16_sw_64,     fpr_f17_sw_64, fpr_f18_sw_64,     fpr_f19_sw_64,
    fpr_f20_sw_64,     fpr_f21_sw_64, fpr_f22_sw_64,     fpr_f23_sw_64,
    fpr_f24_sw_64,     fpr_f25_sw_64, fpr_f26_sw_64,     fpr_f27_sw_64,
    fpr_f28_sw_64,     fpr_f29_sw_64, fpr_f30_sw_64,     fpr_f31_sw_64,
    LLDB_INVALID_REGNUM // register sets need to end with this flag
};

static_assert((sizeof(g_fp_regnums_sw_64) / sizeof(g_fp_regnums_sw_64[0])) -
                      1 ==
                  k_num_fpr_registers_sw_64,
              "g_fp_regnums_sw_64 has wrong number of register infos");

// sw_64 SIMD registers.
const uint32_t g_simd_regnums_sw_64[] = {
    simd_v0_sw_64,      simd_v1_sw_64,  simd_v2_sw_64,   simd_v3_sw_64,
    simd_v4_sw_64,      simd_v5_sw_64,  simd_v6_sw_64,   simd_v7_sw_64,
    simd_v8_sw_64,      simd_v9_sw_64,  simd_v10_sw_64,  simd_v11_sw_64,
    simd_v12_sw_64,     simd_v13_sw_64, simd_v14_sw_64,  simd_v15_sw_64,
    simd_v16_sw_64,     simd_v17_sw_64, simd_v18_sw_64,  simd_v19_sw_64,
    simd_v20_sw_64,     simd_v21_sw_64, simd_v22_sw_64,  simd_v23_sw_64,
    simd_v24_sw_64,     simd_v25_sw_64, simd_v26_sw_64,  simd_v27_sw_64,
    simd_v28_sw_64,     simd_v29_sw_64, simd_v30_sw_64,  simd_v31_sw_64,
    LLDB_INVALID_REGNUM // register sets need to end with this flag
};

static_assert((sizeof(g_simd_regnums_sw_64) / sizeof(g_simd_regnums_sw_64[0])) -
                      1 ==
                  k_num_simd_registers_sw_64,
              "g_simd_regnums_sw_64 has wrong number of register infos");

// Number of register sets provided by this context.
//constexpr size_t k_num_register_sets = 3;
enum { k_num_register_sets = 3 };

// Register sets for sw_64.
static const RegisterSet g_reg_sets_sw_64[k_num_register_sets] = {
    {"General Purpose Registers", "gpr", k_num_gpr_registers_sw_64,
     g_gp_regnums_sw_64},
    {"Floating Point Registers", "fpu", k_num_fpr_registers_sw_64,
     g_fp_regnums_sw_64},
    {"SIMD Registers", "simd", k_num_simd_registers_sw_64, g_simd_regnums_sw_64},
};

const RegisterSet *
RegisterContextLinux_sw_64::GetRegisterSet(size_t set) const {
  if (set >= k_num_register_sets)
    return nullptr;

  switch (GetTargetArchitecture().GetMachine()/*m_target_arch.GetMachine()*/) {
  case llvm::Triple::sw_64:
    return &g_reg_sets_sw_64[set];
  default:
    assert(false && "Unhandled target architecture.");
    return nullptr;
  }
  return nullptr;
}

size_t
RegisterContextLinux_sw_64::GetRegisterSetCount() const {
  return k_num_register_sets;
}

static const RegisterInfo *GetRegisterInfoPtr(const ArchSpec &target_arch) {
  switch (target_arch.GetMachine()) {
  case llvm::Triple::sw_64:
    return g_register_infos_sw_64;
  default:
    assert(false && "Unhandled target architecture.");
    return nullptr;
  }
}

static uint32_t GetRegisterInfoCount(const ArchSpec &target_arch) {
  switch (target_arch.GetMachine()) {
  case llvm::Triple::sw_64:
    return static_cast<uint32_t>(sizeof(g_register_infos_sw_64) /
                                 sizeof(g_register_infos_sw_64[0]));
  default:
    assert(false && "Unhandled target architecture.");
    return 0;
  }
}

//uint32_t GetUserRegisterInfoCount_sw_64(const ArchSpec &target_arch, bool simd_present) {
uint32_t GetUserRegisterInfoCount_sw_64(const ArchSpec &target_arch) {
  switch (target_arch.GetMachine()) {
   case llvm::Triple::sw_64:
    return static_cast<uint32_t>(k_num_user_registers_sw_64);
  default:
    assert(false && "Unhandled target architecture.");
    return 0;
  }
}

RegisterContextLinux_sw_64::RegisterContextLinux_sw_64(
//    const ArchSpec &target_arch, bool simd_present)
    const ArchSpec &target_arch)
    : lldb_private::RegisterInfoInterface(target_arch),
      m_register_info_p(GetRegisterInfoPtr(target_arch)),
      m_register_info_count(GetRegisterInfoCount(target_arch)),
      m_user_register_count(
//          GetUserRegisterInfoCount_sw_64(target_arch, simd_present)) {}
          GetUserRegisterInfoCount_sw_64(target_arch)) {}

size_t RegisterContextLinux_sw_64::GetGPRSize() const {
  return sizeof(GPR_linux_sw_64);
}

const RegisterInfo *RegisterContextLinux_sw_64::GetRegisterInfo() const {
  return m_register_info_p;
}

uint32_t RegisterContextLinux_sw_64::GetRegisterCount() const {
  return m_register_info_count;
}

uint32_t RegisterContextLinux_sw_64::GetUserRegisterCount() const {
  return m_user_register_count;
}

