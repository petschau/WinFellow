#pragma once

#include <cstdint>

#include "ChipTimeOffset.h"
#include "CiaTimeOffset.h"

struct MasterTimeOffset
{
  uint32_t Offset;

  static MasterTimeOffset FromChipTimeOffset(const ChipTimeOffset chipTimeOffset);
  static MasterTimeOffset FromCiaTimeOffset(const CiaTimeOffset ciaTimeOffset);
  static MasterTimeOffset FromValue(uint32_t offset);
  CiaTimeOffset ToCiaTimeOffset() const;
  MasterTimeOffset GetCiaTimeRemainder() const;
};
