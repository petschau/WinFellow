#ifndef FELLOW_API_VM_IM68K_H
#define FELLOW_API_VM_IM68K_H

#include "fellow/api/defs.h"

namespace fellow::api::vm
{
  class IM68K
  {
  public:
    virtual void SetDReg(ULO registerNumber, ULO value) = 0;
    virtual ULO GetDReg(ULO registerNumber) = 0;
    virtual ULO GetAReg(ULO registerNumber) = 0;
  };
}

#endif
