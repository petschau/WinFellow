#pragma once

#include <string>
#include <vector>

namespace fellow::api::vm
{
  class M68KDisassemblyLine
  {
  public:
    uint32_t Address;
    uint32_t Length;
    std::string Data;
    std::string Instruction;
    std::string Operands;

    const std::string &GetData() const
    {
      return Data;
    }
    const std::string &GetInstruction() const
    {
      return Instruction;
    }
  };

  class M68KRegisters
  {
  public:
    std::vector<uint32_t> DataRegisters;
    std::vector<uint32_t> AddressRegisters;
    uint32_t PC;
    uint32_t USP;
    uint32_t SSP;
    uint16_t SR;

    uint32_t VBR;
    uint32_t MSP;
    uint32_t ISP;
  };

  class IM68K
  {
  public:
    virtual void SetDReg(unsigned int registerNumber, uint32_t value) = 0;
    virtual uint32_t GetDReg(unsigned int registerNumber) = 0;
    virtual uint32_t GetAReg(unsigned int registerNumber) = 0;
    virtual uint32_t GetPC() = 0;
    virtual uint32_t GetUSP() = 0;
    virtual uint32_t GetSSP() = 0;
    virtual uint32_t GetMSP() = 0;
    virtual uint32_t GetISP() = 0;
    virtual uint32_t GetVBR() = 0;
    virtual uint16_t GetSR() = 0;

    virtual M68KRegisters GetRegisters() = 0;
    virtual M68KDisassemblyLine GetDisassembly(uint32_t address) = 0;

    virtual void StepOne() = 0;
    virtual void StepOver() = 0;
    virtual void StepUntilBreakpoint(uint32_t breakpointAddress) = 0;

    virtual ~IM68K() = default;
  };
}
