//===-- Sw64.h ------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABISw64.h"
#include "ABISysV_sw_64.h"
#include "lldb/Core/PluginManager.h"

LLDB_PLUGIN_DEFINE(ABISw64)

void ABISw64::Initialize() {
  ABISysV_sw_64::Initialize();
}

void ABISw64::Terminate() {
  ABISysV_sw_64::Terminate();
}
