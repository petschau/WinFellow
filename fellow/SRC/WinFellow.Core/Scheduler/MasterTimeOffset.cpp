#include "MasterTimeOffset.h"
#include "ClockConstants.h"

MasterTimeOffset MasterTimeOffset::FromChipTimeOffset(const ChipTimeOffset chipTimeOffset)
{
  return MasterTimeOffset{.Offset = chipTimeOffset.Offset << ClockConstants::MasterTimeOffsetFromChipTimeOffsetShift};
}

MasterTimeOffset MasterTimeOffset::FromCiaTimeOffset(const CiaTimeOffset ciaTimeOffset)
{
  return MasterTimeOffset{.Offset = ciaTimeOffset.Offset * ClockConstants::MasterTimeCyclesInCiaTimeCycle};
}

CiaTimeOffset MasterTimeOffset::ToCiaTimeOffset() const
{
  return CiaTimeOffset{.Offset = Offset / ClockConstants::MasterTimeCyclesInCiaTimeCycle};
}

MasterTimeOffset MasterTimeOffset::GetCiaTimeRemainder() const
{
  return MasterTimeOffset{.Offset = Offset % ClockConstants::MasterTimeCyclesInCiaTimeCycle};
}

MasterTimeOffset MasterTimeOffset::FromValue(uint32_t offset)
{
  return MasterTimeOffset{.Offset = offset};
}