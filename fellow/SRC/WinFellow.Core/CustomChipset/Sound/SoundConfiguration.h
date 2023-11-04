#pragma once

namespace CustomChipset
{
  enum class sound_rates
  {
    SOUND_15650,
    SOUND_22050,
    SOUND_31300,
    SOUND_44100
  };

  enum class sound_emulations
  {
    SOUND_NONE,
    SOUND_PLAY,
    SOUND_EMULATE
  };

  enum class sound_filters
  {
    SOUND_FILTER_ORIGINAL,
    SOUND_FILTER_ALWAYS,
    SOUND_FILTER_NEVER
  };

  enum class sound_notifications
  {
    SOUND_DSOUND_NOTIFICATION,
    SOUND_MMTIMER_NOTIFICATION
  };
}
