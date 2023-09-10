#ifndef FELLOW_VM_M68K_H
#define FELLOW_VM_M68K_H

#include "fellow/api/vm/IM68K.h"

namespace fellow::vm
{
  class M68K : public fellow::api::vm::IM68K
  {
  public:
    void SetDReg(ULO registerNumber, ULO value) override;
    ULO GetDReg(ULO registerNumber) override;
    ULO GetAReg(ULO registerNumber) override;

    M68K();
  };
}

#endif
