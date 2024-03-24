#include "CiaTimeOffset.h"

CiaTimeOffset CiaTimeOffset::operator-(const CiaTimeOffset other) const
{
  return CiaTimeOffset{.Offset = Offset - other.Offset};
}

CiaTimeOffset CiaTimeOffset::MaskTo16Bits() const
{
  return CiaTimeOffset{.Offset = Offset & 0xffff};
}

bool CiaTimeOffset::IsZero() const
{
  return Offset == 0;
}

CiaTimeOffset CiaTimeOffset::FromValue(uint32_t offset)
{
  return CiaTimeOffset{.Offset = offset};
}
