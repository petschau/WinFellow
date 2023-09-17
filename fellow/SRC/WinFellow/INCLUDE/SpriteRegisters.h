#ifndef SPRITEREGISTERS_H
#define SPRITEREGISTERS_H

#include "DEFS.H"

extern void wsprxpth(UWO data, uint32_t address);
extern void wsprxptl(UWO data, uint32_t address);
extern void wsprxpos(UWO data, uint32_t address);
extern void wsprxctl(UWO data, uint32_t address);
extern void wsprxdata(UWO data, uint32_t address);
extern void wsprxdatb(UWO data, uint32_t address);

class SpriteRegisters
{
public:
  uint32_t sprpt[8];
  UWO sprpos[8];
  UWO sprctl[8];
  UWO sprdata[8];
  UWO sprdatb[8];

  void InstallIOHandlers();

  void ClearState();
  void LoadState(FILE *F);
  void SaveState(FILE *F);
};

extern SpriteRegisters sprite_registers;

#endif
