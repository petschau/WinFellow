#pragma once

#include <cstdint>

struct GfxDrvMappedBufferPointer
{
  uint8_t *Buffer;
  ptrdiff_t Pitch;
  bool IsValid;
};
