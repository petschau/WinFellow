#pragma once

#include <cstdint>

#include "Memory/IMemory.h"

class SpriteRegisters
{
private:
  IMemory &_memory;
  bool _isCycleBasedEmulation;

  static unsigned int GetSpriteNumberForControlWords(uint32_t address);
  static unsigned int GetSpriteNumberForPointer(uint32_t address);

public:
  uint32_t sprpt[8];
  uint16_t sprpos[8];
  uint16_t sprctl[8];
  uint16_t sprdata[8];
  uint16_t sprdatb[8];

  static void wsprxpth(uint16_t data, uint32_t address);
  static void wsprxptl(uint16_t data, uint32_t address);
  static void wsprxpos(uint16_t data, uint32_t address);
  static void wsprxctl(uint16_t data, uint32_t address);
  static void wsprxdata(uint16_t data, uint32_t address);
  static void wsprxdatb(uint16_t data, uint32_t address);

  void SetPth(unsigned int spriteNumber, uint16_t data);
  void SetPtl(unsigned int spriteNumber, uint16_t data);
  void SetPos(unsigned int spriteNumber, uint16_t data);
  void SetCtl(unsigned int spriteNumber, uint16_t data);
  void SetDatA(unsigned int spriteNumber, uint16_t data);
  void SetDatB(unsigned int spriteNumber, uint16_t data);

  void InstallIOHandlers();

  void ClearState();

  void HardReset();
  void EmulationStart(bool isCycleBasedEmulation);

  SpriteRegisters(IMemory &memory);
};
