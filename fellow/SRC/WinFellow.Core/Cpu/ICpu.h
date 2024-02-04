#pragma once

#include <functional>

#include "Scheduler/SchedulerEvent.h"

typedef std::function<void(std::function<void()> innerMasterEventLoop)> CpuMasterEventLoopWrapper;

class ICpu
{
public:
  virtual void SetCpuInstructionHandlerFromConfig() = 0;
  virtual CpuMasterEventLoopWrapper GetMasterEventLoopWrapper() = 0;
  virtual void InitializeCpuEvent() = 0;

  virtual uint32_t GetPc() = 0;
};
