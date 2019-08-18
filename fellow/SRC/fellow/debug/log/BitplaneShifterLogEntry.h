#pragma once

#include "fellow/debug/log/DebugLogEntry.h"

enum class BitplaneShifterLogReasons
{
  BitplaneShifter_output_until,
  BitplaneShifter_duplicate
};

class BitplaneShifterLogEntry : public DebugLogEntry
{
public:
  BitplaneShifterLogReasons Reason;
  uint32_t OutputLine;
  uint32_t StartX;
  uint32_t PixelCount;

  const char *GetSource() const override;
  std::string GetDescription() const override;

  BitplaneShifterLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, BitplaneShifterLogReasons reason, uint32_t outputLine, uint32_t startX, uint32_t pixelCount);
  virtual ~BitplaneShifterLogEntry() = default;
};
