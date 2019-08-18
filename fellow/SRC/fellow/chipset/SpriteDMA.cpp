#include "fellow/chipset/SpriteDMA.h"
#include "fellow/chipset/SpriteRegisters.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/chipset/BitplaneUtility.h"
#include "fellow/chipset/DMAController.h"
#include "fellow/scheduler/Scheduler.h"

SpriteDMA sprite_dma;

bool SpriteDMAChannel::WantsData() const
{
  return ReadData || ReadControl;
}

void SpriteDMA::DMAReadCallback(const ChipBusTimestamp &currentTime, UWO value)
{
  sprite_dma.ReadCallback(currentTime, value);
}

void SpriteDMA::ReadCallback(const ChipBusTimestamp &currentTime, UWO value)
{
  if (_channel[_currentChannel].ReadData)
  {
    ReadDataWord(_currentChannel, currentTime, value);
  }
  else if (_channel[_currentChannel].ReadControl)
  {
    ReadControlWord(_currentChannel, currentTime, value);
  }
}

void SpriteDMA::HandleEvent()
{
  sprite_dma.Handle();
}

void SpriteDMA::Handle()
{
  spriteDMAEvent.Disable();

  if (!BitplaneUtility::IsSpriteDMAEnabled())
  {
    // Don't schedule any more events on this line
    return;
  }

  ULO line = scheduler.GetRasterY();

  UpdateSpriteDMAState(_currentChannel, line);

  if (_channel[_currentChannel].WantsData())
  {
    _channel[_currentChannel].ReadFirstWord = true;
    dma_controller.ScheduleSpriteDMA(ChipBusTimestamp(line, scheduler.GetChipBusTimestamp().GetCycle() + 1), sprite_registers.sprpt[_currentChannel], _currentChannel);
  }
  else if (_currentChannel < 7)
  {
    _currentChannel++;
    ScheduleEvent(scheduler.GetBaseClockTimestamp(line, scheduler.GetRasterX() + scheduler.GetCycleFromCycle280ns(4)));
  }
}

bool SpriteDMA::IsInVerticalBlank(ULO line)
{
  return line < (scheduler.GetVerticalBlankEnd() - 1);
}

bool SpriteDMA::IsFirstLineAfterVerticalBlank(ULO line)
{
  return line == (scheduler.GetVerticalBlankEnd() - 1);
}

void SpriteDMA::UpdateSpriteDMAState(unsigned int spriteNumber, ULO line)
{
  if (IsFirstLineAfterVerticalBlank(line) || line == _channel[spriteNumber].VStop)
  {
    _channel[spriteNumber].ReadData = false;
    _channel[spriteNumber].ReadControl = true;
    return;
  }

  _channel[spriteNumber].ReadControl = false;

  if (line == _channel[spriteNumber].VStart)
  {
    _channel[spriteNumber].ReadData = true;
  }
}

void SpriteDMA::SetVStartLow(unsigned int spriteNumber, UWO vstartLow)
{
  _channel[spriteNumber].VStart = (_channel[spriteNumber].VStart & 0x100) | vstartLow;
}

void SpriteDMA::SetVStartHigh(unsigned int spriteNumber, UWO vstartHigh)
{
  _channel[spriteNumber].VStart = vstartHigh | (_channel[spriteNumber].VStart & 0xff);
}

void SpriteDMA::SetVStop(unsigned int spriteNumber, UWO vstop)
{
  _channel[spriteNumber].VStop = vstop;
}

void SpriteDMA::ReadControlWord(unsigned int spriteNumber, const ChipBusTimestamp &currentTime, UWO value)
{
  unsigned int offset = spriteNumber * 8;

  IncreasePtr(spriteNumber);

  ChipBusTimestamp nextTime(currentTime.GetLine(), currentTime.GetCycle() + 2);

  if (_channel[spriteNumber].ReadFirstWord)
  {
    wsprxpos(value, 0x140 + offset);
    _channel[spriteNumber].ReadFirstWord = false;

    dma_controller.ScheduleSpriteDMA(nextTime, sprite_registers.sprpt[spriteNumber], spriteNumber);
  }
  else
  {
    wsprxctl(value, 0x142 + offset);

    if (_currentChannel < 7)
    {
      _currentChannel++;
      ScheduleEvent(nextTime.ToBaseClockTimestamp() - 1);
    }
  }
}

void SpriteDMA::ReadDataWord(unsigned int spriteNumber, const ChipBusTimestamp &currentTime, UWO value)
{
  unsigned int offset = spriteNumber * 8;

  IncreasePtr(spriteNumber);

  ChipBusTimestamp nextTime(currentTime.GetLine(), currentTime.GetCycle() + 2);

  if (_channel[spriteNumber].ReadFirstWord)
  {
    wsprxdatb(value, 0x146 + offset);
    _channel[spriteNumber].ReadFirstWord = false;

    dma_controller.ScheduleSpriteDMA(nextTime, sprite_registers.sprpt[spriteNumber], spriteNumber);
  }
  else
  {
    wsprxdata(value, 0x144 + offset);

    if (_currentChannel < 7)
    {
      _currentChannel++;
      ScheduleEvent(nextTime.ToBaseClockTimestamp() - 1);
    }
  }
}

void SpriteDMA::IncreasePtr(unsigned int spriteNumber)
{
  sprite_registers.sprpt[spriteNumber] = chipsetMaskPtr(sprite_registers.sprpt[spriteNumber] + 2);
}

void SpriteDMA::ScheduleEvent(ULO baseClockTimestamp)
{
  spriteDMAEvent.cycle = baseClockTimestamp;
  scheduler.InsertEvent(&spriteDMAEvent);
}

void SpriteDMA::InitiateDMASequence(ULO line)
{
  _currentChannel = 0;
  ScheduleEvent(scheduler.GetBaseClockTimestamp(line, InitialBaseClockCycleForDMACheck));
}

void SpriteDMA::EndOfLine()
{
  ULO nextLine = scheduler.GetRasterY() + 1;

  if (!IsInVerticalBlank(nextLine))
  {
    InitiateDMASequence(nextLine);
  }
}

void SpriteDMA::EndOfFrame()
{
  if (spriteDMAEvent.IsEnabled())
  {
    scheduler.RemoveEvent(&spriteDMAEvent);
    spriteDMAEvent.Disable();
  }
}

void SpriteDMA::Clear()
{
  for (auto &i : _channel)
  {
    i.ReadControl = i.ReadData = false;
  }
}

SpriteDMA::SpriteDMA() : InitialBaseClockCycleForDMACheck(scheduler.GetCycleFromCycle280ns(0x15) - 1)
{
}
