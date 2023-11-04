#include "Debug/DebugVM.h"
#include "Debug/M68K.h"
#include "Debug/MemorySystem.h"

namespace Debug
{
  DebugVM::DebugVM()
    : CPU(nullptr), Memory(nullptr)
  {
  }
}
