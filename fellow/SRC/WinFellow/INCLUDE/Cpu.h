#pragma once

#include "Cpu/ICpu.h"
#include "Scheduler/Clocks.h"

class Cpu : public ICpu
{
private:
  SchedulerEvent &_cpuEvent;
  Clocks &_clocks;

  void MasterEventLoopWrapper(std::function<void()> innerMasterEventLoop);
  SchedulerEventHandler GetInstructionEventHandler();

public:
  void SetCpuInstructionHandlerFromConfig() override;
  CpuMasterEventLoopWrapper GetMasterEventLoopWrapper() override;
  void InitializeCpuEvent() override;

  uint32_t GetPc() override;

  Cpu(SchedulerEvent &cpuEvent, Clocks &clocks);
};