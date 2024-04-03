#include "ChipTimestamp.h"
#include "FrameParameters.h"
#include <cassert>

bool ChipTimestamp::operator<=(const ChipTimestamp other) const
{
  return Line < other.Line || (Line == other.Line && Cycle <= other.Cycle);
}

bool ChipTimestamp::IsOddCycle() const
{
  return (Cycle & 1) == 1;
}

bool ChipTimestamp::IsEvenCycle() const
{
  return (Cycle & 1) == 0;
}

ChipTimestamp ChipTimestamp::FromMasterTime(const MasterTimestamp masterTime, const FrameParameters &currentFrameParameters)
{
  return ChipTimestamp{
      .Line = masterTime.Cycle / currentFrameParameters.LongLineMasterCycles.Offset,
      .Cycle = masterTime.Cycle % currentFrameParameters.LongLineMasterCycles.Offset,
  };
}

ChipTimestamp ChipTimestamp::FromOriginAndOffset(const ChipTimestamp origin, const ChipTimeOffset offset, const FrameParameters &currentFrameParameters)
{
  uint32_t newLine = origin.Line;
  uint32_t newCycle = origin.Cycle + offset.Offset;

  if (newCycle >= currentFrameParameters.LongLineChipCycles.Offset)
  {
    newLine += newCycle / currentFrameParameters.LongLineChipCycles.Offset;
    newCycle = newCycle % currentFrameParameters.LongLineChipCycles.Offset;
  }

  return ChipTimestamp{.Line = newLine, .Cycle = newCycle};
}

ChipTimestamp ChipTimestamp::FromLineAndCycle(uint32_t line, uint32_t cycle, const FrameParameters &currentFrameParameters)
{
  assert(cycle < currentFrameParameters.LongLineChipCycles.Offset);

  return ChipTimestamp{.Line = line, .Cycle = cycle};
}

void ChipTimestamp::Clear()
{
  Line = 0;
  Cycle = 0;
}