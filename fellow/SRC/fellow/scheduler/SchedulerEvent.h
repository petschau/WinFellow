#pragma once

#include "fellow/api/defs.h"

typedef void (*SchedulerEventHandler)();

struct SchedulerEvent
{
  static constexpr unsigned int EventDisableCycle = 0xffffffff;

  SchedulerEvent *next;
  SchedulerEvent *prev;
  ULO cycle;
  SchedulerEventHandler handler;
  const char *Name;

  bool IsEnabled() const;
  void Disable();
  void Initialize(SchedulerEventHandler handlerFunc, const char *name);

  SchedulerEvent();
};
