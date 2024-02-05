#include "SpriteRegisters.h"
#include "Memory/IMemory.h"
#include "chipset.h"
#include "VirtualHost/Core.h"

unsigned int SpriteRegisters::GetSpriteNumberForControlWords(uint32_t address)
{
  return (unsigned int)((address >> 3) & 7);
}

unsigned int SpriteRegisters::GetSpriteNumberForPointer(uint32_t address)
{
  return (unsigned int)((address >> 2) & 7);
}

// SPRXPT - $dff120 to $dff13e

void SpriteRegisters::wsprxpth(uint16_t data, uint32_t address)
{
  const unsigned int spriteNumber = GetSpriteNumberForPointer(address);
  _core.SpriteRegisters->SetPth(spriteNumber, data);
}

void SpriteRegisters::wsprxptl(uint16_t data, uint32_t address)
{
  const unsigned int spriteNumber = GetSpriteNumberForPointer(address);
  _core.SpriteRegisters->SetPtl(spriteNumber, data);
}

// SPRXPOS - $dff140 to $dff178

void SpriteRegisters::wsprxpos(uint16_t data, uint32_t address)
{
  const unsigned int spriteNumber = GetSpriteNumberForControlWords(address);
  _core.SpriteRegisters->SetPos(spriteNumber, data);
}

// SPRXCTL $dff142 to $dff17a

void SpriteRegisters::wsprxctl(uint16_t data, uint32_t address)
{
  const unsigned int spriteNumber = GetSpriteNumberForControlWords(address);
  _core.SpriteRegisters->SetCtl(spriteNumber, data);
}

// SPRXDATA $dff144 to $dff17c

void SpriteRegisters::wsprxdata(uint16_t data, uint32_t address)
{
  const unsigned int spriteNumber = GetSpriteNumberForControlWords(address);
  _core.SpriteRegisters->SetDatA(spriteNumber, data);
}

// SPRXDATB $dff146 to $dff17e

void SpriteRegisters::wsprxdatb(uint16_t data, uint32_t address)
{
  const unsigned int spriteNumber = GetSpriteNumberForControlWords(address);
  _core.SpriteRegisters->SetDatB(spriteNumber, data);
}

// ------------------------------------------

void SpriteRegisters::SetPth(unsigned int spriteNumber, uint16_t data)
{
  if (_isCycleBasedEmulation)
  {
    sprpt[spriteNumber] = chipsetReplaceHighPtr(sprpt[spriteNumber], data);
  }
  _core.CurrentSprites->NotifySprpthChanged(data, spriteNumber);
}

void SpriteRegisters::SetPtl(unsigned int spriteNumber, uint16_t data)
{
  if (_isCycleBasedEmulation)
  {
    sprpt[spriteNumber] = chipsetReplaceLowPtr(sprpt[spriteNumber], data);
  }
  _core.CurrentSprites->NotifySprptlChanged(data, spriteNumber);
}

void SpriteRegisters::SetPos(unsigned int spriteNumber, uint16_t data)
{
  if (_isCycleBasedEmulation)
  {
    sprpos[spriteNumber] = data;
  }
  _core.CurrentSprites->NotifySprposChanged(data, spriteNumber);
}

void SpriteRegisters::SetCtl(unsigned int spriteNumber, uint16_t data)
{
  if (_isCycleBasedEmulation)
  {
    sprctl[spriteNumber] = data;
  }
  _core.CurrentSprites->NotifySprctlChanged(data, spriteNumber);
}

void SpriteRegisters::SetDatA(unsigned int spriteNumber, uint16_t data)
{
  if (_isCycleBasedEmulation)
  {
    sprdata[spriteNumber] = data;
  }
  _core.CurrentSprites->NotifySprdataChanged(data, spriteNumber);
}

void SpriteRegisters::SetDatB(unsigned int spriteNumber, uint16_t data)
{
  if (_isCycleBasedEmulation)
  {
    sprdatb[spriteNumber] = data;
  }
  _core.CurrentSprites->NotifySprdatbChanged(data, spriteNumber);
}

void SpriteRegisters::InstallIOHandlers()
{
  for (int i = 0; i < 8; i++)
  {
    _memory.SetIoWriteStub(0x120 + i * 4, wsprxpth);
    _memory.SetIoWriteStub(0x122 + i * 4, wsprxptl);
    _memory.SetIoWriteStub(0x140 + i * 8, wsprxpos);
    _memory.SetIoWriteStub(0x142 + i * 8, wsprxctl);
    _memory.SetIoWriteStub(0x144 + i * 8, wsprxdata);
    _memory.SetIoWriteStub(0x146 + i * 8, wsprxdatb);
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

void SpriteRegisters::HardReset()
{
  ClearState();
}

void SpriteRegisters::EmulationStart(bool isCycleBasedEmulation)
{
  _isCycleBasedEmulation = isCycleBasedEmulation;

  InstallIOHandlers();
}

SpriteRegisters::SpriteRegisters(IMemory &memory) : _memory(memory), _isCycleBasedEmulation(false)
{
  ClearState();
}
