#pragma once

#include "fellow/api/defs.h"
#include <cstdint>

struct chipset_information
{
  bool ecs;
  uint32_t ptr_mask;
  uint32_t address_mask;
  bool IsCycleExact;
  uint16_t MaximumDDF;
  uint16_t DDFMask;

  bool GfxDebugImmediateRenderingFromConfig = false;
  bool GfxDebugImmediateRendering = false;
  bool GfxDebugDisableShifter = false;
  bool GfxDebugDisableRenderer = false;
  bool GfxDebugDisableCopier = false;
};

extern chipset_information chipset_info;

#define chipsetMaskAddress(ptr) ((ptr)&chipset_info.address_mask)
#define chipsetMaskPtr(ptr) ((ptr)&chipset_info.ptr_mask)
#define chipsetReplaceLowPtr(original_ptr, low_part) chipsetMaskPtr(((original_ptr)&0xffff0000) | (((uint32_t)(low_part)) & 0xfffe))
#define chipsetReplaceHighPtr(original_ptr, high_part) chipsetMaskPtr(((original_ptr)&0xfffe) | (((uint32_t)(high_part)) << 16))

extern bool chipsetSetECS(bool ecs);
extern bool chipsetGetECS();
extern bool chipsetSetIsCycleExact(bool isCycleExact);
extern bool chipsetIsCycleExact();
extern uint16_t chipsetGetMaximumDDF();
extern uint16_t chipsetGetDDFMask();

extern void chipsetStartup();
