#pragma once

#include <cstdint>
#include "MasterTimestamp.h"

// Cycle unit is shres (35ns)
struct FrameParameters
{
  MasterTimeOffset HorisontalBlankStart;
  MasterTimeOffset HorisontalBlankEnd;
  uint32_t VerticalBlankEnd;

  MasterTimeOffset ShortLineMasterCycles;
  MasterTimeOffset LongLineMasterCycles;

  ChipTimeOffset ShortLineChipCycles;
  ChipTimeOffset LongLineChipCycles;

  uint32_t LinesInFrame;
  MasterTimeOffset CyclesInFrame;
};
