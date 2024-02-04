#include "CustomChipset/Copper/CopperRegisters.h"
#include "VirtualHost/Core.h"
#include "chipset.h"

// COPCON
// $dff02e

void CopperRegisters::wcopcon(uint16_t data, uint32_t address)
{
  _core.CopperRegisters->SetCopCon(data);
}

// COP1LC
// $dff080

void CopperRegisters::wcop1lch(uint16_t data, uint32_t address)
{
  _core.CopperRegisters->SetCop1LcH(data);
}

// $dff082

void CopperRegisters::wcop1lcl(uint16_t data, uint32_t address)
{
  _core.CopperRegisters->SetCop1LcL(data);
}

// COP2LC
// $dff084

void CopperRegisters::wcop2lch(uint16_t data, uint32_t address)
{
  _core.CopperRegisters->SetCop2LcH(data);
}

// $dff082

void CopperRegisters::wcop2lcl(uint16_t data, uint32_t address)
{
  _core.CopperRegisters->SetCop2LcL(data);
}

// COPJMP1
// $dff088

void CopperRegisters::wcopjmp1(uint16_t data, uint32_t address)
{
  _core.CopperRegisters->SetCopJmp1();
}

uint16_t CopperRegisters::rcopjmp1(uint32_t address)
{
  return _core.CopperRegisters->GetCopJmp1();
}

// COPJMP2
// $dff08A

void CopperRegisters::wcopjmp2(uint16_t data, uint32_t address)
{
  _core.CopperRegisters->SetCopJmp2();
}

uint16_t CopperRegisters::rcopjmp2(uint32_t address)
{
  return _core.CopperRegisters->GetCopJmp2();
}

//---------------------------------------------

void CopperRegisters::SetCopCon(uint16_t data)
{
  copcon = data;
}

void CopperRegisters::SetCop1LcH(uint16_t data)
{
  cop1lc = chipsetReplaceHighPtr(cop1lc, data);
  _core.CurrentCopper->NotifyCop1lcChanged();
}

void CopperRegisters::SetCop1LcL(uint16_t data)
{
  cop1lc = chipsetReplaceLowPtr(cop1lc, data);
  _core.CurrentCopper->NotifyCop1lcChanged();
}

void CopperRegisters::SetCop2LcH(uint16_t data)
{
  cop2lc = chipsetReplaceHighPtr(cop2lc, data);
}

void CopperRegisters::SetCop2LcL(uint16_t data)
{
  cop2lc = chipsetReplaceLowPtr(cop2lc, data);
}

void CopperRegisters::SetCopJmp1()
{
  _core.CurrentCopper->Load(cop1lc);
}

uint16_t CopperRegisters::GetCopJmp1()
{
  _core.CurrentCopper->Load(cop1lc);
  return 0;
}

void CopperRegisters::SetCopJmp2()
{
  _core.CurrentCopper->Load(cop2lc);
}

uint16_t CopperRegisters::GetCopJmp2()
{
  _core.CurrentCopper->Load(cop2lc);
  return 0;
}

void CopperRegisters::InstallIOHandlers()
{
  _memory.SetIoWriteStub(0x02e, wcopcon);
  _memory.SetIoWriteStub(0x080, wcop1lch);
  _memory.SetIoWriteStub(0x082, wcop1lcl);
  _memory.SetIoWriteStub(0x084, wcop2lch);
  _memory.SetIoWriteStub(0x086, wcop2lcl);
  _memory.SetIoWriteStub(0x088, wcopjmp1);
  _memory.SetIoWriteStub(0x08a, wcopjmp2);
  _memory.SetIoReadStub(0x088, rcopjmp1);
  _memory.SetIoReadStub(0x08a, rcopjmp2);
}

void CopperRegisters::ClearState()
{
  cop1lc = 0;
  cop2lc = 0;
  copper_suspended_wait = SchedulerEvent::EventDisableCycle;
  copper_pc = cop1lc;
  copper_dma = false;
}

CopperRegisters::CopperRegisters(IMemory &memory) : _memory(memory)
{
  ClearState();
}
