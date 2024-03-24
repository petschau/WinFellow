#pragma once

#include <cstdint>

struct CiaTimeOffset
{
  uint32_t Offset;

  CiaTimeOffset operator-(const CiaTimeOffset other) const;
  CiaTimeOffset MaskTo16Bits() const;
  bool IsZero() const;
  static CiaTimeOffset FromValue(uint32_t offset);
};
