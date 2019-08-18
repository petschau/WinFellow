#pragma once

#include "fellow/debug/log/DebugLogEntry.h"

enum class DIWYLogReasons
{
  DIWY_start_changed,
  DIWY_stop_changed,
  DIWY_start_found,
  DIWY_stop_found,
  DIWY_new_frame
};

class DIWYLogEntry : public DebugLogEntry
{
public:
  DIWYLogReasons Reason;
  uint16_t Start;
  uint16_t Stop;

  const char *GetSource() const override;
  std::string GetDescription() const override;

  DIWYLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, DIWYLogReasons reason, uint16_t start, uint16_t stop);
  virtual ~DIWYLogEntry() = default;
};
