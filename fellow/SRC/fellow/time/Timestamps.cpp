#include "fellow/time/Timestamps.h"
#include "fellow/scheduler/Scheduler.h"

constexpr ULL CHIPBUSTIMESTAMP_DISABLE = 0xffffffffffffffff;

ULO ChipBusTimestamp::GetLine() const
{
  return LineAndCycle[0];
}

ULO ChipBusTimestamp::GetCycle() const
{
  return LineAndCycle[1];
}

void ChipBusTimestamp::Clear()
{
  Timestamp = CHIPBUSTIMESTAMP_DISABLE;
}

bool ChipBusTimestamp::HasValue() const
{
  return Timestamp != CHIPBUSTIMESTAMP_DISABLE;
}

void ChipBusTimestamp::Add(UWO cycles)
{
  LineAndCycle[1] += cycles;

  ULO cyclesInLine = scheduler.Get280nsCyclesInLine();
  ULO cycle = GetCycle();
  if (cycle >= cyclesInLine)
  {
    LineAndCycle[0]++;
    LineAndCycle[1] = cycle - cyclesInLine;
  }
}

void ChipBusTimestamp::Set(ULO line, ULO cycle)
{
  LineAndCycle[0] = line;
  LineAndCycle[1] = cycle;
}

void ChipBusTimestamp::operator=(const ChipBusTimestamp &other)
{
  Timestamp = other.Timestamp;
}

bool ChipBusTimestamp::operator==(const ChipBusTimestamp &other) const
{
  return Timestamp == other.Timestamp;
}

bool ChipBusTimestamp::operator>(const ChipBusTimestamp &other) const
{
  return Timestamp > other.Timestamp;
}

ULO ChipBusTimestamp::ToBaseClockTimestamp() const
{
  if (!HasValue())
  {
    return SchedulerEvent::EventDisableCycle;
  }

  return scheduler.GetBaseClockTimestamp(LineAndCycle[0], scheduler.GetCycleFromCycle280ns(LineAndCycle[1]));
}

ULO ChipBusTimestamp::ToBaseClockTimestamp(ULO line, ULO cycle)
{
  return scheduler.GetBaseClockTimestamp(line, scheduler.GetCycleFromCycle280ns(cycle));
}

ULO ChipBusTimestamp::GetCyclesInLine()
{
  return scheduler.Get280nsCyclesInLine();
}

ChipBusTimestamp::ChipBusTimestamp(UWO line, UWO cycle)
{
  LineAndCycle[0] = line;
  LineAndCycle[1] = cycle;
}

ChipBusTimestamp::ChipBusTimestamp(ULO timestamp) : Timestamp(timestamp)
{
}

ChipBusTimestamp::ChipBusTimestamp(const ChipBusTimestamp &timestamp) : Timestamp(timestamp.Timestamp)
{
}

ChipBusTimestamp::ChipBusTimestamp() : Timestamp(CHIPBUSTIMESTAMP_DISABLE)
{
}

void SHResTimestamp::Set(ULO line, ULO pixel)
{
  Line = line;
  Pixel = pixel;
}

ULO SHResTimestamp::GetUnwrappedLine() const
{
  if (Pixel < scheduler.GetHorisontalBlankStart())
  {
    if (Line == 0)
    {
      return scheduler.GetLinesInFrame() - 1;
    }

    return Line - 1;
  }

  return Line;
}

ULO SHResTimestamp::GetUnwrappedPixel() const
{
  if (Pixel < scheduler.GetHorisontalBlankStart())
  {
    return Pixel + scheduler.GetCyclesInLine();
  }

  return Pixel;
}

ULO SHResTimestamp::GetUnwrappedFirstPixelInNextCylinder() const
{
  return (GetUnwrappedPixel() & BUS_CYCLE_140NS_MASK) + BUS_CYCLE_FROM_140NS_MULTIPLIER;
}

void SHResTimestamp::AddWithLineWrap(ULO count)
{
  Pixel += count;

  ULO cyclesInLine = scheduler.GetCyclesInLine();
  if (Pixel >= cyclesInLine)
  {
    Line++;
    Pixel = Pixel - cyclesInLine;
  }
}

bool SHResTimestamp::operator==(const SHResTimestamp &timestamp) const
{
  return Line == timestamp.Line && Pixel == timestamp.Pixel;
}

bool SHResTimestamp::operator<=(const SHResTimestamp &timestamp) const
{
  return (Line < timestamp.Line) || (Line == timestamp.Line && Pixel <= timestamp.Pixel);
}

ULO SHResTimestamp::ToBaseClockTimestamp() const
{
  return scheduler.GetBaseClockTimestamp(Line, Pixel);
}

ULO SHResTimestamp::Cylinder() const
{
  return Pixel / 4;
}

ULO SHResTimestamp::ChipCycle() const
{
  return scheduler.GetCycle280nsFromCycle(Pixel);
}

SHResTimestamp::SHResTimestamp(ULO line, ULO pixel) : Line(line), Pixel(pixel)
{
}

SHResTimestamp::SHResTimestamp(const SHResTimestamp &timestamp) : Line(timestamp.Line), Pixel(timestamp.Pixel)
{
}

SHResTimestamp::SHResTimestamp(const SHResTimestamp &timestamp, ULO addCount) : Line(timestamp.Line), Pixel(timestamp.Pixel)
{
  AddWithLineWrap(addCount);
}
