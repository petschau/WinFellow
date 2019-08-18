#pragma once

#include "fellow/api/vm/IM68K.h"

namespace fellow::vm
{
  class M68K : public fellow::api::vm::IM68K
  {
  public:
    void SetDReg(unsigned int registerNumber, uint32_t value) override;
    uint32_t GetDReg(unsigned int registerNumber) override;
    uint32_t GetAReg(unsigned int registerNumber) override;
    uint32_t GetPC() override;
    uint32_t GetUSP() override;
    uint32_t GetSSP() override;
    uint32_t GetMSP() override;
    uint32_t GetISP() override;
    uint32_t GetVBR() override;
    uint16_t GetSR() override;

    fellow::api::vm::M68KRegisters GetRegisters() override;
    fellow::api::vm::M68KDisassemblyLine GetDisassembly(uint32_t address) override;

    void StepOne() override;
    void StepOver() override;
    void StepUntilBreakpoint(uint32_t breakpointAddress) override;

    M68K() = default;
    virtual ~M68K() = default;
  };
}
