#pragma once

#include "fellow/api/defs.h"

class IMouseDriver
{
public:
  virtual BOOLE GetFocus() = 0;
  virtual void StateHasChanged(BOOLE active) = 0;
  virtual void ToggleFocus() = 0;
  virtual void MovementHandler() = 0;
  virtual void SetFocus(const BOOLE bNewFocus, const BOOLE bRequestedByRPHost) = 0;

  virtual void HardReset() = 0;
  virtual BOOLE EmulationStart() = 0;
  virtual void EmulationStop() = 0;
  virtual void Startup() = 0;
  virtual void Shutdown() = 0;
};
