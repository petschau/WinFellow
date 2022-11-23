#pragma once

#include <cstdint>
#include <stddef.h>

struct GfxDrvMappedBufferPointer
{
  uint8_t *Buffer;
  ptrdiff_t Pitch;
  bool IsValid;
};
