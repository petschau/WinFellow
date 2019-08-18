#pragma once

#include "fellow/api/vm/IDisplay.h"

namespace fellow::vm
{
  class Display : public fellow::api::vm::IDisplay
  {
  public:
    fellow::api::vm::DisplayState GetState() override;

    Display() = default;
    virtual ~Display() = default;
  };
}
