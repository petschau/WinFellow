#include "MasterTimestamp.h"

bool MasterTimestamp::operator<(const MasterTimestamp other) const
{
  return Cycle < other.Cycle;
}

bool MasterTimestamp::operator<=(const MasterTimestamp other) const
{
  return Cycle <= other.Cycle;
}

bool MasterTimestamp::operator>=(const MasterTimestamp other) const
{
  return Cycle >= other.Cycle;
}

bool MasterTimestamp::operator==(const MasterTimestamp other) const
{
  return Cycle == other.Cycle;
}

bool MasterTimestamp::operator!=(const MasterTimestamp other) const
{
  return Cycle != other.Cycle;
}

void MasterTimestamp::operator-=(const MasterTimeOffset offset)
{
  Cycle -= offset.Offset;
}

void MasterTimestamp::operator+=(const MasterTimeOffset offset)
{
  Cycle += offset.Offset;
}

MasterTimestamp MasterTimestamp::operator+(const MasterTimeOffset offset) const
{
  return MasterTimestamp{.Cycle = Cycle + offset.Offset};
}

MasterTimeOffset MasterTimestamp::operator-(const MasterTimestamp timestamp) const
{
  return MasterTimeOffset{.Offset = Cycle - timestamp.Cycle};
}

bool MasterTimestamp::IsEnabledAndValid() const
{
  return ((int)Cycle) >= 0;
}

bool MasterTimestamp::IsNowOrLater(const MasterTimestamp now) const
{
  return Cycle >= now.Cycle;
}

void MasterTimestamp::Clear()
{
  Cycle = 0;
}
