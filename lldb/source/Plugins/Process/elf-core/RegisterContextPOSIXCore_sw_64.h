//===-- RegisterContextPOSIXCore_sw_64.h -----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_RegisterContextCorePOSIX_sw_64_h_
#define liblldb_RegisterContextCorePOSIX_sw_64_h_

#include "Plugins/Process/Utility/RegisterContextPOSIX_sw_64.h"
#include "Plugins/Process/elf-core/RegisterUtilities.h"
#include "lldb/Utility/DataBufferHeap.h"
#include "lldb/Utility/DataExtractor.h"
//LHX: this file mostly same to aarch64, mips64, ppc64, x86_64
class RegisterContextCorePOSIX_sw_64 : public RegisterContextPOSIX_sw_64 {
public:
  RegisterContextCorePOSIX_sw_64(
      lldb_private::Thread &thread,
      lldb_private::RegisterInfoInterface *register_info,
      const lldb_private::DataExtractor &gpregset,
      llvm::ArrayRef<lldb_private::CoreNote> notes);

  ~RegisterContextCorePOSIX_sw_64() override;

  bool ReadRegister(const lldb_private::RegisterInfo *reg_info,
                    lldb_private::RegisterValue &value) override;

  bool WriteRegister(const lldb_private::RegisterInfo *reg_info,
                     const lldb_private::RegisterValue &value) override;

  bool ReadAllRegisterValues(lldb::WritableDataBufferSP &data_sp) override;

  bool WriteAllRegisterValues(const lldb::DataBufferSP &data_sp) override;

  bool HardwareSingleStep(bool enable) override;

protected:
  bool ReadGPR() override;

  bool ReadFPR() override;

  bool WriteGPR() override;

  bool WriteFPR() override;

private:
  lldb::DataBufferSP m_gpr_buffer;
  lldb::DataBufferSP m_fpr_buffer;
  lldb_private::DataExtractor m_gpr;
  lldb_private::DataExtractor m_fpr;
};

#endif // liblldb_RegisterContextCorePOSIX_sw_64_h_
