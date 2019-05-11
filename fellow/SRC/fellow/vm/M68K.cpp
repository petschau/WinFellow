#include "fellow/vm/M68K.h"
#include "CpuModule_Internal.h"

namespace fellow::vm
{
  void M68K::SetDReg(ULO registerNumber, ULO value)
  {
    cpuSetDReg(registerNumber, value);
  }

  ULO M68K::GetDReg(ULO registerNumber)
  {
    return cpuGetDReg(registerNumber);
  }

  ULO M68K::GetAReg(ULO registerNumber)
  {
    return cpuGetAReg(registerNumber);
  }

  M68K::M68K()
  {
  }
}