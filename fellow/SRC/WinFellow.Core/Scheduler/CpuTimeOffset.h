#pragma once

#include <cstdint>

struct CpuTimeOffset
{
  uint32_t Offset;

  static CpuTimeOffset FromValue(uint32_t offset);
};
