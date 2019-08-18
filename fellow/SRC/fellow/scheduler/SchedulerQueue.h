#pragma once

#include "fellow/scheduler/SchedulerEvent.h"

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
  ULO GetNextEventCycle() const;

  SchedulerQueue();
};
