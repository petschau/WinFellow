#pragma once

#include <cstdint>
#include <functional>

typedef std::function<void()> SchedulerEventHandler;

struct SchedulerEvent
{
  static constexpr uint32_t EventDisableCycle = 0xffffffff;

  SchedulerEvent *next;
  SchedulerEvent *prev;
  uint32_t cycle;
  SchedulerEventHandler handler;
  const char *Name;

  bool IsEnabled() const;
  void Disable();
  void Initialize(SchedulerEventHandler handlerFunc, const char *name);

  SchedulerEvent();
};
