#include "fellow/scheduler/SchedulerEvent.h"

bool SchedulerEvent::IsEnabled() const
{
  return cycle != EventDisableCycle;
}

void SchedulerEvent::Disable()
{
  cycle = EventDisableCycle;
}

void SchedulerEvent::Initialize(SchedulerEventHandler handlerFunc, const char *name)
{
  next = prev = nullptr;
  cycle = EventDisableCycle;
  handler = handlerFunc;
  Name = name;
}

SchedulerEvent::SchedulerEvent() : next(nullptr), prev(nullptr), cycle(EventDisableCycle), handler(nullptr), Name(nullptr)
{
}
