#include "fellow/api/VM.h"
#include "fellow/vm/M68K.h"
#include "fellow/vm/MemorySystem.h"

using namespace fellow::vm;

namespace fellow::api
{
  M68K m68k;
  MemorySystem memorySystem;
  VirtualMachine *VM = new VirtualMachine(m68k, memorySystem);
}
