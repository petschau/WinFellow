#ifndef SPRITEREGISTERS_H
#define SPRITEREGISTERS_H

#include "DEFS.H"

extern void wsprxpth(uint16_t data, uint32_t address);
extern void wsprxptl(uint16_t data, uint32_t address);
extern void wsprxpos(uint16_t data, uint32_t address);
extern void wsprxctl(uint16_t data, uint32_t address);
extern void wsprxdata(uint16_t data, uint32_t address);
extern void wsprxdatb(uint16_t data, uint32_t address);

class SpriteRegisters
{
public:
  uint32_t sprpt[8];
  uint16_t sprpos[8];
  uint16_t sprctl[8];
  uint16_t sprdata[8];
  uint16_t sprdatb[8];

  void InstallIOHandlers();

  void ClearState();
  void LoadState(FILE *F);
  void SaveState(FILE *F);
};

extern SpriteRegisters sprite_registers;

#endif
