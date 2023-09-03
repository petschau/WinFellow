#include "CopperRegisters.h"
#include "COPPER.H"

#include "chipset.h"
#include "FMEM.H"

/*============================================================================*/
/* Copper registers                                                           */
/*============================================================================*/

CopperRegisters copper_registers;

/*
========
COPCON
========
$dff02e
*/

void wcopcon(UWO data, ULO address)
{
  copper_registers.copcon = data;
}

/*
=======
COP1LC
=======
$dff080
*/

void wcop1lch(UWO data, ULO address)
{
  copper_registers.cop1lc = chipsetReplaceHighPtr(copper_registers.cop1lc, data);
  copper->NotifyCop1lcChanged();
}

/* $dff082 */

void wcop1lcl(UWO data, ULO address)
{
  copper_registers.cop1lc = chipsetReplaceLowPtr(copper_registers.cop1lc, data);
  copper->NotifyCop1lcChanged();
}

/*
=======
COP2LC
=======

$dff084
*/

void wcop2lch(UWO data, ULO address)
{
  copper_registers.cop2lc = chipsetReplaceHighPtr(copper_registers.cop2lc, data);
}

/* $dff082 */

void wcop2lcl(UWO data, ULO address)
{
  copper_registers.cop2lc = chipsetReplaceLowPtr(copper_registers.cop2lc, data);
}

/*
========
COPJMP1
========
$dff088
*/

void wcopjmp1(UWO data, ULO address)
{
  copper->Load(copper_registers.cop1lc);
}

UWO rcopjmp1(ULO address)
{
  copper->Load(copper_registers.cop1lc);
  return 0;
}

/*
========
COPJMP2
========
$dff08A
*/


void wcopjmp2(UWO data, ULO address)
{
  copper->Load(copper_registers.cop2lc);
}

UWO rcopjmp2(ULO address)
{
  copper->Load(copper_registers.cop2lc);
  return 0;
}

void CopperRegisters::InstallIOHandlers()
{
  memorySetIoWriteStub(0x02e, wcopcon);
  memorySetIoWriteStub(0x080, wcop1lch);
  memorySetIoWriteStub(0x082, wcop1lcl);
  memorySetIoWriteStub(0x084, wcop2lch);
  memorySetIoWriteStub(0x086, wcop2lcl);
  memorySetIoWriteStub(0x088, wcopjmp1);
  memorySetIoWriteStub(0x08a, wcopjmp2);
  memorySetIoReadStub(0x088, rcopjmp1);
  memorySetIoReadStub(0x08a, rcopjmp2);
}

void CopperRegisters::ClearState()
{
  cop1lc = 0;
  cop2lc = 0;
  copper_suspended_wait = 0xffffffff;
  copper_pc = cop1lc;
  copper_dma = false;
}

void CopperRegisters::LoadState(FILE *F)
{
  fread(&copcon, sizeof(copcon), 1, F);
  fread(&cop1lc, sizeof(cop1lc), 1, F);
  fread(&cop2lc, sizeof(cop2lc), 1, F);
  fread(&copper_pc, sizeof(copper_pc), 1, F);
  fread(&copper_dma, sizeof(copper_dma), 1, F);
  fread(&copper_suspended_wait, sizeof(copper_suspended_wait), 1, F);
}

void CopperRegisters::SaveState(FILE *F)
{
  fwrite(&copcon, sizeof(copcon), 1, F);
  fwrite(&cop1lc, sizeof(cop1lc), 1, F);
  fwrite(&cop2lc, sizeof(cop2lc), 1, F);
  fwrite(&copper_pc, sizeof(copper_pc), 1, F);
  fwrite(&copper_dma, sizeof(copper_dma), 1, F);
  fwrite(&copper_suspended_wait, sizeof(copper_suspended_wait), 1, F);
}
