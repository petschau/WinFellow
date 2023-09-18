#pragma once

#include "fellow/api/vm/IM68K.h"

namespace fellow::vm
{
  class M68K : public fellow::api::vm::IM68K
  {
  public:
    void SetDReg(uint32_t registerNumber, uint32_t value) override;
    uint32_t GetDReg(uint32_t registerNumber) override;
    uint32_t GetAReg(uint32_t registerNumber) override;

    M68K();
  };
}
