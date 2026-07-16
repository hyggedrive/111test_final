//===-- NativeRegisterContextLinux_sw_64.h ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#if defined(__sw_64__)

#ifndef lldb_NativeRegisterContextLinux_sw_64_h
#define lldb_NativeRegisterContextLinux_sw_64_h

#include "Plugins/Process/Linux/NativeRegisterContextLinux.h"
#include "Plugins/Process/Utility/RegisterContext_sw_64.h"
#include "Plugins/Process/Utility/lldb-sw_64-linux-register-enums.h"
#include <sys/uio.h> // For struct iovec

// LHX: Here only used a pair of debug register: DA_MATCH and DA_MASK,
// MAX_NUM_WP set to 1 will cause watchpoint failed.
#define MAX_NUM_WP 2

namespace lldb_private {
namespace process_linux {

class NativeProcessLinux;

class NativeRegisterContextLinux_sw_64 : public NativeRegisterContextLinux {
public:
  NativeRegisterContextLinux_sw_64(const ArchSpec &target_arch,
                                    NativeThreadProtocol &native_thread);

  uint32_t GetRegisterSetCount() const override;

  const RegisterSet *GetRegisterSet(uint32_t set_index) const override;

  Status ReadRegister(const RegisterInfo *reg_info,
                      RegisterValue &reg_value) override;

  Status WriteRegister(const RegisterInfo *reg_info,
                       const RegisterValue &reg_value) override;

  Status ReadAllRegisterValues(lldb::WritableDataBufferSP &data_sp) override;

  Status WriteAllRegisterValues(const lldb::DataBufferSP &data_sp) override;

  Status ReadCP1();

  Status WriteCP1();

  uint8_t *ReturnFPOffset(uint8_t reg_index, uint32_t byte_offset);

// Hardware breakpoints/watchpoint management functions

  uint32_t NumSupportedHardwareWatchpoints() override;

  uint32_t SetHardwareWatchpoint(lldb::addr_t addr, size_t size,
                                 uint32_t watch_flags) override;

  bool ClearHardwareWatchpoint(uint32_t wp_index) override;

  Status ClearAllHardwareWatchpoints() override;

  Status GetWatchpointHitIndex(uint32_t &wp_index,
                               lldb::addr_t trap_addr) override;

  lldb::addr_t GetWatchpointHitAddress(uint32_t wp_index) override;

  lldb::addr_t GetWatchpointAddress(uint32_t wp_index) override;

  uint32_t GetWatchpointSize(uint32_t wp_index);

  bool WatchpointIsEnabled(uint32_t wp_index);

  Status SetHardwareWatchpointWithIndex(lldb::addr_t addr, size_t size,
                                        uint32_t watch_flags,
                                        uint32_t wp_index);

  Status IsWatchpointVacant(uint32_t wp_index, bool &is_vacant) override;

protected:
  Status ReadRegisterRaw(uint32_t reg_index, RegisterValue &value) override;

  Status WriteRegisterRaw(uint32_t reg_index,
                          const RegisterValue &value) override;

  Status ReadDebugRegisterValue(lldb::tid_t tid, int regnum, uint64_t *value);

  Status WriteDebugRegisterValue(lldb::tid_t tid, int regnum, uint64_t value);

  bool IsFPR(uint32_t reg_index) const;

  bool IsSIMD(uint32_t reg_index) const;

  void *GetGPRBuffer() override { return &m_gpr; }

  void *GetFPRBuffer() override { return &m_fpr; }

  size_t GetFPRSize() override { return sizeof(m_fpr); }

private:
  // Info about register ranges.
  struct RegInfo {
    uint32_t num_registers;
    uint32_t num_gpr_registers;
    uint32_t num_fpr_registers;

    uint32_t last_gpr;
    uint32_t first_fpr;
    uint32_t last_fpr;
    uint32_t first_simd;
    uint32_t last_simd;
  };

  RegInfo m_reg_info;

  GPR_linux_sw_64 m_gpr;

  FPR_linux_sw_64 m_fpr;

  SIMD_linux_sw_64 m_simd;

  lldb::addr_t hw_addr_map[MAX_NUM_WP];

  struct iovec m_iovec;

  // Debug register info for hardware watchpoints management.
  struct DREG {
    lldb::addr_t address;  // Watchpoint address value.
    lldb::addr_t hit_addr; // Address at which last watchpoint trigger exception
                           // occurred.
    lldb::addr_t real_addr; // Address value that should cause target to stop.
    uint32_t control;       // Watchpoint control value. DA_MATCH[54:53]
    //lldb::addr_t match;  // DA_MATCH
    //lldb::addr_t mask;   // DA_MASK
    //lldb::addr_t dc_ctl; // DC_CTL[20:19]: dv_match=[0:1], dav_match=[1:1]
  };

  struct DREG m_hwp_regs[MAX_NUM_WP]; // Native linux hardware watchpoints
  uint32_t m_max_hwp_supported;
  //bool m_refresh_hwdebug_info;
};

} // namespace process_linux
} // namespace lldb_private

#endif // #ifndef lldb_NativeRegisterContextLinux_sw_64_h

#endif // defined (__sw_64__)
