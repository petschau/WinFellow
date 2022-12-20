#pragma once

#include "fellow/api/drivers/IMouseDriver.h"
#include <windows.h>
#include "fellow/os/windows/dxver.h"

class MouseDriver : public IMouseDriver
{
private:
  LPDIRECTINPUT _lpDI;
  LPDIRECTINPUTDEVICE _lpDID;
  HANDLE _DIevent;
  BOOLE _focus;
  BOOLE _active;
  BOOLE _in_use;
  BOOLE _initialization_failed;
  bool _unacquired;
  static BOOLE _bLeftButton;
  static BOOLE _bRightButton;
  int _num_mouse_attached = 0;

  static BOOL FAR PASCAL GetMouseInfoCallback(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef);
  BOOL HandleGetMouseInfoCallback();

  const char *DInputErrorString(HRESULT hResult);
  const char *DInputUnaquireReturnValueString(HRESULT hResult);
  void DInputFailure(const char *header, HRESULT err);
  void DInputUnacquireFailure(const char *header, HRESULT err);
  void DInputAcquireFailure(const char *header, HRESULT err);
  void DInputAcquire();
  void DInputRelease();
  BOOLE DInputInitialize();

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
