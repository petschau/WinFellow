#pragma once

#include <cstdint>
#include <functional>

#include "MasterTimestamp.h"

typedef std::function<void()> SchedulerEventHandler;

struct SchedulerEvent
{
  static constexpr MasterTimestamp EventDisableCycle{.Cycle = 0xffffffff};

  SchedulerEvent *next;
  SchedulerEvent *prev;
  MasterTimestamp cycle;
  SchedulerEventHandler handler;
  const char *Name;

  bool IsEnabled() const;
  void Disable();
  void Initialize(SchedulerEventHandler handlerFunc, const char *name);

  SchedulerEvent();
};
