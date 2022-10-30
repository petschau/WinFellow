#include "fellow/chipset/SpriteRegisters.h"
#include "fellow/chipset/Sprite.h"
#include "fellow/memory/Memory.h"
#include "fellow/chipset/ChipsetInfo.h"

SpriteRegisters sprite_registers;

/* SPRXPT - $dff120 to $dff13e */

constexpr unsigned int GetSpriteNumberForControlWords(ULO address)
{
  return (unsigned int)((address >> 3) & 7);
}

constexpr unsigned int GetSpriteNumberForPointer(ULO address)
{
  return (unsigned int)((address >> 2) & 7);
}

void wsprxpth(UWO data, ULO address)
{
  unsigned int spriteNumber = GetSpriteNumberForPointer(address);

  if (chipsetIsCycleExact())
  {
    sprite_registers.sprpt[spriteNumber] = chipsetReplaceHighPtr(sprite_registers.sprpt[spriteNumber], data);
  }

  sprites->NotifySprpthChanged(data, spriteNumber);
}

void wsprxptl(UWO data, ULO address)
{
  unsigned int spriteNumber = GetSpriteNumberForPointer(address);

  if (chipsetIsCycleExact())
  {
    sprite_registers.sprpt[spriteNumber] = chipsetReplaceLowPtr(sprite_registers.sprpt[spriteNumber], data);
  }

  sprites->NotifySprptlChanged(data, spriteNumber);
}

/* SPRXPOS - $dff140 to $dff178 */

void wsprxpos(UWO data, ULO address)
{
  unsigned int spriteNumber = GetSpriteNumberForControlWords(address);

  if (chipsetIsCycleExact())
  {
    sprite_registers.sprpos[spriteNumber] = data;
  }

  sprites->NotifySprposChanged(data, spriteNumber);
}

/* SPRXCTL $dff142 to $dff17a */

void wsprxctl(UWO data, ULO address)
{
  unsigned int spriteNumber = GetSpriteNumberForControlWords(address);

  if (chipsetIsCycleExact())
  {
    sprite_registers.sprctl[spriteNumber] = data;
  }

  sprites->NotifySprctlChanged(data, spriteNumber);
}

/* SPRXDATA $dff144 to $dff17c */

void wsprxdata(UWO data, ULO address)
{
  unsigned int spriteNumber = GetSpriteNumberForControlWords(address);

  if (chipsetIsCycleExact())
  {
    sprite_registers.sprdata[spriteNumber] = data;
  }

  sprites->NotifySprdataChanged(data, spriteNumber);
}

/* SPRXDATB $dff146 to $dff17e */

void wsprxdatb(UWO data, ULO address)
{
  unsigned int spriteNumber = GetSpriteNumberForControlWords(address);

  if (chipsetIsCycleExact())
  {
    sprite_registers.sprdatb[spriteNumber] = data;
  }

  sprites->NotifySprdatbChanged(data, spriteNumber);
}

void SpriteRegisters::InstallIOHandlers()
{
  for (unsigned int i = 0; i < 8; i++)
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
  for (unsigned int i = 0; i < 8; i++)
  {
    sprpt[i] = 0;
    sprpos[i] = 0;
    sprctl[i] = 0;
    sprdata[i] = 0;
    sprdatb[i] = 0;
  }
}
