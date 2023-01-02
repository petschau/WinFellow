#pragma once

class IJoystickDriver
{
public:
  virtual void StateHasChanged(bool active) = 0;
  virtual void ToggleFocus() = 0;
  virtual void MovementHandler() = 0;

  virtual void HardReset() = 0;
  virtual void EmulationStart() = 0;
  virtual void EmulationStop() = 0;
  virtual void Initialize() = 0;
  virtual void Release() = 0;
};
