#pragma once

#include "Defs.h"

namespace fellow::api::vm
{
  class IM68K
  {
  public:
    virtual void SetDReg(uint32_t registerNumber, uint32_t value) = 0;
    virtual uint32_t GetDReg(uint32_t registerNumber) = 0;
    virtual uint32_t GetAReg(uint32_t registerNumber) = 0;
  };
}
