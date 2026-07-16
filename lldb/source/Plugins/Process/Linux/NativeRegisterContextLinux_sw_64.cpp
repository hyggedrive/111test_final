//===-- NativeRegisterContextLinux_sw_64.cpp ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#if defined(__sw_64__)

#include "NativeRegisterContextLinux_sw_64.h"


#include "Plugins/Process/Linux/NativeProcessLinux.h"
#include "Plugins/Process/Linux/Procfs.h"
#include "Plugins/Process/POSIX/ProcessPOSIXLog.h"
#include "Plugins/Process/Utility/RegisterContextLinux_sw_64.h"
#include "lldb/Core/EmulateInstruction.h"
#include "lldb/Host/Host.h"
#include "lldb/Host/HostInfo.h"
#include "lldb/Utility/DataBufferHeap.h"
#include "lldb/Utility/LLDBAssert.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Status.h"
#include "lldb/lldb-enumerations.h"
#include "lldb/lldb-private-enumerations.h"

#define NUM_REGISTERS 32

#include <asm/ptrace.h>
#include <sys/ptrace.h>

// refer to linux-stable-sw/arch/sw_64/include/uapi/asm/ptrace.h
#define DA_MATCH 163
#define DA_MASK  164
#define DV_MATCH 165
#define DV_MASK  166
#define DC_CTL   167
#define MATCH_CTL   167

using namespace lldb_private;
using namespace lldb_private::process_linux;

std::unique_ptr<NativeRegisterContextLinux>
NativeRegisterContextLinux::CreateHostNativeRegisterContextLinux(
    //const ArchSpec &target_arch, NativeThreadProtocol &native_thread) {
    const ArchSpec &target_arch, NativeThreadLinux &native_thread) {
  switch (target_arch.GetMachine()) {
  case llvm::Triple::sw_64:
    return std::make_unique<NativeRegisterContextLinux_sw_64>(target_arch,
                                                                 native_thread);
  default:
    llvm_unreachable("have no register context for architecture");
  }
}

llvm::Expected<ArchSpec>
NativeRegisterContextLinux::DetermineArchitecture(lldb::tid_t tid) {
  return HostInfo::GetArchitecture();
}

#define REG_CONTEXT_SIZE                                                       \
  (GetRegisterInfoInterface().GetGPRSize() + sizeof(FPR_linux_sw_64) +         \
  sizeof(SIMD_linux_sw_64))

// NativeRegisterContextLinux_sw_64 members.

static RegisterInfoInterface *
CreateRegisterInfoInterface(const ArchSpec &target_arch) {
  assert((HostInfo::GetArchitecture().GetAddressByteSize() == 8) &&
         "Register setting path assumes this is a 64-bit host");
  return new RegisterContextLinux_sw_64(target_arch);
//        target_arch, NativeRegisterContextLinux_sw_64::IsSIMDAvailable());
//        target_arch);
}

NativeRegisterContextLinux_sw_64::NativeRegisterContextLinux_sw_64(
    const ArchSpec &target_arch, NativeThreadProtocol &native_thread)
//    : NativeRegisterContextLinux(native_thread, CreateRegisterInfoInterface(target_arch)) {
    : NativeRegisterContextRegisterInfo(
          native_thread, CreateRegisterInfoInterface(target_arch)),
      NativeRegisterContextLinux(native_thread) {
  switch (target_arch.GetMachine()) {
  case llvm::Triple::sw_64:
    m_reg_info.num_registers = k_num_registers_sw_64;
    m_reg_info.num_gpr_registers = k_num_gpr_registers_sw_64;
    m_reg_info.num_fpr_registers = k_num_fpr_registers_sw_64;
    m_reg_info.last_gpr = k_last_gpr_sw_64;
    m_reg_info.first_fpr = k_first_fpr_sw_64;
    m_reg_info.last_fpr = k_last_fpr_sw_64;
    m_reg_info.first_simd = k_first_simd_sw_64;
    m_reg_info.last_simd = k_last_simd_sw_64;
    break;
  default:
    assert(false && "Unhandled target architecture.");
    break;
  }

  // Initialize m_iovec to point to the buffer and buffer size using the
  // conventions of Berkeley style UIO structures, as required by PTRACE
  // extensions.
  m_iovec.iov_base = &m_simd;
  m_iovec.iov_len = sizeof(SIMD_linux_sw_64);

  // init h/w watchpoint addr map
//  for (int index = 0; index <= MAX_NUM_WP; index++)
//    hw_addr_map[index] = LLDB_INVALID_ADDRESS;

  ::memset(&m_gpr, 0, sizeof(GPR_linux_sw_64));
  ::memset(&m_fpr, 0, sizeof(FPR_linux_sw_64));
  ::memset(&m_simd, 0, sizeof(SIMD_linux_sw_64));
  ::memset(&m_hwp_regs, 0, sizeof(m_hwp_regs));

  m_max_hwp_supported = MAX_NUM_WP;
  //m_refresh_hwdebug_info = true;
}

uint32_t NativeRegisterContextLinux_sw_64::GetRegisterSetCount() const {
  switch (GetRegisterInfoInterface().GetTargetArchitecture().GetMachine()) {
  case llvm::Triple::sw_64: {
    const auto context = static_cast<const RegisterContextLinux_sw_64 &>
                         (GetRegisterInfoInterface());
    return context.GetRegisterSetCount();
  }
  default:
    llvm_unreachable("Unhandled target architecture.");
  }
}

const RegisterSet *
NativeRegisterContextLinux_sw_64::GetRegisterSet(uint32_t set_index) const {
  if (set_index >= GetRegisterSetCount())
    return nullptr;

  switch (GetRegisterInfoInterface().GetTargetArchitecture().GetMachine()) {
  case llvm::Triple::sw_64: {
    const auto context = static_cast<const RegisterContextLinux_sw_64 &>
                          (GetRegisterInfoInterface());
    return context.GetRegisterSet(set_index);
  }
  default:
    llvm_unreachable("Unhandled target architecture.");
  }
}

lldb_private::Status
NativeRegisterContextLinux_sw_64::ReadRegister(const RegisterInfo *reg_info,
                                                RegisterValue &reg_value) {
  Status error;

  if (!reg_info) {
    error.SetErrorString("reg_info NULL");
    return error;
  }

  const uint32_t reg = reg_info->kinds[lldb::eRegisterKindLLDB];
  uint8_t byte_size = reg_info->byte_size;
  if (reg == LLDB_INVALID_REGNUM) {
    // This is likely an internal register for lldb use only and should not be
    // directly queried.
    error.SetErrorStringWithFormat("register \"%s\" is an internal-only lldb "
                                   "register, cannot read directly",
                                   reg_info->name);
    return error;
  }

//  if (IsSIMD(reg) /*&& !IsSIMDAvailable()*/) {
//    error.SetErrorString("SIMD not available on this processor");
//    printf("SIMD not available on this processor\n");
//    return error;
//  }
#ifndef LHX20210820 // refer to x86 and arm64
  if (IsSIMD(reg)) {
    error = ReadFPR();
    if (error.Fail())
      return error;
  } else {
    uint32_t full_reg = reg;
    bool is_subreg = reg_info->invalidate_regs &&
                     (reg_info->invalidate_regs[0] != LLDB_INVALID_REGNUM);

    if (is_subreg) {
      // Read the full aligned 64-bit register.
      full_reg = reg_info->invalidate_regs[0];
    }

    error = ReadRegisterRaw(full_reg, reg_value);

    if (error.Success()) {
      // If our read was not aligned (for ah,bh,ch,dh), shift our returned
      // value one byte to the right.
      if (is_subreg && (reg_info->byte_offset & 0x1))
        reg_value.SetUInt64(reg_value.GetAsUInt64() >> 8);

      // If our return byte size was greater than the return value reg size,
      // then use the type specified by reg_info rather than the uint64_t
      // default
      if (reg_value.GetByteSize() > reg_info->byte_size)
        reg_value.SetType(*reg_info);
    }
    return error;
  }
  // Get pointer to m_fpr variable and set the data from it.
  uint8_t *src = (uint8_t *)reg_info->byte_offset -
            (sizeof(m_gpr) + sizeof(m_fpr));
//  reg_value.SetFromMemoryData(reg_info, src, reg_info->byte_size,
//                            eByteOrderLittle, error);
  error = ReadRegisterRaw(reg, reg_value);
  return error;
}
#endif

lldb_private::Status NativeRegisterContextLinux_sw_64::WriteRegister(
    const RegisterInfo *reg_info, const RegisterValue &reg_value) {
  Status error;

  assert(reg_info && "reg_info is null");

  const uint32_t reg_index = reg_info->kinds[lldb::eRegisterKindLLDB];

  if (reg_index == LLDB_INVALID_REGNUM)
    return Status("no lldb regnum for %s", reg_info && reg_info->name
                                               ? reg_info->name
                                               : "<unknown register>");

  if (IsSIMD(reg_index)/* && !IsSIMDAvailable()*/) {
    error.SetErrorString("SIMD not available on this processor");
    //printf("SIMD not available on this processor\n");
    return error;
  }

  if (IsFPR(reg_index) || IsSIMD(reg_index)) {
    uint8_t *dst = nullptr;
    uint64_t *src = nullptr;
    uint8_t byte_size = reg_info->byte_size;
    lldbassert(reg_info->byte_offset < sizeof(UserArea_sw_64));

    // Initialise the FP and SIMD buffers by reading all co-processor 1
    // registers
    ReadCP1();

    if (IsFPR(reg_index)) {
      if (/*IsFR0() &&*/ (byte_size != 4)) {
        byte_size = 4;
        uint8_t ptrace_index;
        ptrace_index = reg_info->kinds[lldb::eRegisterKindProcessPlugin];
        dst = ReturnFPOffset(ptrace_index, reg_info->byte_offset);
      } else
        dst = (uint8_t *)&m_fpr + reg_info->byte_offset - sizeof(m_gpr);
    } else
      dst =(uint8_t *)&m_simd + reg_info->byte_offset -
            (sizeof(m_gpr) + sizeof(m_fpr));
    switch (byte_size) {
    case 4:
      *(uint32_t *)dst = reg_value.GetAsUInt32();
      break;
    case 8:
      *(uint64_t *)dst = reg_value.GetAsUInt64();
      break;
    case 16:
      src = (uint64_t *)reg_value.GetBytes();
      *(uint64_t *)dst = *src;
      *(uint64_t *)(dst + 8) = *(src + 1);
      break;
    default:
      assert(false && "Unhandled data size.");
      error.SetErrorStringWithFormat("unhandled byte size: %" PRIu32,
                                     reg_info->byte_size);
      break;
    }
     error = WriteCP1();
    if (!error.Success()) {
      error.SetErrorString("failed to write co-processor 1 register");
      return error;
    }
  } else {
    error = WriteRegisterRaw(reg_index, reg_value);
  }

    error = WriteRegisterRaw(reg_index, reg_value);
  return error;
}

Status NativeRegisterContextLinux_sw_64::ReadAllRegisterValues(
    lldb::WritableDataBufferSP &data_sp) {
  Status error;

  data_sp.reset(new DataBufferHeap(REG_CONTEXT_SIZE, 0));
  error = ReadGPR();
  if (!error.Success()) {
    error.SetErrorString("ReadGPR() failed");
    return error;
  }

  error = ReadCP1();
  if (!error.Success()) {
    error.SetErrorString("ReadCP1() failed");
    return error;
  }

  uint8_t *dst = data_sp->GetBytes();
  ::memcpy(dst, &m_gpr, GetRegisterInfoInterface().GetGPRSize());
  dst += GetRegisterInfoInterface().GetGPRSize();

  ::memcpy(dst, &m_fpr, GetFPRSize());
  dst += GetFPRSize();

  ::memcpy(dst, &m_simd, sizeof(SIMD_linux_sw_64));

  return error;
}

Status NativeRegisterContextLinux_sw_64::WriteAllRegisterValues(
    const lldb::DataBufferSP &data_sp) {
  Status error;

  if (!data_sp) {
    error.SetErrorStringWithFormat(
        "NativeRegisterContextLinux_sw_64::%s invalid data_sp provided",
        __FUNCTION__);
    return error;
  }

  if (data_sp->GetByteSize() != REG_CONTEXT_SIZE) {
    error.SetErrorStringWithFormat(
        "NativeRegisterContextLinux_sw_64::%s data_sp contained mismatched "
        "data size, expected %" PRIu64 ", actual %" PRIu64,
        __FUNCTION__, REG_CONTEXT_SIZE, data_sp->GetByteSize());
    return error;
  }

  const uint8_t *src = data_sp->GetBytes();
  if (src == nullptr) {
    error.SetErrorStringWithFormat("NativeRegisterContextLinux_sw_64::%s "
                                   "DataBuffer::GetBytes() returned a null "
                                   "pointer",
                                   __FUNCTION__);
    return error;
  }

  ::memcpy(&m_gpr, src, GetRegisterInfoInterface().GetGPRSize());
  src += GetRegisterInfoInterface().GetGPRSize();

  ::memcpy(&m_fpr, src, GetFPRSize());
  src += GetFPRSize();

  ::memcpy(&m_simd, src, sizeof(SIMD_linux_sw_64));

  error = WriteGPR();
  if (!error.Success()) {
    error.SetErrorStringWithFormat(
        "NativeRegisterContextLinux_sw_64::%s WriteGPR() failed",
        __FUNCTION__);
    return error;
  }

  error = WriteCP1();
  if (!error.Success()) {
    error.SetErrorStringWithFormat(
        "NativeRegisterContextLinux_sw_64::%s WriteCP1() failed",
        __FUNCTION__);
    return error;
  }

  return error;
}

Status NativeRegisterContextLinux_sw_64::ReadCP1() {
  Status error;

  uint8_t *src = nullptr;
  uint8_t *dst = nullptr;

  lldb::ByteOrder byte_order = GetByteOrder();

  bool IsBigEndian = (byte_order == lldb::eByteOrderBig);
/*
  if (IsSIMDAvailable()) {
    error = NativeRegisterContextLinux::ReadRegisterSet(
        &m_iovec, sizeof(SIMD_linux_sw_64), NFPREG);
    src = (uint8_t *)&m_simd + (IsBigEndian * 8);
    dst = (uint8_t *)&m_fpr;
    for (int i = 0; i < NUM_REGISTERS; i++) {
      // Copy fp values from simd buffer fetched via ptrace
      *(uint64_t *)dst = *(uint64_t *)src;
      src = src + 16;
      dst = dst + 8;
    }
//    m_fpr.fir = m_simd.fir;
//    m_fpr.fcsr = m_simd.fcsr;
//    m_fpr.config5 = m_simd.config5;
  } else {
    error = NativeRegisterContextLinux::ReadFPR();
  }
*/
    error = NativeRegisterContextLinux::ReadFPR();
  return error;
}

uint8_t *
NativeRegisterContextLinux_sw_64::ReturnFPOffset(uint8_t reg_index,
                                                  uint32_t byte_offset) {

  uint8_t *fp_buffer_ptr = nullptr;
  lldb::ByteOrder byte_order = GetByteOrder();
  bool IsBigEndian = (byte_order == lldb::eByteOrderBig);
  if (reg_index % 2) {
    uint8_t offset_diff = (IsBigEndian) ? 8 : 4;
    fp_buffer_ptr =
        (uint8_t *)&m_fpr + byte_offset - offset_diff - sizeof(m_gpr);
  } else {
    fp_buffer_ptr =
        (uint8_t *)&m_fpr + byte_offset + 4 * (IsBigEndian) - sizeof(m_gpr);
  }
  return fp_buffer_ptr;
}

Status NativeRegisterContextLinux_sw_64::WriteCP1() {
  Status error;

  uint8_t *src = nullptr;
  uint8_t *dst = nullptr;

  lldb::ByteOrder byte_order = GetByteOrder();

  bool IsBigEndian = (byte_order == lldb::eByteOrderBig);
/*
  if (IsSIMDAvailable()) {
    dst = (uint8_t *)&m_simd + (IsBigEndian * 8);
    src = (uint8_t *)&m_fpr;
    for (int i = 0; i < NUM_REGISTERS; i++) {
      // Copy fp values to simd buffer for ptrace
      *(uint64_t *)dst = *(uint64_t *)src;
      dst = dst + 16;
      src = src + 8;
    }
//    m_simd.fir = m_fpr.fir;
//    m_simd.fcsr = m_fpr.fcsr;
//    m_simd.config5 = m_fpr.config5;
    error = NativeRegisterContextLinux::WriteRegisterSet(
        &m_iovec, sizeof(SIMD_linux_sw_64), NFPREG);
  } else {
    error = NativeRegisterContextLinux::WriteFPR();
  }
*/
    error = NativeRegisterContextLinux::WriteFPR();

  return error;
}

bool NativeRegisterContextLinux_sw_64::IsFPR(uint32_t reg_index) const {
  return (m_reg_info.first_fpr <= reg_index &&
          reg_index <= m_reg_info.last_fpr);
}

bool NativeRegisterContextLinux_sw_64::IsSIMD(uint32_t reg_index) const {
  return (m_reg_info.first_simd <= reg_index &&
          reg_index <= m_reg_info.last_simd);
}

Status NativeRegisterContextLinux_sw_64::GetWatchpointHitIndex(
    uint32_t &wp_index, lldb::addr_t trap_addr) {
  // refer to arm64
  Log *log = GetLog(POSIXLog::Watchpoints);
  //Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_WATCHPOINTS));
  LLDB_LOG(log, "wp_index: {0}, trap_addr: {1:x}", wp_index, trap_addr);

  uint32_t watch_size;
  lldb::addr_t watch_addr;

  for (wp_index = 0; wp_index < m_max_hwp_supported; ++wp_index) {
    watch_size = GetWatchpointSize(wp_index);
    watch_addr = m_hwp_regs[wp_index].address;

    if (WatchpointIsEnabled(wp_index) && trap_addr >= watch_addr &&
        trap_addr < watch_addr + watch_size) {
      m_hwp_regs[wp_index].hit_addr = trap_addr;
      return Status();
    }
  }

  wp_index = LLDB_INVALID_INDEX32;
  return Status();
}

Status NativeRegisterContextLinux_sw_64::IsWatchpointVacant(uint32_t wp_index,
                                                             bool &is_vacant) {
  is_vacant = false;
  return Status("SW_64 TODO: "
                "NativeRegisterContextLinux_sw_64::IsWatchpointVacant not "
                "implemented");
}

bool NativeRegisterContextLinux_sw_64::ClearHardwareWatchpoint(
    uint32_t wp_index) {
  // refer to arm64
  Log *log = GetLog(POSIXLog::Watchpoints);
  //Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_WATCHPOINTS));
  LLDB_LOG(log, "wp_index: {0}", wp_index);

  // Read hardware breakpoint and watchpoint information.
  //Status error = ReadHardwareDebugInfo();

//  if (error.Fail())
//    return false;

  if (wp_index >= m_max_hwp_supported)
    return false;

  // Create a backup we can revert to in case of failure.
  lldb::addr_t tempAddr = m_hwp_regs[wp_index].address;
  //lldb::addr_t tempAddr = m_hwp_regs[wp_index].address & ((1L<<53)-1);
  uint32_t tempControl = m_hwp_regs[wp_index].control;
  //uint32_t tempControl = (m_hwp_regs[wp_index].address >> 53) & 0x3L;

  // Update watchpoint in local cache
  m_hwp_regs[wp_index].control &= ~1;
  //m_hwp_regs[wp_index].control &= ~3;
  //m_hwp_regs[wp_index].address &= ~(0x3L<<53);
  m_hwp_regs[wp_index].address = 0;

  // Ptrace call to update hardware debug registers
  Status error = WriteDebugRegisterValue(m_thread.GetID(), DA_MATCH, 0L);
  Status error2 = WriteDebugRegisterValue(m_thread.GetID(), DA_MASK, 0L);

  if (error.Fail() & error2.Fail()) {
    m_hwp_regs[wp_index].control = tempControl;
    m_hwp_regs[wp_index].address = tempAddr;

    return false;
  }

  return true;
}

Status NativeRegisterContextLinux_sw_64::ClearAllHardwareWatchpoints() {
  //printf("--> %s, %d\n", __FUNCTION__, __LINE__);

  lldb::addr_t tempAddr = 0;
  uint32_t tempControl = 0;

  for (uint32_t i = 0; i < m_max_hwp_supported; i++) {
    if (m_hwp_regs[i].control & 0x01) {
    //if (m_hwp_regs[i].control & 0x03) {
      // Create a backup we can revert to in case of failure.
      tempAddr = m_hwp_regs[i].address;
      //tempAddr = m_hwp_regs[i].address & ((1L<<53)-1);
      tempControl = m_hwp_regs[i].control;
      //tempControl = (m_hwp_regs[i].address >> 53) & 0x3L;

      // Clear watchpoints in local cache
      m_hwp_regs[i].control &= ~1;
      //m_hwp_regs[i].control &= ~3;
      m_hwp_regs[i].address = 0;

      // Ptrace call to update hardware debug registers
      Status error = WriteDebugRegisterValue(m_thread.GetID(), DA_MATCH, 0L);
      error = WriteDebugRegisterValue(m_thread.GetID(), DA_MASK, 0L);

      if (error.Fail()) {
        m_hwp_regs[i].control = tempControl;
        m_hwp_regs[i].address = tempAddr;

        return error;
      }
    }
  }

  return Status();
}

Status NativeRegisterContextLinux_sw_64::SetHardwareWatchpointWithIndex(
    lldb::addr_t addr, size_t size, uint32_t watch_flags, uint32_t wp_index) {
  //printf("--> %s, %d\n", __FUNCTION__, __LINE__);
  Status error;
  error.SetErrorString("SW_64 TODO: "
                       "NativeRegisterContextLinux_sw_64::"
                       "SetHardwareWatchpointWithIndex not implemented");
  return error;
}

uint32_t NativeRegisterContextLinux_sw_64::SetHardwareWatchpoint(
    lldb::addr_t addr, size_t size, uint32_t watch_flags) {
  // refer to arm64
  Log *log = GetLog(POSIXLog::Watchpoints);
  //Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_WATCHPOINTS));
  LLDB_LOG(log, "addr: {0:x}, size: {1:x} watch_flags: {2:x}", addr, size,
           watch_flags);

  // Read hardware breakpoint and watchpoint information.
  // sw_64 not need, aarch64 only read value of m_max_hwp_supported
  // First reading the current state of watch regs
  //Status error = ReadHardwareDebugInfo();

//  if (error.Fail())
//    return LLDB_INVALID_INDEX32;

  uint32_t control_value = 0, wp_index = 0;
  lldb::addr_t real_addr = addr;

  // Check if we are setting watchpoint other than read/write/access Also
  // update watchpoint flag to match AArch64 write-read bit configuration.
  switch (watch_flags) {
  case 1:
    watch_flags = 2;
    break;
  case 2:
    watch_flags = 1;
    break;
  case 3:
    break;
  default:
    return LLDB_INVALID_INDEX32;
  }

  // Check if size has a valid hardware watchpoint length.
  if (size != 1 && size != 2 && size != 4 && size != 8)
    return LLDB_INVALID_INDEX32;

  // Check 8-byte alignment for hardware watchpoint target address. Below is a
  // hack to recalculate address and size in order to make sure we can watch
  // non 8-byte alligned addresses as well.
  if (addr & 0x07) {
    uint8_t watch_mask = (addr & 0x07) + size;

    if (watch_mask > 0x08)
      return LLDB_INVALID_INDEX32;
    else if (watch_mask <= 0x02)
      size = 2;
    else if (watch_mask <= 0x04)
      size = 4;
    else
      size = 8;

    //addr = addr & (~0x07);
    //addr = addr & (~0x03);
  }

  // Setup control value
  control_value = watch_flags << 3;
  //control_value = watch_flags;
  control_value |= ((1 << size) - 1) << 5;
  control_value |= (2 << 1) | 1;

  // Iterate over stored watchpoints and find a free wp_index
  wp_index = LLDB_INVALID_INDEX32;
  for (uint32_t i = 0; i < m_max_hwp_supported; i++) {
    if ((m_hwp_regs[i].control & 1) == 0) {
    //if ((m_hwp_regs[i].control & 3) == 0) {
      wp_index = i; // Mark last free slot
    } else if (m_hwp_regs[i].address == addr) {
      return LLDB_INVALID_INDEX32; // We do not support duplicate watchpoints.
    }
  }

  if (wp_index == LLDB_INVALID_INDEX32)
    return LLDB_INVALID_INDEX32;

  // Update watchpoint in local cache
  m_hwp_regs[wp_index].real_addr = real_addr;
  m_hwp_regs[wp_index].address = addr;
  m_hwp_regs[wp_index].control = control_value;

  // PTRACE call to set corresponding watchpoint register.
  //error = WriteHardwareDebugRegs(eDREGTypeWATCH);

  Status error = WriteDebugRegisterValue(m_thread.GetID(), DA_MATCH, (addr|(2UL<<53)));
  //WriteDebugRegisterValue(m_thread.GetID(), DA_MATCH, (addr|(watch_flags<<53)));
  Status error2 = WriteDebugRegisterValue(m_thread.GetID(), DA_MASK, ((1UL<<53)-1));
#ifdef __sw_64_sw8a__
  Status error3 = WriteDebugRegisterValue(m_thread.GetID(), MATCH_CTL, 0x201);

  // TODO: modify control_value to support read, write, read/write watchpoint
  //Status error = WriteDebugRegisterValue(m_thread.GetID(), MATCH_CTL, 0x101); // read, DA_MATCH
  //Status error = WriteDebugRegisterValue(m_thread.GetID(), MATCH_CTL, 0x201); // write, DA_MATCH
  //Status error = WriteDebugRegisterValue(m_thread.GetID(), MATCH_CTL, 0x301); // read & write, DA_MATCH

  if (error.Fail() & error2.Fail() & error3.Fail()) {
#else
  if (error.Fail() & error2.Fail()) {
#endif
    m_hwp_regs[wp_index].address = 0;
    m_hwp_regs[wp_index].control &= ~1;
    //m_hwp_regs[wp_index].control &= ~3;

    return LLDB_INVALID_INDEX32;
  }

  return wp_index;
}

// refer to arm64
uint32_t
NativeRegisterContextLinux_sw_64::GetWatchpointSize(uint32_t wp_index) {
  Log *log = GetLog(POSIXLog::Watchpoints);
  //Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_WATCHPOINTS));
  LLDB_LOG(log, "wp_index: {0}", wp_index);

  switch ((m_hwp_regs[wp_index].control >> 5) & 0xff) {
  case 0x01:
    return 1;
  case 0x03:
    return 2;
  case 0x0f:
    return 4;
  case 0xff:
    return 8;
  default:
    return 0;
  }
}

bool NativeRegisterContextLinux_sw_64::WatchpointIsEnabled(uint32_t wp_index) {
  Log *log = GetLog(POSIXLog::Watchpoints);
  //Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_WATCHPOINTS));
  LLDB_LOG(log, "wp_index: {0}", wp_index);

  if ((m_hwp_regs[wp_index].control & 0x1) == 0x1)
  //if (((m_hwp_regs[wp_index].address >> 53) & 0x3L) != 0x0)
    return true;
  else
    return false;
}

lldb::addr_t
NativeRegisterContextLinux_sw_64::GetWatchpointAddress(uint32_t wp_index) {
  // refer to arm64
  Log *log = GetLog(POSIXLog::Watchpoints);
  //Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_WATCHPOINTS));
  LLDB_LOG(log, "wp_index: {0}", wp_index);

  if (wp_index >= m_max_hwp_supported)
    return LLDB_INVALID_ADDRESS;

  if (WatchpointIsEnabled(wp_index))
    return m_hwp_regs[wp_index].real_addr;
  else
    return LLDB_INVALID_ADDRESS;
}

lldb::addr_t
NativeRegisterContextLinux_sw_64::GetWatchpointHitAddress(uint32_t wp_index) {
  // refer to arm64
  Log *log = GetLog(POSIXLog::Watchpoints);
  //Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_WATCHPOINTS));
  LLDB_LOG(log, "wp_index: {0}", wp_index);

  if (wp_index >= m_max_hwp_supported)
    return LLDB_INVALID_ADDRESS;

  if (WatchpointIsEnabled(wp_index))
    return m_hwp_regs[wp_index].hit_addr;
  else
    return LLDB_INVALID_ADDRESS;
}

uint32_t NativeRegisterContextLinux_sw_64::NumSupportedHardwareWatchpoints() {
  printf("--> %s, %d\n", __FUNCTION__, __LINE__);
  return MAX_NUM_WP;
}

Status
NativeRegisterContextLinux_sw_64::ReadRegisterRaw(uint32_t reg_index,
                                                   RegisterValue &value) {
  const RegisterInfo *const reg_info = GetRegisterInfoAtIndex(reg_index);

  if (!reg_info)
    return Status("register %" PRIu32 " not found", reg_index);

  uint32_t offset = reg_info->kinds[lldb::eRegisterKindProcessPlugin];
  return DoReadRegisterValue(offset, reg_info->name, reg_info->byte_size,
                             value);
}

Status NativeRegisterContextLinux_sw_64::WriteRegisterRaw(
    uint32_t reg_index, const RegisterValue &value) {
  const RegisterInfo *const reg_info = GetRegisterInfoAtIndex(reg_index);

  if (!reg_info)
    return Status("register %" PRIu32 " not found", reg_index);

  if (reg_info->invalidate_regs)
    lldbassert(false && "reg_info->invalidate_regs is unhandled");

  uint32_t offset = reg_info->kinds[lldb::eRegisterKindProcessPlugin];
  return DoWriteRegisterValue(offset, reg_info->name, value);
}

Status NativeRegisterContextLinux_sw_64::ReadDebugRegisterValue(
    lldb::tid_t tid, int regnum, uint64_t *value) {
  Log *log = GetLog(POSIXLog::Watchpoints);
  //Log *log(GetLogIfAllCategoriesSet(LIBLLDB_LOG_WATCHPOINTS));
  Status error;
  error = NativeProcessLinux::PtraceWrapper(PTRACE_PEEKUSER, m_thread.GetID(),
            reinterpret_cast<void *>(regnum), nullptr, 0, reinterpret_cast<int64_t *>(value));
  if (log)
    log->Printf("%s: pid = %d, regnum = %d *value = %d\n", __FUNCTION__, tid, regnum, *value);
  return error;
}

Status NativeRegisterContextLinux_sw_64::WriteDebugRegisterValue(
    lldb::tid_t tid, int regnum, uint64_t value) {
  Log *log = GetLog(POSIXLog::Watchpoints);
  //Log *log(GetLogIfAllCategoriesSet(LIBLLDB_LOG_WATCHPOINTS));
  Status error;
  error = NativeProcessLinux::PtraceWrapper(PTRACE_POKEUSER, m_thread.GetID(),
            reinterpret_cast<void *>(regnum), reinterpret_cast<void *>(value));
  if (log)
    log->Printf("%s: pid = %d, regnum = %d value = %d\n", __FUNCTION__, tid, regnum, value);
  return error;
}


#endif // defined (__sw_64__)
