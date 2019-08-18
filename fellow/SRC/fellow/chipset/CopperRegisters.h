#pragma once

#include "fellow/api/defs.h"

class CopperRegisters
{
public:
  UWO copcon;
  ULO cop1lc;
  ULO cop2lc;

  ULO PC;
  ULO InstructionStart;

  bool IsDMAEnabled; // Mirrors DMACON
  ULO SuspendedWait; // Position the copper should have been waiting for if copper DMA had been turned on

  void InstallIOHandlers();

  void ClearState();
};

extern CopperRegisters copper_registers;
