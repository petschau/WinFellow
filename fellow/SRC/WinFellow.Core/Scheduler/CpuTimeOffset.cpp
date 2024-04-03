#include "CpuTimeOffset.h"

CpuTimeOffset CpuTimeOffset::FromValue(uint32_t offset)
{
  return CpuTimeOffset{.Offset = offset};
}
