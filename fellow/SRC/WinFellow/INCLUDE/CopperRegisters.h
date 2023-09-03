#ifndef COPPERREGISTERS_H
#define COPPERREGISTERS_H

#include "DEFS.H"

class CopperRegisters
{
public:
  ULO copcon;
  ULO cop1lc;
  ULO cop2lc;
  ULO copper_pc;
  bool copper_dma;                                    /* Mirrors DMACON */
  ULO copper_suspended_wait;/* Position the copper should have been waiting for */
                            /* if copper DMA had been turned on */

  void InstallIOHandlers();

  void ClearState();
  void LoadState(FILE *F);
  void SaveState(FILE *F);
};

extern CopperRegisters copper_registers;

#endif
