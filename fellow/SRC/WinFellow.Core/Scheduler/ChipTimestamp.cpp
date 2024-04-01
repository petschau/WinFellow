#include "ChipTimestamp.h"
#include "FrameParameters.h"

bool ChipTimestamp::operator<=(const ChipTimestamp other) const
{
  return Line < other.Line || (Line == other.Line && Cycle <= other.Cycle);
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

void ChipTimestamp::Clear()
{
  Line = 0;
  Cycle = 0;
}