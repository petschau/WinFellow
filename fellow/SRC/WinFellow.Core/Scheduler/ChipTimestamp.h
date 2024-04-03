#pragma once

#include <cstdint>

#include "MasterTimestamp.h"
#include "FrameParameters.h"

struct ChipTimestamp
{
  uint32_t Line;
  uint32_t Cycle;

  bool operator<=(const ChipTimestamp other) const;

  static ChipTimestamp FromOriginAndOffset(const ChipTimestamp timestamp, const ChipTimeOffset offset, const FrameParameters &currentFrameParameters);
  static ChipTimestamp FromLineAndCycle(uint32_t line, uint32_t cycle, const FrameParameters &currentFrameParameters);

  bool IsEvenCycle() const;
  bool IsOddCycle() const;

  void Clear();

  static ChipTimestamp FromMasterTime(const MasterTimestamp masterTime, const FrameParameters &currentFrameParameters);
};
