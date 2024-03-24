#pragma once

#include <cstdint>
#include "MasterTimestamp.h"

// Cycle unit is shres (35ns)
struct FrameParameters
{
  MasterTimeOffset HorisontalBlankStart;
  MasterTimeOffset HorisontalBlankEnd;
  uint32_t VerticalBlankEnd;
  MasterTimeOffset ShortLineCycles;
  MasterTimeOffset LongLineCycles;
  uint32_t LinesInFrame;
  MasterTimeOffset CyclesInFrame;

  uint32_t GetAgnusCyclesInLine(unsigned int line)
  {
    return LongLineCycles;
  }
};
