#pragma once

#include "fellow/api/vm/IM68K.h"
#include "fellow/api/vm/IClocks.h"
#include "fellow/api/vm/IMemorySystem.h"
#include "fellow/api/vm/IChipset.h"
#include "fellow/api/vm/IEventLog.h"

namespace fellow::api
{
  class IRuntimeEnvironment;

  // Encapsulates debug and inspection functionality, it is not implementation of emulation
  class VirtualMachine
  {
  public:
    vm::IM68K *CPU;
    vm::IClocks *Clocks;
    vm::IMemorySystem *Memory;
    vm::IChipset *Chipset;
    vm::IEventLog *EventLog;

    VirtualMachine(vm::IM68K *cpu, vm::IClocks *clocks, vm::IMemorySystem *memory, vm::IChipset *chipset, vm::IEventLog *eventLog)
      : CPU(cpu), Clocks(clocks), Memory(memory), Chipset(chipset), EventLog(eventLog)
    {
    }

    ~VirtualMachine()
    {
      delete CPU;
      delete Clocks;
      delete Memory;
      delete Chipset;
      delete EventLog;
    }
  };

  VirtualMachine *CreateVirtualMachine(IRuntimeEnvironment *runtimeEnvironment);

}
