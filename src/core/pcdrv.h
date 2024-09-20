// SPDX-FileCopyrightText: 2023-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "cpu_types.h"

//////////////////////////////////////////////////////////////////////////
// HLE Implementation of PCDrv
//////////////////////////////////////////////////////////////////////////

namespace PCDrv {
void Initialize();
void Reset();
void Shutdown();

bool HandleSyscall(u32 instruction_bits, CPU::Registers& regs);
}
