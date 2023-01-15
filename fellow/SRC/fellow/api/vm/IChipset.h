#pragma once

#include "fellow/api/vm/ICopper.h"
#include "fellow/api/vm/IDisplay.h"

namespace fellow::api::vm
{
  class IChipset
  {
  public:
    ICopper *Copper;
    IDisplay *Display;

    IChipset(ICopper *copper, IDisplay *display) : Copper(copper), Display(display)
    {
    }

    virtual ~IChipset() = default;
  };
}
