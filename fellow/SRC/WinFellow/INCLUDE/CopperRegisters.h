#pragma once

#include "Defs.h"

class CopperRegisters
{
public:
  uint32_t copcon;
  uint32_t cop1lc;
  uint32_t cop2lc;
  uint32_t copper_pc;
  bool copper_dma;                /* Mirrors DMACON */
  uint32_t copper_suspended_wait; /* Position the copper should have been waiting for */
                                  /* if copper DMA had been turned on */

  void InstallIOHandlers();

  void ClearState();
  void LoadState(FILE *F);
  void SaveState(FILE *F);
};

extern CopperRegisters copper_registers;
