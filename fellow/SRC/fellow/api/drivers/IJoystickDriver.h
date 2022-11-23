#pragma once

#include "fellow/api/defs.h"

class IJoystickDriver
{
public:
  virtual void StateHasChanged(BOOLE active) = 0;
  virtual void ToggleFocus() = 0;
  virtual void MovementHandler() = 0;

  virtual void HardReset() = 0;
  virtual void EmulationStart() = 0;
  virtual void EmulationStop() = 0;
  virtual void Startup() = 0;
  virtual void Shutdown() = 0;
};
