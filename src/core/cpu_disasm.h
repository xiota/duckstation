// SPDX-FileCopyrightText: 2019-2023 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include "cpu_types.h"

class SmallStringBase;

namespace CPU {

void DisassembleInstruction(SmallStringBase* dest, u32 pc, u32 bits);
void DisassembleInstructionComment(SmallStringBase* dest, u32 pc, u32 bits);

const char* GetGTERegisterName(u32 index);

} // namespace CPU
