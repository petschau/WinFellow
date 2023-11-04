#include "DebugApi/DebugVM.h"
#include "DebugApi/M68K.h"
#include "DebugApi/MemorySystem.h"

namespace Debug
{
  DebugVM::DebugVM()
    : CPU(nullptr), Memory(nullptr)
  {
  }
}
