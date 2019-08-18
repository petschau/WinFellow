#pragma once

#include "fellow/debug/log/DebugLogEntry.h"

enum class EventQueueLogReasons
{
  Pop
};

class EventQueueLogEntry : public DebugLogEntry
{
private:
  EventQueueLogReasons Reason;
  const char *EventName;

public:
  const char *GetSource() const override;
  std::string GetDescription() const override;

  EventQueueLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, EventQueueLogReasons reason, const char *eventName);
  virtual ~EventQueueLogEntry() = default;
};
