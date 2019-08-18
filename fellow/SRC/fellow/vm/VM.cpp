#include "fellow/api/VM.h"
#include "fellow/vm/M68K.h"
#include "fellow/vm/Clocks.h"
#include "fellow/vm/MemorySystem.h"
#include "fellow/vm/Display.h"
#include "fellow/vm/Copper.h"
#include "fellow/vm/Chipset.h"
#include "fellow/vm/EventLog.h"

using namespace fellow::vm;

namespace fellow::api
{
  M68K m68k;
  Clocks clocks;
  MemorySystem memorySystem;
  Display display;
  Copper copper;
  Chipset chipset(copper, display);
  EventLog eventLog;

  VirtualMachine *VM = new VirtualMachine(m68k, clocks, memorySystem, chipset, eventLog);
}
