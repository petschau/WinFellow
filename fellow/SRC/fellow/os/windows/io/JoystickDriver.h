#pragma once

#include "fellow/api/drivers/IJoystickDriver.h"
#include <windows.h>
#include "fellow/os/windows/dxver.h"

constexpr auto MAX_JOY_PORT = 2;

class JoystickDriver : public IJoystickDriver
{
private:
  bool _failed = false;

  IDirectInput8 *_lpDI = nullptr;
  IDirectInputDevice8 *_lpDID[MAX_JOY_PORT];

  int num_joy_attached = 0;

  bool _active = false;
  bool _focus = false;
  bool _in_use = false;

  const char *DInputErrorString(HRESULT hResult);
  void DInputFailure(const char *header, HRESULT err);
  void DInputSetCooperativeLevel(int port);
  void DInputUnacquire(int port);
  void DInputAcquire(int port);
  bool DxCreateAndInitDevice(IDirectInput8 *pDi, IDirectInputDevice8 *pDiD[], GUID guid, int port);
  static BOOL FAR PASCAL InitJoystickInputCallback(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef);
  void DInputInitialize();
  void DInputRelease();
  bool CheckJoyMovement(int port, bool *Up, bool *Down, bool *Left, bool *Right, bool *Button1, bool *Button2);

public:
  BOOL HandleInitJoystickInputCallback(LPCDIDEVICEINSTANCE pdinst);
  void StateHasChanged(bool active) override;
  void ToggleFocus() override;
  void MovementHandler() override;

  void HardReset() override;
  void EmulationStart() override;
  void EmulationStop() override;
  void Initialize() override;
  void Release() override;

  virtual ~JoystickDriver() = default;
};
