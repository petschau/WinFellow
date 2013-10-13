#ifndef CHIPSET_H
#define CHIPSET_H

typedef struct _chipset_information
{
  bool ecs;
  ULO ptr_mask;
  ULO address_mask;
} chipset_information;

extern chipset_information chipset;

#define chipsetMaskPtr(ptr) ((ptr) & chipset.ptr_mask)
#define chipsetReplaceLowPtr(original_ptr, low_part) chipsetMaskPtr(((original_ptr) & 0xffff0000) | (((ULO)low_part) & 0xfffe))
#define chipsetReplaceHighPtr(original_ptr, high_part) chipsetMaskPtr(((original_ptr) & 0xfffe) | (((ULO)high_part) << 16))

extern BOOLE chipsetSetECS(bool ecs);
extern bool chipsetGetECS(void);

extern void chipsetStartup(void);

#endif