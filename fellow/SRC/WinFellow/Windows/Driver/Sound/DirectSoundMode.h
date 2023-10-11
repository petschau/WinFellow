#pragma once

#include <cstdint>

struct DirectSoundMode
{
  uint32_t Rate;
  bool Is16Bits;
  bool IsStereo;
  uint32_t BufferSampleCount;
  uint32_t BufferBlockAlign;
};
