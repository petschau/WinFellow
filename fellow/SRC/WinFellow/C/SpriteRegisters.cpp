#include "SpriteRegisters.h"
#include "Sprites.h"
#include "MemoryInterface.h"
#include "chipset.h"
#include "Renderer.h"

SpriteRegisters sprite_registers;

/* SPRXPT - $dff120 to $dff13e */

static unsigned int GetSpriteNumberForControlWords(uint32_t address)
{
  return (unsigned int)((address >> 3) & 7);
}

static unsigned int GetSpriteNumberForPointer(uint32_t address)
{
  return (unsigned int)((address >> 2) & 7);
}

void wsprxpth(uint16_t data, uint32_t address)
{
  unsigned int sprite_number = GetSpriteNumberForPointer(address);

  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    sprite_registers.sprpt[sprite_number] = chipsetReplaceHighPtr(sprite_registers.sprpt[sprite_number], data);
  }
  sprites->NotifySprpthChanged(data, sprite_number);
}

void wsprxptl(uint16_t data, uint32_t address)
{
  unsigned int sprite_number = GetSpriteNumberForPointer(address);
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    sprite_registers.sprpt[sprite_number] = chipsetReplaceLowPtr(sprite_registers.sprpt[sprite_number], data);
  }
  sprites->NotifySprptlChanged(data, sprite_number);
}

/* SPRXPOS - $dff140 to $dff178 */

void wsprxpos(uint16_t data, uint32_t address)
{
  unsigned int sprite_number = GetSpriteNumberForControlWords(address);
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    sprite_registers.sprpos[sprite_number] = data;
  }
  sprites->NotifySprposChanged(data, sprite_number);
}

/* SPRXCTL $dff142 to $dff17a */

void wsprxctl(uint16_t data, uint32_t address)
{
  unsigned int sprite_number = GetSpriteNumberForControlWords(address);
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    sprite_registers.sprctl[sprite_number] = data;
  }
  sprites->NotifySprctlChanged(data, sprite_number);
}

/* SPRXDATA $dff144 to $dff17c */

void wsprxdata(uint16_t data, uint32_t address)
{
  unsigned int sprite_number = GetSpriteNumberForControlWords(address);
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    sprite_registers.sprdata[sprite_number] = data;
  }
  sprites->NotifySprdataChanged(data, sprite_number);
}

/* SPRXDATB $dff146 to $dff17e */

void wsprxdatb(uint16_t data, uint32_t address)
{
  unsigned int sprite_number = GetSpriteNumberForControlWords(address);
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    sprite_registers.sprdatb[sprite_number] = data;
  }
  sprites->NotifySprdatbChanged(data, sprite_number);
}

void SpriteRegisters::InstallIOHandlers()
{
  for (int i = 0; i < 8; i++)
  {
    memorySetIoWriteStub(0x120 + i * 4, wsprxpth);
    memorySetIoWriteStub(0x122 + i * 4, wsprxptl);
    memorySetIoWriteStub(0x140 + i * 8, wsprxpos);
    memorySetIoWriteStub(0x142 + i * 8, wsprxctl);
    memorySetIoWriteStub(0x144 + i * 8, wsprxdata);
    memorySetIoWriteStub(0x146 + i * 8, wsprxdatb);
  }
}

void SpriteRegisters::ClearState()
{
  for (int i = 0; i < 8; i++)
  {
    sprpt[i] = 0;
    sprpos[i] = 0;
    sprctl[i] = 0;
    sprdata[i] = 0;
    sprdatb[i] = 0;
  }
}

void SpriteRegisters::LoadState(FILE *F)
{
  for (int i = 0; i < 8; i++)
  {
    fread(&sprpt[i], sizeof(sprpt[i]), 1, F);
    fread(&sprpos[i], sizeof(sprpos[i]), 1, F);
    fread(&sprctl[i], sizeof(sprctl[i]), 1, F);
    fread(&sprdata[i], sizeof(sprdata[i]), 1, F);
    fread(&sprdatb[i], sizeof(sprdatb[i]), 1, F);
  }
}

void SpriteRegisters::SaveState(FILE *F)
{
  for (int i = 0; i < 8; i++)
  {
    fwrite(&sprpt[i], sizeof(sprpt[i]), 1, F);
    fwrite(&sprpos[i], sizeof(sprpos[i]), 1, F);
    fwrite(&sprctl[i], sizeof(sprctl[i]), 1, F);
    fwrite(&sprdata[i], sizeof(sprdata[i]), 1, F);
    fwrite(&sprdatb[i], sizeof(sprdatb[i]), 1, F);
  }
}
