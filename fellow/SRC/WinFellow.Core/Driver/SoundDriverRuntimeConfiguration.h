#pragma once

#include <cstdint>
#include "CustomChipset/Sound/SoundConfiguration.h"

struct SoundDriverRuntimeConfiguration
{
  CustomChipset::sound_emulations PlaybackMode;
  CustomChipset::sound_rates PlaybackRate;
  CustomChipset::sound_filters FilterMode;
  CustomChipset::sound_notifications NotificationMode;
  bool IsStereo;
  bool Is16Bits;
  int Volume;
  uint32_t ActualSampleRate;
  uint32_t MaximumBufferSampleCount;
};
