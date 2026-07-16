//===-- RegisterContextPOSIX_sw_64.cpp -------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <cstring>
#include <errno.h>
#include <stdint.h>

#include "lldb/Target/Process.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"
#include "lldb/Utility/DataBufferHeap.h"
#include "lldb/Utility/DataExtractor.h"
#include "lldb/Utility/Endian.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Scalar.h"
#include "llvm/Support/Compiler.h"

#include "RegisterContextPOSIX_sw_64.h"
//#include "RegisterContextFreeBSD_mips64.h"
#include "RegisterContextLinux_sw_64.h"
//#include "RegisterContextLinux_mips.h"

using namespace lldb_private;
using namespace lldb;

bool RegisterContextPOSIX_sw_64::IsGPR(unsigned reg) {
  return reg < m_registers_count[gpr_registers_count]; // GPR's come first.
}

bool RegisterContextPOSIX_sw_64::IsFPR(unsigned reg) {
  int set = GetRegisterSetCount();
  if (set > 1)
    return reg < (m_registers_count[fpr_registers_count]
                  + m_registers_count[gpr_registers_count]);
  return false;
}

RegisterContextPOSIX_sw_64::RegisterContextPOSIX_sw_64(
    Thread &thread, uint32_t concrete_frame_idx,
    RegisterInfoInterface *register_info)
    : RegisterContext(thread, concrete_frame_idx) {
  m_register_info_up.reset(register_info);
  m_num_registers = GetRegisterCount();
  int set = GetRegisterSetCount();

  const RegisterSet *reg_set_ptr;
  for(int i = 0; i < set; ++i) {
      reg_set_ptr = GetRegisterSet(i);
      m_registers_count[i] = reg_set_ptr->num_registers;
  }

  assert(m_num_registers ==
         static_cast<uint32_t>(m_registers_count[gpr_registers_count] +
                               m_registers_count[fpr_registers_count] +
                               m_registers_count[simd_registers_count]));
}

RegisterContextPOSIX_sw_64::~RegisterContextPOSIX_sw_64() {}

void RegisterContextPOSIX_sw_64::Invalidate() {}

void RegisterContextPOSIX_sw_64::InvalidateAllRegisters() {}

unsigned RegisterContextPOSIX_sw_64::GetRegisterOffset(unsigned reg) {
  assert(reg < m_num_registers && "Invalid register number.");
  return GetRegisterInfo()[reg].byte_offset;
}

unsigned RegisterContextPOSIX_sw_64::GetRegisterSize(unsigned reg) {
  assert(reg < m_num_registers && "Invalid register number.");
  return GetRegisterInfo()[reg].byte_size;
}

size_t RegisterContextPOSIX_sw_64::GetRegisterCount() {
#ifndef LHX20210708
printf("-->RegisterContextPOSIX_sw_64::GetRegisterCount() return %d\n",m_register_info_up->GetRegisterCount());
#endif
  return m_register_info_up->GetRegisterCount();
}

size_t RegisterContextPOSIX_sw_64::GetGPRSize() {
#ifndef LHX20210708
printf("-->RegisterContextPOSIX_sw_64::GetGPRSize() return %d\n",m_register_info_up->GetGPRSize());
#endif
  return m_register_info_up->GetGPRSize();
}

const RegisterInfo *RegisterContextPOSIX_sw_64::GetRegisterInfo() {
  // Commonly, this method is overridden and g_register_infos is copied and
  // specialized. So, use GetRegisterInfo() rather than g_register_infos in
  // this scope.
  return m_register_info_up->GetRegisterInfo();
}

const RegisterInfo *
RegisterContextPOSIX_sw_64::GetRegisterInfoAtIndex(size_t reg) {
  if (reg < m_num_registers)
    return &GetRegisterInfo()[reg];
  else
    return nullptr;
}

size_t RegisterContextPOSIX_sw_64::GetRegisterSetCount() {
  ArchSpec target_arch = m_register_info_up->GetTargetArchitecture();
  switch (target_arch.GetTriple().getOS()) {
  case llvm::Triple::Linux: {
    const auto *context = static_cast<const RegisterContextLinux_sw_64 *>(
        m_register_info_up.get());
    return context->GetRegisterSetCount();
  }
  default: {
    printf("[default case]: here maybe wrong.\n");
    //const auto *context = static_cast<const RegisterContextFreeBSD_mips64 *>(
    //    m_register_info_up.get());
    //return context->GetRegisterSetCount();
  }

  }
}

const RegisterSet *RegisterContextPOSIX_sw_64::GetRegisterSet(size_t set) {
  ArchSpec target_arch = m_register_info_up->GetTargetArchitecture();
  switch (target_arch.GetTriple().getOS()) {
  case llvm::Triple::Linux: {
    const auto *context = static_cast<const RegisterContextLinux_sw_64 *>(
        m_register_info_up.get());
    return context->GetRegisterSet(set);
  }
  default: {
    printf("[default case]: here maybe wrong.\n");
    //const auto *context = static_cast<const RegisterContextFreeBSD_mips64 *>(
    //    m_register_info_up.get());
    //return context->GetRegisterSet(set);
  }
  }
}

const char *RegisterContextPOSIX_sw_64::GetRegisterName(unsigned reg) {
  assert(reg < m_num_registers && "Invalid register offset.");
#ifndef LHX20210708
printf("-->GetRegisterInfo()[reg].name=%s\n",GetRegisterInfo()[reg].name);
#endif
  return GetRegisterInfo()[reg].name;
}

lldb::ByteOrder RegisterContextPOSIX_sw_64::GetByteOrder() {
  // Get the target process whose privileged thread was used for the register
  // read.
  lldb::ByteOrder byte_order = eByteOrderInvalid;
  Process *process = CalculateProcess().get();

  if (process)
    byte_order = process->GetByteOrder();
#ifndef LHX20210708
printf("-->RegisterContextPOSIX_sw_64::GetByteOrder=%s\n",byte_order);
#endif
  return byte_order;
}

bool RegisterContextPOSIX_sw_64::IsRegisterSetAvailable(size_t set_index) {
  size_t num_sets = GetRegisterSetCount();

  return (set_index < num_sets);
}

// Used when parsing DWARF and EH frame information and any other object file
// sections that contain register numbers in them.
uint32_t RegisterContextPOSIX_sw_64::ConvertRegisterKindToRegisterNumber(
    lldb::RegisterKind kind, uint32_t num) {
  const uint32_t num_regs = m_num_registers;

  assert(kind < kNumRegisterKinds);
  for (uint32_t reg_idx = 0; reg_idx < num_regs; ++reg_idx) {
    const RegisterInfo *reg_info = GetRegisterInfoAtIndex(reg_idx);

    if (reg_info->kinds[kind] == num)
      return reg_idx;
  }

  return LLDB_INVALID_REGNUM;
}
