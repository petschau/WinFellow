#include "DebugApi/M68K.h"
#include "CpuModule_Internal.h"

namespace Debug
{
  void M68K::SetDReg(uint32_t registerNumber, uint32_t value)
  {
    cpuSetDReg(registerNumber, value);
  }

  uint32_t M68K::GetDReg(uint32_t registerNumber)
  {
    return cpuGetDReg(registerNumber);
  }

  uint32_t M68K::GetAReg(uint32_t registerNumber)
  {
    return cpuGetAReg(registerNumber);
  }

  M68K::M68K()
  {
  }

  M68K::~M68K()
  {
  }
}