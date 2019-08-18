#pragma once

#include <stdint.h>
#include <string>
#include "fellow/time/Timestamps.h"

enum class DebugLogSource
{
  CHIP_BUS_BITPLANE_DMA,
  CHIP_BUS_COPPER_DMA,
  CHIP_BUS_SPRITE_DMA,
  DIWX_ACTION,
  DIWY_ACTION,
  BITPLANESHIFTER_ACTION,
  EVENT_QUEUE
};

class DebugLogEntry
{
public:
  DebugLogSource Source;
  uint64_t FrameNumber;
  SHResTimestamp Timestamp;

  virtual const char *GetSource() const = 0;
  virtual std::string GetDescription() const = 0;

  DebugLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp);
};
