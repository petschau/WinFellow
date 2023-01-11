#pragma once

#include "fellow/api/drivers/IMouseDriver.h"
#include <windows.h>
#include "fellow/os/windows/dxver.h"

class MouseDriver : public IMouseDriver
{
private:
  LPDIRECTINPUT _lpDI = nullptr;
  LPDIRECTINPUTDEVICE _lpDID = nullptr;
  HANDLE _DIevent = nullptr;

  bool _focus = true;
  bool _active = false;
  bool _inUse = false;
  bool _initializationFailed = false;
  bool _unacquired = true;
  bool _bLeftButton = false;
  bool _bRightButton = false;
  int _numMouseAttached = 0;

  static BOOL FAR PASCAL GetMouseInfoCallback(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef);
  BOOL HandleGetMouseInfoCallback();

  const char *GetDirectInputErrorString(HRESULT hResult);
  void LogDirectInputFailure(const char *header, HRESULT err);
  bool FailInitializationAndReleaseResources();

  const char *GetDirectInputUnaquireErrorDescription(HRESULT hResult);
  void LogDirectInputUnacquireFailure(const char *header, HRESULT err);
  void LogDirectInputAcquireFailure(const char *header, HRESULT err);
  void DirectInputAcquire();
  void DirectInputRelease();

  bool DirectInputInitialize();

public:
  HANDLE GetDirectInputEvent();

  void StateHasChanged(bool active) override;
  void MovementHandler() override;

  void SetFocus(const bool bNewFocus, const bool bRequestedByRPHost) override;
  bool GetFocus() override;
  void ToggleFocus() override;

  bool GetInitializationFailed() override;

  void HardReset() override;

  bool EmulationStart() override;
  void EmulationStop() override;

  MouseDriver();
  virtual ~MouseDriver();
};
