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

  void Clear();

  static ChipTimestamp FromMasterTime(const MasterTimestamp masterTime, const FrameParameters &currentFrameParameters);
};
