#include "fellow/vm/M68K.h"
#include "fellow/cpu/CpuModule_Internal.h"
#include "fellow/cpu/CpuModule_Disassembler.h"
#include "fellow/application/Fellow.h"

#include <vector>

using namespace fellow::api::vm;
using namespace fellow::api;
using namespace std;

namespace fellow::vm
{
  void M68K::SetDReg(unsigned int registerNumber, uint32_t value)
  {
    cpuSetDReg(registerNumber, value);
  }

  uint32_t M68K::GetDReg(unsigned int registerNumber)
  {
    return cpuGetDReg(registerNumber);
  }

  uint32_t M68K::GetAReg(unsigned int registerNumber)
  {
    return cpuGetAReg(registerNumber);
  }

  uint32_t M68K::GetPC()
  {
    return cpuGetPC();
  }

  uint32_t M68K::GetUSP()
  {
    return cpuGetUspAutoMap();
  }

  uint32_t M68K::GetSSP()
  {
    return cpuGetSspAutoMap();
  }

  uint32_t M68K::GetMSP()
  {
    return cpuGetMspAutoMap();
  }

  uint32_t M68K::GetISP()
  {
    return cpuGetIspAutoMap();
  }

  uint32_t M68K::GetVBR()
  {
    return cpuGetVbr();
  }

  uint16_t M68K::GetSR()
  {
    return cpuGetSR();
  }

  fellow::api::vm::M68KRegisters M68K::GetRegisters()
  {
    return M68KRegisters{
        .DataRegisters = vector<uint32_t>{GetDReg(0), GetDReg(1), GetDReg(2), GetDReg(3), GetDReg(4), GetDReg(5), GetDReg(6), GetDReg(7)},
        .AddressRegisters = vector<uint32_t>{GetAReg(0), GetAReg(1), GetAReg(2), GetAReg(3), GetAReg(4), GetAReg(5), GetAReg(6), GetAReg(7)},
        .PC = GetPC(),
        .USP = GetUSP(),
        .SSP = GetSSP(),
        .SR = GetSR(),
        .VBR = GetVBR(),
        .MSP = GetMSP(),
        .ISP = GetISP()};
  }

  M68KDisassemblyLine M68K::GetDisassembly(uint32_t address)
  {
    char instAddress[256];
    char instData[256];
    char instName[256];
    char instOperands[256];

    instAddress[0] = '\0';
    instData[0] = '\0';
    instName[0] = '\0';
    instOperands[0] = '\0';

    uint32_t nextAddress = cpuDisOpcode(address, instAddress, instData, instName, instOperands);

    return M68KDisassemblyLine{address, nextAddress - address, instData, instName, instOperands};
  }

  void M68K::StepOne()
  {
    _runtimeEnvironment->StepOne();
  }

  void M68K::StepOver()
  {
    _runtimeEnvironment->StepOver();
  }

  void M68K::StepUntilBreakpoint(uint32_t breakpointAddress)
  {
    _runtimeEnvironment->RunDebug(breakpointAddress);
  }

  M68K::M68K(IRuntimeEnvironment *runtimeEnvironment) 
    : _runtimeEnvironment(runtimeEnvironment)
  {
  }
}