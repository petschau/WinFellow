#pragma once

#include "SchedulerEvent.h"

struct SchedulerEvents
{
  SchedulerEvent cpuEvent;
  SchedulerEvent copperEvent;
  SchedulerEvent eolEvent;
  SchedulerEvent eofEvent;
  SchedulerEvent ciaEvent;
  SchedulerEvent blitterEvent;
  SchedulerEvent interruptEvent;
  SchedulerEvent chipBusAccessEvent;
  SchedulerEvent ddfEvent;
  SchedulerEvent diwxEvent;
  SchedulerEvent diwyEvent;
  SchedulerEvent shifterEvent;
  SchedulerEvent bitplaneDMAEvent;
  SchedulerEvent spriteDMAEvent;
};
