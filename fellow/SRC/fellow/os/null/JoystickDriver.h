#pragma once

#include "fellow/api/drivers/IJoystickDriver.h"

class JoystickDriver : public IJoystickDriver
{
public:
  void StateHasChanged(BOOLE active) override;
  void ToggleFocus() override;
  void MovementHandler() override;

  void HardReset() override;
  void EmulationStart() override;
  void EmulationStop() override;
  void Startup() override;
  void Shutdown() override;
};
