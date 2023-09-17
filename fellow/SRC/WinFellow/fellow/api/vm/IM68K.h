#ifndef FELLOW_API_VM_IM68K_H
#define FELLOW_API_VM_IM68K_H

#include "fellow/api/defs.h"

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

#endif
