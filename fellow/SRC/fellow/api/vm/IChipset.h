#pragma once

#include "fellow/api/vm/ICopper.h"
#include "fellow/api/vm/IDisplay.h"

namespace fellow::api::vm
{
  class IChipset
  {
  public:
    fellow::api::vm::ICopper &Copper;
    fellow::api::vm::IDisplay &Display;

    IChipset(fellow::api::vm::ICopper &copper, fellow::api::vm::IDisplay &display) : Copper(copper), Display(display)
    {
    }

    virtual ~IChipset() = default;
  };
}
