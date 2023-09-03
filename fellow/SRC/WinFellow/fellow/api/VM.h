#ifndef FELLOW_API_VM_H
#define FELLOW_API_VM_H

#include "fellow/api/VM/IM68K.h"
#include "fellow/api/VM/IMemorySystem.h"

namespace fellow::api
{
  class VirtualMachine
  {
  public:
    fellow::api::vm::IM68K& CPU;
    fellow::api::vm::IMemorySystem& Memory;

    VirtualMachine(fellow::api::vm::IM68K& cpu, fellow::api::vm::IMemorySystem& memory) : CPU(cpu), Memory(memory) {}
  };

  extern VirtualMachine* VM;
}

#endif
