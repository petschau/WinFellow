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
  MasterTimestamp GetNextEventCycle() const;
  void RebaseEvents(MasterTimeOffset offset);

  SchedulerQueue();
};
