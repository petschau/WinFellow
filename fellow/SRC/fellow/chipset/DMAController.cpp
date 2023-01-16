#include "fellow/chipset/DMAController.h"
#include "fellow/chipset/BitplaneDMA.h"
#include "fellow/chipset/SpriteDMA.h"
#include "fellow/chipset/CycleExactCopper.h"
#include "fellow/memory/Memory.h"
#include "fellow/scheduler/Scheduler.h"

DMAChannel::DMAChannel() : ReadCallback(nullptr), WriteCallback(nullptr), NullCallback(nullptr), IsRequested(false), Mode(DMAChannelMode::ReadMode), Pt(0), Value(0)
{
}

DebugLogSource DMAController::MapDMAChannelTypeToDebugLogSource(DMAChannelType type) const
{
  switch (type)
  {
    case DMAChannelType::BitplaneChannel: return DebugLogSource::CHIP_BUS_BITPLANE_DMA;
    case DMAChannelType::CopperChannel: return DebugLogSource::CHIP_BUS_COPPER_DMA;
    case DMAChannelType::SpriteChannel: return DebugLogSource::CHIP_BUS_SPRITE_DMA;
  }
}

DebugLogChipBusMode DMAController::MapDMAChannelModeToDebugLogChipBusMode(DMAChannelMode mode) const
{
  switch (mode)
  {
    case DMAChannelMode::ReadMode: return DebugLogChipBusMode::Read;
    case DMAChannelMode::WriteMode: return DebugLogChipBusMode::Write;
    case DMAChannelMode::NullMode: return DebugLogChipBusMode::Null;
  }
}

void DMAController::AddLogEntry(const DMAChannel *operation, UWO value)
{
  if (DebugLog.Enabled)
  {
    DebugLog.AddChipBusLogEntry(
        MapDMAChannelTypeToDebugLogSource(operation->Source),
        _scheduler->GetRasterFrameCount(),
        _scheduler->GetSHResTimestamp(),
        MapDMAChannelModeToDebugLogChipBusMode(operation->Mode),
        operation->Pt,
        value,
        operation->Channel);
  }
}

void DMAController::Execute(const DMAChannel *operation)
{
  switch (operation->Mode)
  {
    case DMAChannelMode::ReadMode:
    {
      const UWO value = chipmemReadWord(operation->Pt);
      AddLogEntry(operation, value);
      operation->ReadCallback(operation->Timestamp, value);
      break;
    }
    case DMAChannelMode::WriteMode:
    {
      AddLogEntry(operation, operation->Value);
      chipmemWriteWord(operation->Value, operation->Pt);
      operation->WriteCallback();
      break;
    }
    case DMAChannelMode::NullMode:
    {
      AddLogEntry(operation, operation->Value);
      operation->NullCallback(operation->Timestamp);
      break;
    }
  }
}

void DMAController::Clear()
{
  _bitplane.IsRequested = false;
  _copper.IsRequested = false;
  _sprite.IsRequested = false;

  UpdateNextTimeFullScan();
}

void DMAController::UpdateNextTimeFullScan()
{
  _nextTime.Clear();

  if (_bitplane.IsRequested)
  {
    _nextTime = _bitplane.Timestamp;
  }

  if (_sprite.IsRequested && (!_nextTime.HasValue() || _nextTime > _sprite.Timestamp))
  {
    _nextTime = _sprite.Timestamp;
  }

  if (_copper.IsRequested && (!_nextTime.HasValue() || _nextTime > _copper.Timestamp))
  {
    _nextTime = _copper.Timestamp;
  }

  SetEventTime();
}

void DMAController::UpdateNextTime(const ChipBusTimestamp &timestamp)
{
  if (!_nextTime.HasValue() || _nextTime > timestamp)
  {
    _nextTime = timestamp;
    SetEventTime();
  }
}

void DMAController::SetEventTime() const
{
  if (UseEventQueue && chipBusAccessEvent.IsEnabled())
  {
    _scheduler->RemoveEvent(&chipBusAccessEvent);
  }

  chipBusAccessEvent.cycle = _nextTime.ToBaseClockTimestamp();

  F_ASSERT((int)chipBusAccessEvent.cycle >= -1);

  if (UseEventQueue && chipBusAccessEvent.IsEnabled())
  {
    _scheduler->InsertEvent(&chipBusAccessEvent);
  }
}

void DMAController::ScheduleBitplaneDMA(const ChipBusTimestamp &timestamp, ULO pt, ULO channel)
{
  _bitplane.Timestamp = timestamp;
  _bitplane.IsRequested = true;
  _bitplane.Pt = pt;
  _bitplane.Channel = channel;

  UpdateNextTime(_bitplane.Timestamp);
}

void DMAController::ScheduleCopperDMARead(const ChipBusTimestamp &timestamp, ULO pt)
{
  _copper.OriginalTimestamp = timestamp;
  _copper.Timestamp = timestamp;
  _copper.IsRequested = true;
  _copper.Mode = DMAChannelMode::ReadMode;
  _copper.Pt = pt;

  UpdateNextTime(_copper.Timestamp);
}

void DMAController::ScheduleCopperDMANull(const ChipBusTimestamp &timestamp)
{
  bool doFullScan = _copper.IsRequested && _nextTime == _copper.Timestamp;

  _copper.OriginalTimestamp = timestamp;
  _copper.Timestamp = timestamp;
  _copper.IsRequested = true;
  _copper.Mode = DMAChannelMode::NullMode;

  if (doFullScan)
  {
    UpdateNextTimeFullScan();
  }
  else
  {
    UpdateNextTime(_copper.Timestamp);
  }
}

void DMAController::RemoveCopperDMA()
{
  if (_copper.IsRequested)
  {
    _copper.IsRequested = false;

    if (_nextTime == _copper.Timestamp)
    {
      UpdateNextTimeFullScan();
    }
  }
}

void DMAController::ScheduleSpriteDMA(const ChipBusTimestamp &timestamp, ULO pt, ULO channel)
{
  _sprite.Timestamp = timestamp;
  _sprite.IsRequested = true;
  _sprite.Pt = pt;
  _sprite.Channel = channel;

  UpdateNextTime(_sprite.Timestamp);
}

DMAChannel *DMAController::GetPrioritizedDMAChannel()
{
  if (_bitplane.IsRequested && _nextTime == _bitplane.Timestamp)
  {
    // If bitplane is requested, do that.

    // Check if a sprite was kicked out
    if (_sprite.IsRequested && _nextTime == _sprite.Timestamp)
    {
      // This stops the sprite DMA sequence for the remainder of the line
      _sprite.IsRequested = false;
    }

    // Maybe delay copper
    if (_copper.IsRequested && _nextTime == _copper.Timestamp)
    {
      _copper.Timestamp.Add(2);
      if (_refreshCycle[_copper.Timestamp.GetCycle()]) // ie. last cycle in a line with odd number of cycles
      {
        _copper.Timestamp.Add(1);
      }
    }

    return &_bitplane;
  }

  if (_copper.IsRequested && _nextTime == _copper.Timestamp)
  {
    return &_copper;
  }

  if (_sprite.IsRequested && _nextTime == _sprite.Timestamp)
  {
    return &_sprite;
  }

  return nullptr;
}

void DMAController::HandleEvent()
{
  dma_controller.Handler();
}

void DMAController::Handler()
{
  chipBusAccessEvent.Disable();

  DMAChannel *dmaChannel = GetPrioritizedDMAChannel();
  dmaChannel->IsRequested = false;
  UpdateNextTimeFullScan();
  Execute(dmaChannel);
}

void DMAController::InitializeRefreshCycleTable()
{
  for (bool &i : _refreshCycle)
  {
    i = false;
  }

  _refreshCycle[1] = true;
  _refreshCycle[3] = true;
  _refreshCycle[5] = true;
  _refreshCycle[226] = true;
}

/* Fellow events */

void DMAController::EndOfFrame()
{
  if (UseEventQueue)
  {
    _scheduler->RebaseForNewFrame(&chipBusAccessEvent);
  }
}

void DMAController::SoftReset()
{
}

void DMAController::HardReset()
{
  Clear();
}

void DMAController::EmulationStart()
{
}

void DMAController::EmulationStop()
{
}

void DMAController::Startup()
{
}

void DMAController::Shutdown()
{
}

DMAController::DMAController(Scheduler *scheduler, BitplaneDMA *bitplaneDMA) : _scheduler(scheduler), _bitplaneDMA(bitplaneDMA)
{
  UseEventQueue = true;

  _bitplane.ReadCallback = [bitplaneDMA](const ChipBusTimestamp &currentTime, uint16_t value) -> void { bitplaneDMA->DMAReadCallback(currentTime, value); };
  _bitplane.Mode = DMAChannelMode::ReadMode;
  _bitplane.Source = DMAChannelType::BitplaneChannel;

  _copper.ReadCallback = CycleExactCopper::DMAReadCallback;
  _copper.NullCallback = CycleExactCopper::DMANullCallback;
  _copper.Mode = DMAChannelMode::ReadMode;
  _copper.Source = DMAChannelType::CopperChannel;

  _sprite.Mode = DMAChannelMode::ReadMode;
  _sprite.Source = DMAChannelType::SpriteChannel;
  _sprite.ReadCallback = SpriteDMA::DMAReadCallback;

  InitializeRefreshCycleTable();
}
