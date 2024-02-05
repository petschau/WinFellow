#pragma once

#include "SchedulerEvent.h"

class SchedulerQueue
{
private:
  SchedulerEvent *_events;

public:
  void Clear();
  void InsertEvent(SchedulerEvent *ev);
  void InsertEventWithNullCheck(SchedulerEvent *ev);
  void RemoveEvent(SchedulerEvent *ev);
  SchedulerEvent *PopEvent();
  SchedulerEvent *PeekNextEvent() const;
  uint32_t GetNextEventCycle() const;
  void RebaseEvents(uint32_t cycleOffset);

  SchedulerQueue();
};
