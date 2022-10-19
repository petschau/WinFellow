#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/BitplaneUtility.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/chipset/DDFStateMachine.h"
#include "fellow/chipset/DIWXStateMachine.h"
#include "fellow/chipset/DIWYStateMachine.h"
#include "fellow/chipset/HostFrameDelayedRenderer.h"
#include "fellow/chipset/HostFrameImmediateRenderer.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/memory/Memory.h"

BitplaneRegisters bitplane_registers;

// vposr - $dff004 Read vpos and Agnus/Alice ID bits on ECS and later
UWO rvposr(ULO address)
{
  return bitplane_registers.GetVPosR();
}

// vhposr - $dff006
UWO rvhposr(ULO address)
{
  return BitplaneRegisters::GetVHPosR();
}

// vpos - $dff02a
void wvpos(UWO data, ULO address)
{
  bitplane_registers.SetVPos(data);
}

// vhpos - $dff02c
void wvhpos(UWO data, ULO address)
{
  bitplane_registers.SetVHPos(data);
}

// deniseid - $dff07c, ECS Denise and later
UWO rdeniseid(ULO address)
{
  return BitplaneRegisters::GetDeniseId();
}

// diwstrt - $dff08e
void wdiwstrt(UWO data, ULO address)
{
  bitplane_registers.SetDIWStrt(data);
}

// diwstop - $dff090
void wdiwstop(UWO data, ULO address)
{
  bitplane_registers.SetDIWStop(data);
}

// ddfstrt - $dff092
void wddfstrt(UWO data, ULO address)
{
  bitplane_registers.SetDDFStrt(data);
}

// ddfstop - $dff094
void wddfstop(UWO data, ULO address)
{
  bitplane_registers.SetDDFStop(data);
}

// bplxpt - $dff0e0 to $dff0f6
void wbpl1pth(UWO data, ULO address)
{
  bitplane_registers.SetBplPtHigh(0, data);
}

void wbpl1ptl(UWO data, ULO address)
{
  bitplane_registers.SetBplPtLow(0, data);
}

void wbpl2pth(UWO data, ULO address)
{
  bitplane_registers.SetBplPtHigh(1, data);
}

void wbpl2ptl(UWO data, ULO address)
{
  bitplane_registers.SetBplPtLow(1, data);
}

void wbpl3pth(UWO data, ULO address)
{
  bitplane_registers.SetBplPtHigh(2, data);
}

void wbpl3ptl(UWO data, ULO address)
{
  bitplane_registers.SetBplPtLow(2, data);
}

void wbpl4pth(UWO data, ULO address)
{
  bitplane_registers.SetBplPtHigh(3, data);
}

void wbpl4ptl(UWO data, ULO address)
{
  bitplane_registers.SetBplPtLow(3, data);
}

void wbpl5pth(UWO data, ULO address)
{
  bitplane_registers.SetBplPtHigh(4, data);
}

void wbpl5ptl(UWO data, ULO address)
{
  bitplane_registers.SetBplPtLow(4, data);
}

void wbpl6pth(UWO data, ULO address)
{
  bitplane_registers.SetBplPtHigh(5, data);
}

void wbpl6ptl(UWO data, ULO address)
{
  bitplane_registers.SetBplPtLow(5, data);
}

// bplcon0 - $dff100
void wbplcon0(UWO data, ULO address)
{
  bitplane_registers.SetBplCon0(data);
}

// bplcon1 - $dff102
void wbplcon1(UWO data, ULO address)
{
  bitplane_registers.SetBplCon1(data);
}

// bplcon2 - $dff104
void wbplcon2(UWO data, ULO address)
{
  bitplane_registers.SetBplCon2(data);
}

// bplxmod - $dff108 to $dff10a
void wbpl1mod(UWO data, ULO address)
{
  bitplane_registers.SetBpl1Mod(data);
}

void wbpl2mod(UWO data, ULO address)
{
  bitplane_registers.SetBpl2Mod(data);
}

// bplxdat - $dff110 to $dff11a
void wbpl1dat(UWO data, ULO address)
{
  bitplane_registers.SetBplDat(0, data);
}

void wbpl2dat(UWO data, ULO address)
{
  bitplane_registers.SetBplDat(1, data);
}

void wbpl3dat(UWO data, ULO address)
{
  bitplane_registers.SetBplDat(2, data);
}

void wbpl4dat(UWO data, ULO address)
{
  bitplane_registers.SetBplDat(3, data);
}

void wbpl5dat(UWO data, ULO address)
{
  bitplane_registers.SetBplDat(4, data);
}

void wbpl6dat(UWO data, ULO address)
{
  bitplane_registers.SetBplDat(5, data);
}

// color - $dff180 to $dff1be
void wcolor(UWO data, ULO address)
{
  const ULO colorIndex = ((address & 0x1ff) - 0x180) >> 1;
  bitplane_registers.SetColor(colorIndex, data);
}

void wdiwhigh(UWO data, ULO address)
{
  bitplane_registers.SetDIWHigh(data);
}

void whtotal(UWO data, ULO address)
{
  bitplane_registers.SetHTotal(data);
}

void whcenter(UWO data, ULO address)
{
  bitplane_registers.SetHCenter(data);
}

void whbstrt(UWO data, ULO address)
{
  bitplane_registers.SetHBStrt(data);
}

void whbstop(UWO data, ULO address)
{
  bitplane_registers.SetHBStop(data);
}

void whsstrt(UWO data, ULO address)
{
  bitplane_registers.SetHSStrt(data);
}

void whsstop(UWO data, ULO address)
{
  bitplane_registers.SetHSStop(data);
}

void wvtotal(UWO data, ULO address)
{
  bitplane_registers.SetVTotal(data);
}

void wvbstrt(UWO data, ULO address)
{
  bitplane_registers.SetVBStrt(data);
}

void wvbstop(UWO data, ULO address)
{
  bitplane_registers.SetVBStop(data);
}

void wvsstrt(UWO data, ULO address)
{
  bitplane_registers.SetVSStrt(data);
}

void wvsstop(UWO data, ULO address)
{
  bitplane_registers.SetVSStop(data);
}

void wbeamcon0(UWO data, ULO address)
{
  bitplane_registers.SetBeamcon0(data);
}

UWO BitplaneRegisters::GetVPosR() const
{
  UWO value = lof;
  if (chipsetGetECS())
  {
    value |= GetAgnusId() << 8;                   // Fat Pal Agnus id
    value |= (scheduler.GetRasterY() >> 8) & 0x7; // ECS adds V9 and V10
  }
  else // OCS
  {
    value |= (scheduler.GetRasterY() >> 8) & 1;
  }
  return value;
}

UWO BitplaneRegisters::GetVHPosR()
{
  return (UWO)((scheduler.GetRasterX() >> 3) | ((scheduler.GetRasterY() & 0x000000FF) << 8));
}

void BitplaneRegisters::SetVPos(UWO data)
{
  lof = (UWO)(data & 0x8000);

  // TODO: Add changing of raster position counters
}

void BitplaneRegisters::SetVHPos(UWO data)
{
  // TODO: Add changing of raster position counters
}

UWO BitplaneRegisters::GetAgnusId()
{
  return chipsetGetECS() ? 0x20 : 0;
}

UWO BitplaneRegisters::GetDeniseId()
{
  // Not installed in the io map on OCS, in that case random values are returned
  return chipsetGetECS() ? 0xfc : 0; // 0xfc = ECS Denise
}

UWO BitplaneRegisters::GetVerticalPosition()
{
  return (UWO)scheduler.GetRasterY();
}

UWO BitplaneRegisters::GetHorisontalPosition()
{
  return (UWO)scheduler.GetRasterX();
}

void BitplaneRegisters::ResetDiwhigh()
{
  diwhigh = (~diwstop >> 7) & 0x100;
}

void BitplaneRegisters::SetDIWStrt(UWO data)
{
  const UWO newDiwXStart = (data & 0xff) << 2;
  const UWO newDiwYStart = data >> 8;
  const bool diwXStartChanged = newDiwXStart != DiwXStart;
  const bool diwYStartChanged = newDiwYStart != DiwYStart;
  const bool wasChanged = diwXStartChanged || diwYStartChanged;

  if (chipset_info.IsCycleExact && wasChanged)
  {
    FlushShifter();
  }

  diwstrt = data;
  DiwXStart = newDiwXStart;
  DiwYStart = newDiwYStart;
  ResetDiwhigh();

  if (chipset_info.IsCycleExact)
  {
    if (diwXStartChanged)
    {
      diwx_state_machine.NotifyDIWChanged();
    }

    if (diwYStartChanged)
    {
      diwy_state_machine.NotifyDIWStrtChanged(DiwYStart);
    }
  }
  else
  {
    graphNotifyDIWStrtChanged();
  }
}

void BitplaneRegisters::SetDIWStop(UWO data)
{
  const UWO newDiwXStop = ((data & 0xff) ^ 0x100) << 2;
  const UWO newDiwYStop = (data >> 8) | (~(data >> 7) & 0x100);
  const bool diwXStopChanged = newDiwXStop != DiwXStop;
  const bool diwYStopChanged = newDiwYStop != DiwYStop;
  const bool wasChanged = diwXStopChanged || diwYStopChanged;

  if (chipset_info.IsCycleExact && wasChanged)
  {
    FlushShifter();
  }

  diwstop = data;
  DiwXStop = newDiwXStop;
  DiwYStop = newDiwYStop;
  ResetDiwhigh();

  if (chipset_info.IsCycleExact)
  {
    if (diwXStopChanged)
    {
      diwx_state_machine.NotifyDIWChanged();
    }

    if (diwYStopChanged)
    {
      diwy_state_machine.NotifyDIWStopChanged(DiwYStop);
    }
  }
  else
  {
    graphNotifyDIWStopChanged();
  }
}

void BitplaneRegisters::SetDDFStrt(UWO data)
{
  const UWO ddfstrtNew = data & chipsetGetDDFMask();

  if (ddfstrt != ddfstrtNew)
  {
    if (chipset_info.IsCycleExact)
    {
      FlushShifter();
    }

    ddfstrt = ddfstrtNew;

    if (chipset_info.IsCycleExact)
    {
      ddf_state_machine.ChangedValue();
    }
    else
    {
      graphNotifyDDFStrtChanged();
    }
  }
}

void BitplaneRegisters::SetDDFStop(UWO data)
{
  const UWO ddfstopNew = data & chipsetGetDDFMask();
  if (ddfstop != ddfstopNew)
  {
    if (chipset_info.IsCycleExact)
    {
      FlushShifter();
    }

    ddfstop = ddfstopNew;

    if (chipset_info.IsCycleExact)
    {
      ddf_state_machine.ChangedValue();
    }
    else
    {
      graphNotifyDDFStopChanged();
    }
  }
}

void BitplaneRegisters::SetBplPtHigh(unsigned int index, UWO data)
{
  const ULO newValue = chipsetReplaceHighPtr(bplpt[index], data);
  if (newValue != bplpt[index])
  {
    if (chipset_info.IsCycleExact)
    {
      FlushShifter();
    }

    bplpt[index] = newValue;
  }
}

void BitplaneRegisters::SetBplPtLow(unsigned int index, UWO data)
{
  const ULO newValue = chipsetReplaceLowPtr(bplpt[index], data);
  if (newValue != bplpt[index])
  {
    if (chipset_info.IsCycleExact)
    {
      FlushShifter();
    }

    bplpt[index] = newValue;
  }
}

void BitplaneRegisters::SetBplCon0(UWO data)
{
  if (bplcon0 != data)
  {
    if (chipset_info.IsCycleExact)
    {
      FlushShifter(); // Make sure shifting including current bus cycle is commited using old values
    }

    bplcon0 = data;
    IsLores = (bitplane_registers.bplcon0 & 0x8000) == 0;
    IsHires = !IsLores;
    IsDualPlayfield = (bitplane_registers.bplcon0 & 0x0400) == 0x0400;
    IsHam = (bitplane_registers.bplcon0 & 0x0800) == 0x0800;
    EnabledBitplaneCount = (bitplane_registers.bplcon0 >> 12) & 7;

    if (!chipset_info.IsCycleExact)
    {
      graphNotifyBplCon0Changed();
    }
    else
    {
      bitplane_shifter.UpdateDrawBatchFunc();
    }
  }
}

// when ddfstrt is (mod 8)+4, shift order is 8-f,0-7 (lores) (Example: New Zealand Story)
// when ddfstrt is (mod 8)+2, shift order is 4-7,0-3 (hires)
void BitplaneRegisters::SetBplCon1(UWO data)
{
  if (chipset_info.IsCycleExact)
  {
    if (bplcon1 != (data & 0xff))
    {
      FlushShifter();
    }
  }

  bplcon1 = data & 0xff;

  if (!chipset_info.IsCycleExact)
  {
    graphNotifyBplCon1Changed();
  }
}

void BitplaneRegisters::SetBplCon2(UWO data)
{
  bplcon2 = data;
}

void BitplaneRegisters::SetBpl1Mod(UWO data)
{
  const UWO new_value = data & 0xfffe;
  if (chipset_info.IsCycleExact)
  {
    if (bpl1mod != new_value)
    {
      FlushShifter();
    }
  }

  bpl1mod = new_value;
}

void BitplaneRegisters::SetBpl2Mod(UWO data)
{
  const UWO new_value = data & 0xfffe;
  if (chipset_info.IsCycleExact)
  {
    if (bpl2mod != new_value)
    {
      FlushShifter();
    }
  }

  bpl2mod = new_value;
}

void BitplaneRegisters::SetBplDat(unsigned int index, UWO data)
{
  if (chipset_info.IsCycleExact)
  {
    bpldat[index] = data;

    if (index == 0 && BitplaneShifter_AddData != nullptr)
    {
      (*BitplaneShifter_AddData)(bpldat);
    }
  }
  // Line exact mode does not use this register.
}

void BitplaneRegisters::SetColor(unsigned int index, UWO data)
{
  UWO newColor = data & 0x0fff;

  if (color[index] != newColor)
  {
    UWO newHalfbriteColor = (newColor & 0xeee) >> 1;

    if (chipset_info.IsCycleExact)
    {
      const SHResTimestamp &shresTimestamp = scheduler.GetSHResTimestamp();
      ULO line = shresTimestamp.GetUnwrappedLine();
      bool isOutsideVerticalBlank = line >= scheduler.GetVerticalBlankEnd();

      if (chipset_info.GfxDebugImmediateRendering)
      {
        if (isOutsideVerticalBlank)
        {
          FlushShifter();
        }

        PublishColorChanged(index, newColor, newHalfbriteColor);
      }
      else
      {
        if (isOutsideVerticalBlank)
        {
          host_frame_delayed_renderer.ChangeList.AddColorChange(line, shresTimestamp.GetUnwrappedFirstPixelInNextCylinder(), index, newColor, newHalfbriteColor);
        }
        else
        {
          PublishColorChanged(index, newColor, newHalfbriteColor);
        }
      }
    }
    else
    {
      PublishColorChanged(index, newColor, newHalfbriteColor);
    }

    color[index] = newColor;
    color[index + 32] = newHalfbriteColor;
  }
}

void BitplaneRegisters::SetDIWHigh(UWO data)
{
  const UWO newDiwXStart = (DiwXStart & 0x3fc) | ((data & 0x18) >> 3) | ((data & 0x20) << 5);
  const UWO newDiwXStop = (DiwXStop & 0x3fc) | ((data & 0x1800) >> 11) | ((data & 0x2000) >> 3);
  const UWO newDiwYStart = (DiwYStart & 0xff) | (data & 0x7) << 8;
  const UWO newDiwYStop = (DiwYStop & 0xff) | (data & 0x700);

  const bool diwXStartChanged = newDiwXStart != DiwXStart;
  const bool diwXStopChanged = newDiwXStop != DiwXStop;
  const bool diwYStartChanged = newDiwYStart != DiwYStart;
  const bool diwYStopChanged = newDiwYStop != DiwYStop;
  const bool wasChanged = diwXStartChanged || diwXStopChanged || diwYStartChanged || diwYStopChanged;

  if (chipset_info.IsCycleExact && wasChanged)
  {
    FlushShifter();
  }

  diwhigh = data;
  DiwXStart = newDiwXStart;
  DiwXStop = newDiwXStop;
  DiwYStart = newDiwYStart;
  DiwYStop = newDiwYStop;

  if (wasChanged)
  {
    if (chipset_info.IsCycleExact)
    {
      if (diwXStartChanged || diwXStopChanged)
      {
        diwx_state_machine.NotifyDIWChanged();
      }

      if (diwYStartChanged)
      {
        diwy_state_machine.NotifyDIWStrtChanged(DiwYStart);
      }

      if (diwYStopChanged)
      {
        diwy_state_machine.NotifyDIWStopChanged(DiwYStop);
      }
    }
  }

  if (!chipset_info.IsCycleExact)
  {
    graphNotifyDIWStrtChanged();
    graphNotifyDIWStopChanged();
  }
}

void BitplaneRegisters::SetHTotal(UWO data)
{
  constexpr UWO mask = 0xff;
  htotal = data & mask;
}

void BitplaneRegisters::SetHCenter(UWO data)
{
  constexpr UWO mask = 0xff;
  hcenter = data & mask;
}

void BitplaneRegisters::SetHBStrt(UWO data)
{
  // SHres bits in AGA only?
  constexpr UWO mask = 0xff;
  hbstrt = data & mask;
}

void BitplaneRegisters::SetHBStop(UWO data)
{
  // SHres bits in AGA only?
  constexpr UWO mask = 0xff;
  hbstop = data & mask;
}

void BitplaneRegisters::SetHSStrt(UWO data)
{
  // SHres bits in AGA only?
  constexpr UWO mask = 0xff;
  hsstrt = data & mask;
}

void BitplaneRegisters::SetHSStop(UWO data)
{
  // SHres bits in AGA only?
  constexpr UWO mask = 0xff;
  hsstop = data & mask;
}

void BitplaneRegisters::SetVTotal(UWO data)
{
  // HRM v3 says nothing about number of bits for verticals in programmed mode
  // However DIWHIGH officially implements V10, V9 and V8 in ECS (and possibly V11 as mentioned on eab UAHS thread)
  constexpr UWO mask = 0x7ff;
  vtotal = data & mask;
}

void BitplaneRegisters::SetVBStrt(UWO data)
{
  constexpr UWO mask = 0x7ff;
  vbstrt = data & mask;
}

void BitplaneRegisters::SetVBStop(UWO data)
{
  constexpr UWO mask = 0x7ff;
  vbstop = data & mask;
}

void BitplaneRegisters::SetVSStrt(UWO data)
{
  constexpr UWO mask = 0x7ff;
  vsstrt = data & mask;
}

void BitplaneRegisters::SetVSStop(UWO data)
{
  constexpr UWO mask = 0x7ff;
  vsstop = data & mask;
}

void BitplaneRegisters::SetBeamcon0(UWO data)
{
  constexpr UWO mask = 0x7fff;
  UWO newBeamcon0 = data & mask;

  if (newBeamcon0 != beamcon0)
  {
    beamcon0 = newBeamcon0;
    InferBeamcon0Values();
  }
}

void BitplaneRegisters::InferBeamcon0Values()
{
  HardBlankDisable = beamcon0 & 0x4000;
  IgnoreLatchedPenValueOnVerticalPosRead = beamcon0 & 0x2000;
  HardVerticalWindowDisable = beamcon0 & 0x1000;
  LongLineToggleDisable = beamcon0 & 0x800;
  CompositeSyncRedirection = beamcon0 & 0x400;
  VariableVerticalSyncEnable = beamcon0 & 0x200;
  VariableHorisontalSyncEnable = beamcon0 & 0x100;
  VariableBeamCounterComparatorEnable = beamcon0 & 0x80;
  SpecialUltraResolutionModeEnable = beamcon0 & 0x40;
  ProgrammablePALModeEnable = beamcon0 & 0x20;
  VariableCompositeSyncEnable = beamcon0 & 0x10;
  CompositeBlankRedirectionEnable = beamcon0 & 0x8;
  CSyncPinTrue = beamcon0 & 0x4;
  VSyncPinTrue = beamcon0 & 0x2;
  HSyncPinTrue = beamcon0 & 0x1;
}

void BitplaneRegisters::LinearAddModulo(unsigned int enabledBitplaneCount, ULO bplLengthInBytes)
{
  const ULO mod1 = (ULO)(LON)(WOR)bpl1mod; // Sign extend
  const ULO mod2 = (ULO)(LON)(WOR)bpl2mod;

  switch (enabledBitplaneCount)
  {
    case 6: AddBplPt(5, bplLengthInBytes + mod2);
    case 5: AddBplPt(4, bplLengthInBytes + mod1);
    case 4: AddBplPt(3, bplLengthInBytes + mod2);
    case 3: AddBplPt(2, bplLengthInBytes + mod1);
    case 2: AddBplPt(1, bplLengthInBytes + mod2);
    case 1: AddBplPt(0, bplLengthInBytes + mod1);
    default:;
  }
}

void BitplaneRegisters::AddModulo()
{
  const ULO mod1 = (ULO)(LON)(WOR)bpl1mod; // Sign extend
  const ULO mod2 = (ULO)(LON)(WOR)bpl2mod;

  switch (BitplaneUtility::GetEnabledBitplaneCount())
  {
    case 6: AddBplPt(5, mod2);
    case 5: AddBplPt(4, mod1);
    case 4: AddBplPt(3, mod2);
    case 3: AddBplPt(2, mod1);
    case 2: AddBplPt(1, mod2);
    case 1: AddBplPt(0, mod1);
    default:;
  }
}

void BitplaneRegisters::AddBplPt(unsigned int bitplaneIndex, ULO add)
{
  bplpt[bitplaneIndex] = chipsetMaskPtr(bplpt[bitplaneIndex] + add);
}

void BitplaneRegisters::PublishColorChanged(const unsigned int colorIndex, const UWO color12Bit, const UWO halfbriteColor12Bit)
{
  if (_colorChangeEventHandler != nullptr)
  {
    _colorChangeEventHandler->ColorChangedHandler(colorIndex, color12Bit, halfbriteColor12Bit);
  }
}

void BitplaneRegisters::InstallIOHandlers()
{
  memorySetIoReadStub(0x004, rvposr);
  memorySetIoReadStub(0x006, rvhposr);
  memorySetIoWriteStub(0x02a, wvpos);
  memorySetIoWriteStub(0x02c, wvhpos);
  memorySetIoWriteStub(0x08e, wdiwstrt);
  memorySetIoWriteStub(0x090, wdiwstop);
  memorySetIoWriteStub(0x092, wddfstrt);
  memorySetIoWriteStub(0x094, wddfstop);
  memorySetIoWriteStub(0x0e0, wbpl1pth);
  memorySetIoWriteStub(0x0e2, wbpl1ptl);
  memorySetIoWriteStub(0x0e4, wbpl2pth);
  memorySetIoWriteStub(0x0e6, wbpl2ptl);
  memorySetIoWriteStub(0x0e8, wbpl3pth);
  memorySetIoWriteStub(0x0ea, wbpl3ptl);
  memorySetIoWriteStub(0x0ec, wbpl4pth);
  memorySetIoWriteStub(0x0ee, wbpl4ptl);
  memorySetIoWriteStub(0x0f0, wbpl5pth);
  memorySetIoWriteStub(0x0f2, wbpl5ptl);
  memorySetIoWriteStub(0x0f4, wbpl6pth);
  memorySetIoWriteStub(0x0f6, wbpl6ptl);
  memorySetIoWriteStub(0x100, wbplcon0);
  memorySetIoWriteStub(0x102, wbplcon1);
  memorySetIoWriteStub(0x104, wbplcon2);
  memorySetIoWriteStub(0x108, wbpl1mod);
  memorySetIoWriteStub(0x10a, wbpl2mod);
  memorySetIoWriteStub(0x110, wbpl1dat);
  memorySetIoWriteStub(0x112, wbpl2dat);
  memorySetIoWriteStub(0x114, wbpl3dat);
  memorySetIoWriteStub(0x116, wbpl4dat);
  memorySetIoWriteStub(0x118, wbpl5dat);
  memorySetIoWriteStub(0x11a, wbpl6dat);

  for (unsigned int i = 0x180; i < 0x1c0; i += 2)
  {
    memorySetIoWriteStub(i, wcolor);
  }

  if (chipsetGetECS())
  {
    memorySetIoReadStub(0x07c, rdeniseid);
    memorySetIoWriteStub(0x1c0, whtotal);
    memorySetIoWriteStub(0x1c2, whsstop);
    memorySetIoWriteStub(0x1c4, whbstrt);
    memorySetIoWriteStub(0x1c6, whbstop);
    memorySetIoWriteStub(0x1c8, wvtotal);
    memorySetIoWriteStub(0x1ca, wvsstop);
    memorySetIoWriteStub(0x1cc, wvbstrt);
    memorySetIoWriteStub(0x1ce, wvbstop);
    memorySetIoWriteStub(0x1dc, wbeamcon0);
    memorySetIoWriteStub(0x1de, whsstrt);
    memorySetIoWriteStub(0x1e0, wvsstrt);
    memorySetIoWriteStub(0x1e2, whcenter);
    memorySetIoWriteStub(0x1e4, wdiwhigh);
  }
}

void BitplaneRegisters::SetAddDataCallback(IBitplaneShifter_AddDataCallback *bitplaneShifter_AddData)
{
  BitplaneShifter_AddData = bitplaneShifter_AddData;
}

void BitplaneRegisters::SetFlushCallback(IBitplaneShifter_FlushCallback *bitplaneShifter_Flush)
{
  BitplaneShifter_Flush = bitplaneShifter_Flush;
}

void BitplaneRegisters::SetColorChangeEventHandler(IColorChangeEventHandler *colorChangeEventHandler)
{
  _colorChangeEventHandler = colorChangeEventHandler;
}

void BitplaneRegisters::FlushShifter() const
{
  if (BitplaneShifter_Flush != nullptr)
  {
    (*BitplaneShifter_Flush)();
  }
}

void BitplaneRegisters::ClearState()
{
  lof = 0x8000; /* Long frame is default */
  diwstrt = 0;
  diwstop = 0;
  ddfstrt = 0;
  ddfstop = 0;

  for (unsigned int i = 0; i < 6; i++)
  {
    bplpt[i] = 0;
    bpldat[i] = 0;
  }

  bplcon0 = 0;
  bplcon1 = 0;
  bplcon2 = 0;
  bpl1mod = 0;
  bpl2mod = 0;

  for (unsigned int i = 0; i < 64; i++)
  {
    color[i] = 0;
  }

  ResetDiwhigh();
  DiwXStart = 0;
  DiwYStart = 0;
  DiwXStop = 0;
  DiwYStop = 0x100;

  // Not changeable on OCS
  // Initialize for PAL
  htotal = 227;  // 0xe3   (Total number of chip bus cycles in line, 228 == 0)
  hcenter = 113; // 0x71   (0xe2 CCK)
  hbstrt = 9;    // 0x9    (0x12 CCK)  First invisible CCK (Maybe 8?)
  hbstop = 44;   // 0x2c   (0x58 CCK)  First visible CCK (Maybe 46?)
  hsstrt = 18;   // 0x12   (0x24 CCK)  Horisontal sync start
  hsstop = 35;   // 0x23   (0x46 CCK)  Horisontal sync stop
  vtotal = 314;  // 0x138  Total number of lines in a long frame
  vbstrt = 0;    // 0x0    First invisible line
  vbstop = 25;   // 0x19   First visible line
  vsstrt = 3;    // 0x3    Vertical sync start
  vsstop = 6;    // 0x6    Vertical sync stop

  beamcon0 = 0;
  InferBeamcon0Values();

  IsLores = true;
  IsHires = false;
  IsDualPlayfield = false;
  IsHam = false;
  EnabledBitplaneCount = 0;
}

BitplaneRegisters::BitplaneRegisters() : BitplaneShifter_AddData(nullptr), BitplaneShifter_Flush(nullptr), _colorChangeEventHandler(nullptr)
{
  ClearState();
}

BitplaneRegisters::~BitplaneRegisters()
{
  delete BitplaneShifter_AddData;
  delete BitplaneShifter_Flush;
}
