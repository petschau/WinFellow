#pragma once

#include "fellow/debug/log/DebugLogEntry.h"

enum class DIWXLogReasons
{
  DIWX_changed,
  DIWX_start_found,
  DIWX_stop_found,
  DIWX_new_frame
};

class DIWXLogEntry : public DebugLogEntry
{
public:
  DIWXLogReasons Reason;
  uint16_t Start;
  uint16_t Stop;

  const char *GetSource() const override;
  std::string GetDescription() const override;

  DIWXLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, DIWXLogReasons reason, uint16_t start, uint16_t stop);
  virtual ~DIWXLogEntry() = default;
};
