#pragma once

#include <cstdint>
#include "CustomChipset/Sound/SoundConfiguration.h"

struct SoundDriverRuntimeConfiguration
{
  sound_emulations PlaybackMode;
  sound_rates PlaybackRate;
  sound_filters FilterMode;
  sound_notifications NotificationMode;
  bool IsStereo;
  bool Is16Bits;
  int Volume;
  uint32_t ActualSampleRate;
  uint32_t MaximumBufferSampleCount;
};
