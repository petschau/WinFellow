#pragma once

#include "fellow/api/vm/IChipset.h"

namespace fellow::vm
{
  class Chipset : public fellow::api::vm::IChipset
  {
  public:
    Chipset(fellow::api::vm::ICopper *copper, fellow::api::vm::IDisplay *display);
    virtual ~Chipset();
  };
}
