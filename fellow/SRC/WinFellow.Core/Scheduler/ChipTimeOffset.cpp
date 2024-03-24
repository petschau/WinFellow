#include "ChipTimeOffset.h"

ChipTimeOffset ChipTimeOffset::FromValue(uint32_t offset)
{
  return ChipTimeOffset{.Offset = offset};
}
