#pragma once

#include "fellow/api/vm/IM68K.h"
#include "fellow/api/vm/IClocks.h"
#include "fellow/api/vm/IMemorySystem.h"
#include "fellow/api/vm/IChipset.h"
#include "fellow/api/vm/IEventLog.h"

namespace fellow::api
{
  class VirtualMachine
  {
  public:
    fellow::api::vm::IM68K &CPU;
    fellow::api::vm::IClocks &Clocks;
    fellow::api::vm::IMemorySystem &Memory;
    fellow::api::vm::IChipset &Chipset;
    fellow::api::vm::IEventLog &EventLog;

    VirtualMachine(fellow::api::vm::IM68K &cpu, fellow::api::vm::IClocks &clocks, fellow::api::vm::IMemorySystem &memory, fellow::api::vm::IChipset &chipset, fellow::api::vm::IEventLog &eventLog)
        : CPU(cpu), Clocks(clocks), Memory(memory), Chipset(chipset), EventLog(eventLog)
    {
    }
  };

  extern VirtualMachine *VM;
}
