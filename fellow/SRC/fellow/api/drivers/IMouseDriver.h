#pragma once

class IMouseDriver
{
public:
  virtual void SetFocus(const bool bNewFocus, const bool bRequestedByRPHost) = 0;
  virtual bool GetFocus() = 0;
  virtual void ToggleFocus() = 0;
  virtual void StateHasChanged(bool active) = 0;
  virtual void MovementHandler() = 0;

  virtual bool GetInitializationFailed() = 0;

  virtual void HardReset() = 0;
  virtual bool EmulationStart() = 0;
  virtual void EmulationStop() = 0;

  IMouseDriver() = default;
  virtual ~IMouseDriver() = default;
};
