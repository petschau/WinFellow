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
  VirtualMachine *CreateVM(IRuntimeEnvironment* runtimeEnvironment)
  {
    return new VirtualMachine(new M68K(runtimeEnvironment), new Clocks(), new MemorySystem(), new Chipset(new Copper(), new Display()), new EventLog());
  }
}
