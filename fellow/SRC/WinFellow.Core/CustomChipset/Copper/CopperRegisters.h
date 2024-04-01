#pragma once

#include <cstdint>
#include <cstdio>

#include "Scheduler/MasterTimestamp.h"
#include "Memory/IMemory.h"

class CopperRegisters
{
private:
  IMemory &_memory;

public:
  uint32_t copcon;
  uint32_t cop1lc;
  uint32_t cop2lc;
  uint32_t copper_pc;
  bool copper_dma;                       // Mirrors DMACON
  MasterTimestamp copper_suspended_wait; // Position the copper should have been waiting for or woken up at if copper DMA had been on

  static void wcopcon(uint16_t data, uint32_t address);
  static void wcop1lch(uint16_t data, uint32_t address);
  static void wcop1lcl(uint16_t data, uint32_t address);
  static void wcop2lch(uint16_t data, uint32_t address);
  static void wcop2lcl(uint16_t data, uint32_t address);
  static uint16_t rcopjmp1(uint32_t address);
  static void wcopjmp1(uint16_t data, uint32_t address);
  static uint16_t rcopjmp2(uint32_t address);
  static void wcopjmp2(uint16_t data, uint32_t address);

  void SetCopCon(uint16_t data);
  void SetCop1LcH(uint16_t data);
  void SetCop1LcL(uint16_t data);
  void SetCop2LcH(uint16_t data);
  void SetCop2LcL(uint16_t data);
  void SetCopJmp1();
  uint16_t GetCopJmp1();
  void SetCopJmp2();
  uint16_t GetCopJmp2();

  void InstallIOHandlers();

  void ClearState();

  CopperRegisters(IMemory &memory);
};
