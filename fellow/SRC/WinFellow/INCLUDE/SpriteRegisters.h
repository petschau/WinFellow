#ifndef SPRITEREGISTERS_H
#define SPRITEREGISTERS_H

#include "DEFS.H"

extern void wsprxpth(UWO data, ULO address);
extern void wsprxptl(UWO data, ULO address);
extern void wsprxpos(UWO data, ULO address);
extern void wsprxctl(UWO data, ULO address);
extern void wsprxdata(UWO data, ULO address);
extern void wsprxdatb(UWO data, ULO address);

class SpriteRegisters
{
public:
  ULO sprpt[8];
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
