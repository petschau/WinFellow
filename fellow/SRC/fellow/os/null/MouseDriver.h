#pragma once

#include "fellow/api/drivers/IMouseDriver.h"

class MouseDriver : public IMouseDriver
{
public:
  BOOLE GetFocus() override;
  void StateHasChanged(BOOLE active) override;
  void ToggleFocus() override;
  void MovementHandler() override;
  void SetFocus(const BOOLE bNewFocus, const BOOLE bRequestedByRPHost) override;

  void HardReset() override;
  BOOLE EmulationStart() override;
  void EmulationStop() override;
  void Startup() override;
  void Shutdown() override;
};
