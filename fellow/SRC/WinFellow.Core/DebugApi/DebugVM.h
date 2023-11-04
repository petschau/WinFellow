#pragma once

#include "Debug/IM68K.h"
#include "Debug/IMemorySystem.h"

namespace Debug
{
  class DebugVM
  {
  public:
    IM68K *CPU;
    IMemorySystem *Memory;

    DebugVM();
  };
}
