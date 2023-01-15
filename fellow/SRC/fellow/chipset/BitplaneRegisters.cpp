#include "fellow/chipset/BitplaneRegisters.h"

#include "fellow/api/ChipsetConstants.h"
#include "fellow/chipset/BitplaneUtility.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/chipset/DDFStateMachine.h"
#include "fellow/chipset/DIWXStateMachine.h"
#include "fellow/chipset/DIWYStateMachine.h"
#include "fellow/chipset/HostFrameDelayedRenderer.h"
#include "fellow/chipset/HostFrameImmediateRenderer.h"
#include "fellow/memory/ChipsetBankHandler.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/memory/Memory.h"

// VPOSR - $dff004
// Bit 15 14 13 12 11 10 09 08  07 06 05 04 03 02  01 00
//    LOF I6 I5 I4 I3 I2 II 10 LOL -- -- -- -- V1O V9 V8

// LOF    Long frame = 1, short frame = 0
// I6-I0  Agnus ID is ECS/AGA only
// LOL    Long/short line
// V10-V8 Vertical position high bits, V10 and V9 is ECS/AGA only

// Agnus/Alice IDs
// 8361/8370 (NTSC original or fat) = 0x10
// 8367/8371 (PAL original or fat) = 0x00
// 8372 (NTSC until rev4) = 0x30
// 8372 (PAL until rev4) = 0x20
// 8372 (NTSC rev5) = 0x31
// 8372 (PAL rev5) = 0x21
// 8374 (NTSC until rev2) = 0x32
// 8374 (PAL until rev2) = 0x22
// 8374 (NTSC rev3 and rev4) = 0x33
// 8374 (PAL rev3 and rev4) = 0x23

UWO BitplaneRegisters::GetVPosR() const
{
  UWO value = lof;
  if (chipsetGetECS())
  {
    value |= GetAgnusId() << 8;
    value |= (_scheduler->GetRasterY() >> 8) & 0x7;
  }
  else // OCS
  {
    value |= (_scheduler->GetRasterY() >> 8) & 1;
  }
  return value;
}

// VHPOSR - $dff006
// Bit 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//     V7 V6 V5 V4 V3 V2 V1 V0 H8 H7 H6 H5 H4 H3 H2 H1

// H8-H1 Horisontal raster position. Has bus cycle clock resolution, for instance range 0x00-0xe2 for standard PAL mode
// V7-V0 Vertical raster position
UWO BitplaneRegisters::GetVHPosR()
{
  return (UWO)((_scheduler->GetRasterX() >> 3) | ((_scheduler->GetRasterY() & 0x000000FF) << 8));
}

// VPOSW - $dff02a, see VPOSR for bits
void BitplaneRegisters::SetVPos(UWO data)
{
  lof = (UWO)(data & 0x8000);
  // TODO: Add changing of raster position counters
}

// VHPOSW - $dff02c, see VHPOSR for bits
void BitplaneRegisters::SetVHPos(UWO data)
{
  // TODO: Add changing of raster position counters
}

UWO BitplaneRegisters::GetAgnusId()
{
  return chipsetGetECS() ? AgnusId::Agnus_8372_ECS_PAL_until_Rev4 : AgnusId::Agnus_8371_Fat_PAL;
}

// DENISEID - $dff006 (sometimes called LISAID)
// Bit 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//     -- -- -- -- -- -- -- -- I7 I6 I5 I4 I3 I2 I1 I0

// Known values:
// 0xfc - Denise 8373 (ECS)
// 0xf8 - Lisa (AGA)

// OCS Denise 8362 does not have this register
UWO BitplaneRegisters::GetDeniseId()
{
  return DeniseId::Denise_8373_ECS;
}

UWO BitplaneRegisters::GetVerticalPosition()
{
  return (UWO)_scheduler->GetRasterY();
}

UWO BitplaneRegisters::GetHorisontalPosition()
{
  return (UWO)_scheduler->GetRasterX();
}

void BitplaneRegisters::ResetDiwhigh()
{
  diwhigh = (~diwstop >> 7) & 0x100;
}

// DIWSTRT - $dff08e
// OCS:
// Bit 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//     V7 V6 V5 V4 V3 V2 V1 V0 H7 H6 H5 H4 H3 H2 H1 HO
//
// H8 cannot be set, it is written as 0
// V8 cannot be set, it is written as 0
// Horisontal resolution is lores color clocks
//
// ECS:
// Bit 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//     V7 V6 V5 V4 V3 V2 VI V0 H9 H8 H7 H6 H5 H4 H3 H2
//
// Horisontal resolution accessible on ECS through this register is still lores color clocks and limited to the left part of the screen.
// H10, H1 and H0 can be updated through DIWHIGH for superhires color clock accuracy and full width.
// The inaccessible bits are still initially written as 0. (TODO: Verify this)
//
// Vertical resolution accessible on ECS through this register is still limited to the upper part of the screen.
// V10, V9 and V8 can be updated through DIWHIGH for full height.
// The inaccessible bits are still initially written as 0. (TODO: Verify this)
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

// DIWSTOP - $dff090
// OCS:
// Bit 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//     V7 V6 V5 V4 V3 V2 V1 V0 H7 H6 H5 H4 H3 H2 H1 HO
//
// H8 cannot be set, it is written as 1
// V8 cannot be set, it is written as 1
// Horisontal resolution is lores color clocks
//
// ECS:
// Bit 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//     V7 V6 V5 V4 V3 V2 VI V0 H9 H8 H7 H6 H5 H4 H3 H2
//
// Horisontal resolution accessible on ECS through this register is still lores color clocks and limited to the right part of the screen.
// H10, H1 and H0 can be updated through DIWHIGH for superhires color clock accuracy and full width.
// H10 is still initially written as 1, and H1,H0 is written as 0. (TODO: Verify this)
//
// Vertical resolution accessible on ECS through this register is still limited to the lower part of the screen.
// V10, V9 and V8 can be updated through DIWHIGH for full height.
// V8 is still initially written as 1, and V10,V9 is written as 0. (TODO: Verify this)
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

// DDFSTRT - $dff092
// OCS:
// Bit 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//     -- -- -- -- -- -- -- -- H8 H7 H6 H5 H4 H4 -- --
//
// ECS:
// Bit 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//     -- -- -- -- -- -- -- -- H8 H7 H6 H5 H4 H4 H3 --
//
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

// DDFSTOP - $dff094
// OCS:
// Bit 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//     -- -- -- -- -- -- -- -- H8 H7 H6 H5 H4 H4 -- --
//
// ECS:
// Bit 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//     -- -- -- -- -- -- -- -- H8 H7 H6 H5 H4 H4 H3 --
//
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

// BPLxPTH - $dff0e0,$dff0e4,$dff0e8,$dff0ec,$dff0f0,$dff0f4
// OCS:
// Bit 15 14 13 12 11 10 09 08 07 06 05 04 03 02  01 00
//     -- -- -- -- -- -- -- -- -- -- -- -- -- A10 A9 A8
//
// ECS:
// Bit 15 14 13 12 11 10 09 08 07 06 05 04  03  02  01 00
//     -- -- -- -- -- -- -- -- -- -- -- A12 A11 A10 A9 A8
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

// BPLxPTH - $dff0e2,$dff0e6,$dff0ea,$dff0ee,$dff0f2,$dff0f6
// OCS and ECS:
// Bit 15  14  13  12  11  10  09 08 07 06 05 04 03 02 01 00
//     A15 A14 A13 A12 A11 A10 A9 A8 A7 A6 A5 A4 A3 A2 A1 --
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
    IsLores = (bplcon0 & 0x8000) == 0;
    IsHires = !IsLores;
    IsDualPlayfield = (bplcon0 & 0x0400) == 0x0400;
    IsHam = (bplcon0 & 0x0800) == 0x0800;
    EnabledBitplaneCount = (bplcon0 >> 12) & 7;

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
      const SHResTimestamp &shresTimestamp = _scheduler->GetSHResTimestamp();
      ULO line = shresTimestamp.GetUnwrappedLine();
      bool isOutsideVerticalBlank = line >= _scheduler->GetVerticalBlankEnd();

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
          host_frame_delayed_renderer.ChangeList.AddColorChange(
              line, shresTimestamp.GetUnwrappedFirstPixelInNextCylinder(), index, newColor, newHalfbriteColor);
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

void BitplaneRegisters::InstallIOHandlers(ChipsetBankHandler &chipsetBankHandler, bool isECS)
{
  chipsetBankHandler.SetIoReadFunction(0x004, [this](uint32_t address) -> uint16_t { return this->GetVPosR(); });
  chipsetBankHandler.SetIoReadFunction(0x006, [this](uint32_t address) -> uint16_t { return this->GetVHPosR(); });

  chipsetBankHandler.SetIoWriteFunction(0x02a, [this](uint16_t data, uint32_t address) -> void { this->SetVPos(data); });
  chipsetBankHandler.SetIoWriteFunction(0x02c, [this](uint16_t data, uint32_t address) -> void { this->SetVHPos(data); });
  chipsetBankHandler.SetIoWriteFunction(0x08e, [this](uint16_t data, uint32_t address) -> void { this->SetDIWStrt(data); });
  chipsetBankHandler.SetIoWriteFunction(0x090, [this](uint16_t data, uint32_t address) -> void { this->SetDIWStop(data); });
  chipsetBankHandler.SetIoWriteFunction(0x092, [this](uint16_t data, uint32_t address) -> void { this->SetDDFStrt(data); });
  chipsetBankHandler.SetIoWriteFunction(0x094, [this](uint16_t data, uint32_t address) -> void { this->SetDDFStop(data); });

  for (unsigned int i = 0; i < 6; i++)
  {
    chipsetBankHandler.SetIoWriteFunction(0x0e0 + i * 4, [this, i](uint16_t data, uint32_t address) -> void { this->SetBplPtHigh(i, data); });
    chipsetBankHandler.SetIoWriteFunction(0x0e2 + i * 4, [this, i](uint16_t data, uint32_t address) -> void { this->SetBplPtLow(i, data); });
  }

  chipsetBankHandler.SetIoWriteFunction(0x100, [this](uint16_t data, uint32_t address) -> void { this->SetBplCon0(data); });
  chipsetBankHandler.SetIoWriteFunction(0x102, [this](uint16_t data, uint32_t address) -> void { this->SetBplCon1(data); });
  chipsetBankHandler.SetIoWriteFunction(0x104, [this](uint16_t data, uint32_t address) -> void { this->SetBplCon2(data); });
  chipsetBankHandler.SetIoWriteFunction(0x108, [this](uint16_t data, uint32_t address) -> void { this->SetBpl1Mod(data); });
  chipsetBankHandler.SetIoWriteFunction(0x10a, [this](uint16_t data, uint32_t address) -> void { this->SetBpl2Mod(data); });

  for (unsigned int i = 0; i < 6; i++)
  {
    chipsetBankHandler.SetIoWriteFunction(0x110 + i * 2, [this, i](uint16_t data, uint32_t address) -> void { this->SetBplDat(i, data); });
  }

  for (unsigned int i = 0; i < 32; i++)
  {
    chipsetBankHandler.SetIoWriteFunction(0x180 + i * 2, [this, i](uint16_t data, uint32_t address) -> void { this->SetColor(i, data); });
  }

  if (isECS)
  {
    chipsetBankHandler.SetIoReadFunction(0x07c, [this](uint32_t address) -> uint16_t { return this->GetDeniseId(); });

    chipsetBankHandler.SetIoWriteFunction(0x1c0, [this](uint16_t data, uint32_t address) -> void { this->SetHTotal(data); });
    chipsetBankHandler.SetIoWriteFunction(0x1c2, [this](uint16_t data, uint32_t address) -> void { this->SetHSStop(data); });
    chipsetBankHandler.SetIoWriteFunction(0x1c4, [this](uint16_t data, uint32_t address) -> void { this->SetHBStrt(data); });
    chipsetBankHandler.SetIoWriteFunction(0x1c6, [this](uint16_t data, uint32_t address) -> void { this->SetHBStop(data); });
    chipsetBankHandler.SetIoWriteFunction(0x1c8, [this](uint16_t data, uint32_t address) -> void { this->SetVTotal(data); });
    chipsetBankHandler.SetIoWriteFunction(0x1ca, [this](uint16_t data, uint32_t address) -> void { this->SetVSStop(data); });
    chipsetBankHandler.SetIoWriteFunction(0x1cc, [this](uint16_t data, uint32_t address) -> void { this->SetVBStrt(data); });
    chipsetBankHandler.SetIoWriteFunction(0x1ce, [this](uint16_t data, uint32_t address) -> void { this->SetVBStop(data); });
    chipsetBankHandler.SetIoWriteFunction(0x1dc, [this](uint16_t data, uint32_t address) -> void { this->SetBeamcon0(data); });
    chipsetBankHandler.SetIoWriteFunction(0x1de, [this](uint16_t data, uint32_t address) -> void { this->SetHSStrt(data); });
    chipsetBankHandler.SetIoWriteFunction(0x1e0, [this](uint16_t data, uint32_t address) -> void { this->SetVSStrt(data); });
    chipsetBankHandler.SetIoWriteFunction(0x1e2, [this](uint16_t data, uint32_t address) -> void { this->SetHCenter(data); });
    chipsetBankHandler.SetIoWriteFunction(0x1e4, [this](uint16_t data, uint32_t address) -> void { this->SetDIWHigh(data); });
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

void BitplaneRegisters::Initialize(Scheduler *scheduler)
{
  _scheduler = scheduler;
  ClearState();
}

BitplaneRegisters::BitplaneRegisters() : BitplaneShifter_AddData(nullptr), BitplaneShifter_Flush(nullptr), _colorChangeEventHandler(nullptr)
{
}

BitplaneRegisters::~BitplaneRegisters()
{
  delete BitplaneShifter_AddData;
  delete BitplaneShifter_Flush;
}
