#pragma once

#include <cstdint>

struct ChipTimeOffset
{
  uint32_t Offset;

  static ChipTimeOffset FromValue(uint32_t offset);
};
