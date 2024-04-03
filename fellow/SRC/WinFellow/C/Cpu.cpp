#include "Cpu.h"
#include "CpuIntegration.h"
#include "CpuModule_Internal.h"
#include "CpuModule.h"

#include <csetjmp>

CpuMasterEventLoopWrapper Cpu::GetMasterEventLoopWrapper()
{
  return std::function<void(std::function<void()>)>([this](std::function<void()> innerMasterEventLoop) { this->MasterEventLoopWrapper(innerMasterEventLoop); });
}

void Cpu::MasterEventLoopWrapper(std::function<void()> innerMasterEventLoop)
{
  while (true)
  {
    if (setjmp(cpu_integration_exception_buffer) == 0)
    {
      innerMasterEventLoop();
      return;
    }
    else
    {
      // Returned out of an CPU exception that failed in the middle of an instruction.
      // TODO: This looks wrong, shifting down with cpuIntegrationGetSpeed?
      _cpuEvent.cycle = _clocks.GetMasterTime() + _clocks.ToMasterTimeOffset(cpuIntegrationGetChipCycles()) +
                        MasterTimeOffset::FromValue(cpuGetInstructionTime() * cpuIntegrationGetMasterCycleMultiplier());
    };
    cpuIntegrationSetChipCycles(ChipTimeOffset::FromValue(0));
  }
}

// Used by retro-platform when enabling and disabling turbo
void Cpu::SetCpuInstructionHandlerFromConfig()
{
  _cpuEvent.handler = GetInstructionEventHandler();
}

SchedulerEventHandler Cpu::GetInstructionEventHandler()
{
  if (cpuGetModelMajor() <= 1)
  {
    return cpuIntegrationExecuteInstructionEventHandler68000;
  }
  else
  {
    return cpuIntegrationExecuteInstructionEventHandler68020;
  }
}

void Cpu::InitializeCpuEvent()
{
  _cpuEvent.Initialize(GetInstructionEventHandler(), "CPU");
  _cpuEvent.cycle = MasterTimestamp::FromValue(0);
}

uint32_t Cpu::GetPc()
{
  return cpuGetPC();
}

Cpu::Cpu(SchedulerEvent &cpuEvent, Clocks &clocks) : _cpuEvent(cpuEvent), _clocks(clocks)
{
}
