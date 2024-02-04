#pragma once

#include <cstdint>

struct FrameParameters
{
  uint32_t HorisontalBlankStart;
  uint32_t HorisontalBlankEnd;
  uint32_t VerticalBlankEnd;
  uint32_t ShortLineCycles;
  uint32_t LongLineCycles;
  uint32_t LinesInFrame;
  uint32_t CyclesInFrame;

  uint32_t GetAgnusCyclesInLine(unsigned int line)
  {
    return LongLineCycles;
  }
};
