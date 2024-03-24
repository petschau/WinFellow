#pragma once

#include <cstdint>

#include "MasterTimeOffset.h"

struct MasterTimestamp
{
  uint32_t Cycle;

  bool operator<(const MasterTimestamp other) const;
  bool operator<=(const MasterTimestamp other) const;
  bool operator>=(const MasterTimestamp other) const;
  bool operator==(const MasterTimestamp other) const;
  bool operator!=(const MasterTimestamp other) const;
  void operator-=(const MasterTimeOffset offset);
  void operator+=(const MasterTimeOffset offset);
  MasterTimestamp operator+(const MasterTimeOffset offset) const;
  MasterTimeOffset operator-(const MasterTimestamp timestamp) const;

  bool IsEnabledAndValid() const;
  bool IsNowOrLater(const MasterTimestamp now) const;

  void Clear();
};
