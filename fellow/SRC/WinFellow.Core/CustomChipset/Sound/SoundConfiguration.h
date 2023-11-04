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

  typedef enum
  {
    SOUND_FILTER_ORIGINAL,
    SOUND_FILTER_ALWAYS,
    SOUND_FILTER_NEVER
  } sound_filters;

  typedef enum
  {
    SOUND_DSOUND_NOTIFICATION,
    SOUND_MMTIMER_NOTIFICATION
  } sound_notifications;
}
