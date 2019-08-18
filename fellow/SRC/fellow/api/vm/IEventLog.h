#pragma once

#include <vector>
#include <string>
#include "fellow/api/defs.h"

namespace fellow::api::vm
{
  class EventLogEntry
  {
  public:
    uint64_t FrameNumber;
    uint32_t LineNumber;
    uint32_t BaseCycle;
    std::string Source;
    std::string Description;

    EventLogEntry(uint64_t frameNumber, uint32_t lineNumber, uint32_t baseCycle, const char *source, const std::string &description)
        : FrameNumber(frameNumber), LineNumber(lineNumber), BaseCycle(baseCycle), Source(source), Description(description)
    {
    }
  };

  class IEventLog
  {
  public:
    virtual std::vector<EventLogEntry> GetEvents() = 0;

    virtual ~IEventLog() = default;
  };
}
