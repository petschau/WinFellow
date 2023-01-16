#include "CopperRegisters.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/chipset/CycleExactCopper.h"
#include "LineExactCopper.h"
#include "fellow/memory/Memory.h"
#include "fellow/scheduler/Scheduler.h"

//============================================================================
// Copper registers
//============================================================================

CopperRegisters copper_registers;

// COPCON
// $dff02e
void wcopcon(UWO data, ULO address)
{
  copper_registers.copcon = data;
}

// COP1LC - High
// $dff080
void wcop1lch(UWO data, ULO address)
{
  copper_registers.cop1lc = chipsetReplaceHighPtr(copper_registers.cop1lc, data);
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.NotifyCop1lcChanged();
  }
  else
  {
    line_exact_copper.NotifyCop1lcChanged();
  }
}

// COP1LC - Low
// $dff082
void wcop1lcl(UWO data, ULO address)
{
  copper_registers.cop1lc = chipsetReplaceLowPtr(copper_registers.cop1lc, data);
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.NotifyCop1lcChanged();
  }
  else
  {
    line_exact_copper.NotifyCop1lcChanged();
  }
}

// COP2LC - High
// $dff084
void wcop2lch(UWO data, ULO address)
{
  copper_registers.cop2lc = chipsetReplaceHighPtr(copper_registers.cop2lc, data);
}

// COP2LC - Low
// $dff086
void wcop2lcl(UWO data, ULO address)
{
  copper_registers.cop2lc = chipsetReplaceLowPtr(copper_registers.cop2lc, data);
}

// COPJMP1
// $dff088
void wcopjmp1(UWO data, ULO address)
{
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.LoadNow(0);
  }
  else
  {
    line_exact_copper.Load(copper_registers.cop1lc);
  }
}

// COPJMP1
// $dff088
UWO rcopjmp1(ULO address)
{
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.LoadNow(0);
  }
  else
  {
    line_exact_copper.Load(copper_registers.cop1lc);
  }
  return 0;
}

// COPJMP2
// $dff08A
void wcopjmp2(UWO data, ULO address)
{
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.LoadNow(1);
  }
  else
  {
    line_exact_copper.Load(copper_registers.cop2lc);
  }
}

// COPJMP2
// $dff08A
UWO rcopjmp2(ULO address)
{
  if (chipset_info.IsCycleExact)
  {
    cycle_exact_copper.LoadNow(1);
  }
  else
  {
    line_exact_copper.Load(copper_registers.cop2lc);
  }
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
  PC = cop1lc;
  InstructionStart = cop1lc;
  SuspendedWait = SchedulerEvent::EventDisableCycle;
  IsDMAEnabled = false;
}
