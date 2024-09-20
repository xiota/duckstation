// SPDX-FileCopyrightText: 2019-2023 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define VULKAN_MODULE_ENTRY_POINT(name, required) extern PFN_##name name;
#define VULKAN_INSTANCE_ENTRY_POINT(name, required) extern PFN_##name name;
#define VULKAN_DEVICE_ENTRY_POINT(name, required) extern PFN_##name name;
#include "vulkan_entry_points.inl"
#undef VULKAN_DEVICE_ENTRY_POINT
#undef VULKAN_INSTANCE_ENTRY_POINT
#undef VULKAN_MODULE_ENTRY_POINT

#ifdef __cplusplus
}
#endif
