#pragma once

typedef struct _chipset_information
{
  bool ecs;
  uint32_t ptr_mask;
  uint32_t address_mask;
} chipset_information;

extern chipset_information chipset;

#define chipsetMaskPtr(ptr) ((ptr) & chipset.ptr_mask)
#define chipsetReplaceLowPtr(original_ptr, low_part) chipsetMaskPtr(((original_ptr) & 0xffff0000) | (((uint32_t)low_part) & 0xfffe))
#define chipsetReplaceHighPtr(original_ptr, high_part) chipsetMaskPtr(((original_ptr) & 0xfffe) | (((uint32_t)high_part) << 16))

extern BOOLE chipsetSetECS(bool ecs);
extern bool chipsetGetECS(void);

extern void chipsetStartup(void);
