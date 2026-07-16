//===-- ArchitectureSw64.h ---------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_SOURCE_PLUGINS_ARCHITECTURE_SW64_ARCHITECTURESW64_H
#define LLDB_SOURCE_PLUGINS_ARCHITECTURE_SW64_ARCHITECTURESW64_H

#include "lldb/Core/Architecture.h"
#include "lldb/Utility/ArchSpec.h"

namespace lldb_private {

class ArchitectureSw64 : public Architecture {
public:
  static llvm::StringRef GetPluginNameStatic() { return "sw_64"; }
  static void Initialize();
  static void Terminate();

  llvm::StringRef GetPluginName() override { return GetPluginNameStatic(); }

  void OverrideStopInfo(Thread &thread) const override {}

//  lldb::addr_t GetBreakableLoadAddress(lldb::addr_t addr,
//                                       Target &) const override;
//
//  lldb::addr_t GetCallableLoadAddress(lldb::addr_t load_addr,
//                                      AddressClass addr_class) const override;
//
//  lldb::addr_t GetOpcodeLoadAddress(lldb::addr_t load_addr,
//                                    AddressClass addr_class) const override;

//private:
//  Instruction *GetInstructionAtAddress(Target &target,
//                                       const Address &resolved_addr,
//                                       lldb::addr_t symbol_offset) const;
//
//  static std::unique_ptr<Architecture> Create(const ArchSpec &arch);
//  ArchitectureSw64(const ArchSpec &arch) : m_arch(arch) {}
//
//  ArchSpec m_arch;

private:
  static std::unique_ptr<Architecture> Create(const ArchSpec &arch);
  ArchitectureSw64() = default;
};

} // namespace lldb_private

#endif // LLDB_SOURCE_PLUGINS_ARCHITECTURE_SW64_ARCHITECTURESW64_H
