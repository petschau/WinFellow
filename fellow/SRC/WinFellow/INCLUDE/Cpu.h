#pragma once

#include "Cpu/ICpu.h"
#include "Scheduler/Timekeeper.h"

class Cpu : public ICpu
{
private:
  SchedulerEvent &_cpuEvent;
  Timekeeper &_timekeeper;

  void MasterEventLoopWrapper(std::function<void()> innerMasterEventLoop);
  SchedulerEventHandler GetInstructionEventHandler();

public:
  void SetCpuInstructionHandlerFromConfig() override;
  CpuMasterEventLoopWrapper GetMasterEventLoopWrapper() override;
  void InitializeCpuEvent() override;

  uint32_t GetPc() override;

  Cpu(SchedulerEvent &cpuEvent, Timekeeper &timekeeper);
};