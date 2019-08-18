#pragma once

#include "fellow/api/vm/IChipset.h"

using namespace fellow::api::vm;

namespace fellow::vm
{
  class Chipset : public IChipset
  {
  public:
    Chipset(ICopper &copper, IDisplay &display);
    virtual ~Chipset();
  };
}
