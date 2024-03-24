#include "ChipTimestamp.h"
#include "FrameParameters.h"

ChipTimestamp ChipTimestamp::FromMasterTime(const MasterTimestamp masterTime, const FrameParameters& currentFrameParameters)
{
  return ChipTimestamp{
      .Line = masterTime.Cycle / currentFrameParameters.LongLineCycles,
      .Cycle = masterTime.Cycle % currentFrameParameters.LongLineCycles,
  };
}

void ChipTimestamp::Clear()
{
  Line = 0;
  Cycle = 0;
}