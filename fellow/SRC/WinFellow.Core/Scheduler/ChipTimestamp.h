#pragma once

#include <cstdint>

#include "MasterTimestamp.h"
#include "FrameParameters.h"

struct ChipTimestamp
{
  uint32_t Line;
  uint32_t Cycle;

  void Clear();

  static ChipTimestamp FromMasterTime(const MasterTimestamp masterTime, const FrameParameters &currentFrameParameters);
};
