#pragma once

#include "fellow/api/drivers/IJoystickDriver.h"
#include <windows.h>
#include "fellow/os/windows/dxver.h"

constexpr auto MAX_JOY_PORT = 2;

class JoystickDriver : public IJoystickDriver
{
private:
  BOOLE _failed;

  IDirectInput8 *_lpDI;
  IDirectInputDevice8 *_lpDID[MAX_JOY_PORT];

  int num_joy_supported;
  int num_joy_attached;

  BOOLE _active;
  BOOLE _focus;
  BOOLE _in_use;

  const char *DInputErrorString(HRESULT hResult);
  void DInputFailure(const char *header, HRESULT err);
  void DInputSetCooperativeLevel(int port);
  void DInputUnacquire(int port);
  void DInputAcquire(int port);
  BOOLE DxCreateAndInitDevice(IDirectInput8 *pDi, IDirectInputDevice8 *pDiD[], GUID guid, int port);
  static BOOL FAR PASCAL InitJoystickInputCallback(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef);
  void DInputInitialize();
  void DInputRelease();
  BOOLE CheckJoyMovement(int port, BOOLE *Up, BOOLE *Down, BOOLE *Left, BOOLE *Right, BOOLE *Button1, BOOLE *Button2);

public:
  BOOL HandleInitJoystickInputCallback(LPCDIDEVICEINSTANCE pdinst);
  void StateHasChanged(BOOLE active) override;
  void ToggleFocus() override;
  void MovementHandler() override;

  void HardReset() override;
  void EmulationStart() override;
  void EmulationStop() override;
  void Startup() override;
  void Shutdown() override;

  virtual ~JoystickDriver() = default;
};
