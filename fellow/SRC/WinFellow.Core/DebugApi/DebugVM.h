#pragma once

#include "DebugApi/IM68K.h"
#include "DebugApi/IMemorySystem.h"

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
