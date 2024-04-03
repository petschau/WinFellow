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

  bool IsEnabled() const;
  bool IsEnabledAndValid() const;
  bool IsNowOrLater(const MasterTimestamp now) const;

  static MasterTimestamp FromValue(uint32_t cycle);

  void Clear();
};
