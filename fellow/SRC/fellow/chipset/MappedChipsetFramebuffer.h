#pragma once

#include <cstdint>

struct MappedChipsetFramebuffer
{
  uint8_t *TopPointer;
  ptrdiff_t HostLinePitch;
  ptrdiff_t AmigaLinePitch;
  bool IsValid;
};