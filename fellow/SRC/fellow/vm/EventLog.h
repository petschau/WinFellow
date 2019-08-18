#pragma once

#include <vector>
#include "fellow/api/vm/IEventLog.h"

namespace fellow::vm
{
  class EventLog : public fellow::api::vm::IEventLog
  {
  public:
    std::vector<fellow::api::vm::EventLogEntry> GetEvents() override;

    EventLog() = default;
    virtual ~EventLog() = default;
  };
}
