/*=========================================================================*/
/* Fellow                                                                  */
/* Joystick driver for Windows                                             */
/* Author: Marco Nova (novamarco@hotmail.com)                              */
/*         Torsten Enderling (carfesh@gmx.net) (Nov 2007 DirectX SDK fixes)*/
/*                                                                         */
/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software Foundation, */
/* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          */
/*=========================================================================*/

#include "fellow/api/defs.h"
#include "fellow/api/Drivers.h"
#include "fellow/application/Gameport.h"
#include "fellow/api/Services.h"
#include "fellow/os/windows/io/JoystickDriver.h"
#include "fellow/os/windows/application/WindowsDriver.h"
#include "fellow/os/windows/graphics/GraphicsDriver.h"

#include <mmsystem.h>

using namespace fellow::api;

// min and max values for the axis
// -------------------------------
//
// these min values are the values that the driver will return when
// the lever of the joystick is at the extreme left (x) or the extreme top (y)
// the max values are the same but for the extreme right (x) and the extreme bottom (y)

#define MINX 0
#define MINY 0
#define MAXX 8000
#define MAXY 8000

// med values for both axis
// ------------------------
//
// the med values are used to determine if the stick is in the center or has been moved
// they should be calculated with MEDX = (MAXX-MINX) / 2 and MINY = (MAXY-MINY) / 2
// but they are precalculated for a very very little speed increase (why should we bother
// with runtime division when we already know the results?)

#define MEDX 4000 // precalculated value should be MAXX / 2
#define MEDY 4000 // precalculated value should be MAXY / 2

// dead zone
// ---------
//
// the dead zone is the zone that will not be evaluated as a joy movement
// it is automatically handled by DirectX and it is calculated around the
// center and it based on min and max values previously setuped

#define DEADX 1000 // 10%
#define DEADY 1000 // 10%

const char *JoystickDriver::DInputErrorString(HRESULT hResult)
{
#ifdef _DEBUG
  STR *UnErr = nullptr;
#endif

  switch (hResult)
  {
    case DI_OK: return "The operation completed successfully.";
    case DI_BUFFEROVERFLOW: return "The device buffer overflowed and some input was lost.";
    case DI_POLLEDDEVICE: return "The device is a polled device.";
    case DIERR_ACQUIRED: return "The operation cannot be performed while the device is acquired.";
    case DIERR_ALREADYINITIALIZED: return "This object is already initialized.";
    case DIERR_BADDRIVERVER: return "The object could not be created due to an incompatible driver version or mismatched or incomplete driver components.";
    case DIERR_BETADIRECTINPUTVERSION: return "The application was written for an unsupported prerelease version of DirectInput.";
    case DIERR_DEVICENOTREG: return "The device or device instance is not registered with DirectInput.";
    case DIERR_GENERIC: return "An undetermined error occurred inside the DirectInput subsystem.";
    case DIERR_HANDLEEXISTS: return "The device already has an event notification associated with it.";
    case DIERR_INPUTLOST: return "Access to the input device has been lost. It must be re-acquired.";
    case DIERR_INVALIDPARAM: return "An invalid parameter was passed to the returning function, or the object was not in a state that permitted the function to be called.";
    case DIERR_NOAGGREGATION: return "This object does not support aggregation.";
    case DIERR_NOINTERFACE: return "The specified interface is not supported by the object.";
    case DIERR_NOTACQUIRED: return "The operation cannot be performed unless the device is acquired.";
    case DIERR_NOTINITIALIZED: return "This object has not been initialized.";
    case DIERR_OBJECTNOTFOUND: return "The requested object does not exist.";
    case DIERR_OLDDIRECTINPUTVERSION: return "The application requires a newer version of DirectInput.";
    case DIERR_OUTOFMEMORY: return "The DirectInput subsystem couldn't allocate sufficient memory to complete the caller's request.";
    case DIERR_UNSUPPORTED: return "The function called is not supported at this time.";
    case E_PENDING: return "Data is not yet available.";
    case E_POINTER: return "Invalid pointer.";
  }

#ifdef _DEBUG

  // TODO: This looks like a memory leak
  UnErr = (STR *)malloc(255 * sizeof(STR));
  sprintf(UnErr, "Not a DirectInput Error [%08x]", hResult);
  return UnErr;
#else
  return "Not a DirectInput Error";
#endif
}

void JoystickDriver::DInputFailure(const char *header, HRESULT err)
{
  Service->Log.AddLog("%s %s\n", header, DInputErrorString(err));
}

void JoystickDriver::DInputSetCooperativeLevel(int port)
{
  DIPROPRANGE diprg;
  DIPROPDWORD dipdw;

  Service->Log.AddLog("JoystickDriver::DInputSetCooperativeLevel(%d)\n", port);

  if (_failed)
  {
    return;
  }

  HWND hwnd = ((GraphicsDriver *)Driver->Graphics)->GetHWND();
  HRESULT res = IDirectInputDevice8_SetCooperativeLevel(_lpDID[port], hwnd, ((_focus) ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE) | DISCL_FOREGROUND);
  if (res != DI_OK)
  {
    DInputFailure("JoystickDriver::DInputSetCooperativeLevel():", res);
  }

#define INITDIPROP(diprp, obj, how)                                                                                                                                                                    \
  {                                                                                                                                                                                                    \
    diprp.diph.dwSize = sizeof(diprp);                                                                                                                                                                 \
    diprp.diph.dwHeaderSize = sizeof(diprp.diph);                                                                                                                                                      \
    diprp.diph.dwObj = obj;                                                                                                                                                                            \
    diprp.diph.dwHow = how;                                                                                                                                                                            \
  }

  INITDIPROP(diprg, DIJOFS_X, DIPH_BYOFFSET)
  diprg.lMin = MINX;
  diprg.lMax = MAXX;

  res = IDirectInputDevice8_SetProperty(_lpDID[port], DIPROP_RANGE, &diprg.diph);
  if (res != DI_OK)
  {
    DInputFailure("JoystickDriver::DInputSetCooperativeLevel(): SetProperty RANGE X :", res);
  }

  INITDIPROP(diprg, DIJOFS_Y, DIPH_BYOFFSET)
  diprg.lMin = MINY;
  diprg.lMax = MAXY;

  res = IDirectInputDevice8_SetProperty(_lpDID[port], DIPROP_RANGE, &diprg.diph);
  if (res != DI_OK)
  {
    DInputFailure("JoystickDriver::DInputSetCooperativeLevel(): SetProperty RANGE Y :", res);
  }

  INITDIPROP(dipdw, DIJOFS_X, DIPH_BYOFFSET)
  dipdw.dwData = DEADX;

  res = IDirectInputDevice8_SetProperty(_lpDID[port], DIPROP_DEADZONE, &dipdw.diph);
  if (res != DI_OK)
  {
    DInputFailure("JoystickDriver::DInputSetCooperativeLevel(): SetProperty DEADZONE X :", res);
  }

  INITDIPROP(dipdw, DIJOFS_Y, DIPH_BYOFFSET)
  dipdw.dwData = DEADY;

  res = IDirectInputDevice8_SetProperty(_lpDID[port], DIPROP_DEADZONE, &dipdw.diph);
  if (res != DI_OK)
  {
    DInputFailure("JoystickDriver::DInputSetCooperativeLevel(): SetProperty DEADZONE Y :", res);
  }

#undef INITDIPROP
}

void JoystickDriver::DInputUnacquire(int port)
{
  if (gameportGetAnalogJoystickInUse())
  {
    Service->Log.AddLog("JoystickDriver::DInputUnacquire(%d)\n", port);

    if (!_failed)
    {
      HRESULT res = IDirectInputDevice8_Unacquire(_lpDID[port]);
      if (res != DI_OK)
      {
        DInputFailure("JoystickDriver::DInputUnacquire():", res);
      }
    }
  }
}

void JoystickDriver::DInputAcquire(int port)
{
  if (gameportGetAnalogJoystickInUse())
  {
    Service->Log.AddLog("JoystickDriver::DInputAcquire(%d)\n", port);

    if (_in_use)
    {
      /* A new window is sometimes created, so set new hwnd cooperative level */
      DInputSetCooperativeLevel(port);

      HRESULT res = IDirectInputDevice8_Acquire(_lpDID[port]);
      if (res != DI_OK)
      {
        DInputFailure("JoystickDriver::DInputAcquire():", res);
      }
    }
  }
}

bool JoystickDriver::DxCreateAndInitDevice(IDirectInput8 *pDi, IDirectInputDevice8 *pDiD[], GUID guid, int port)
{
  HRESULT res;

  if (!pDiD[port])
  {
    res = CoCreateInstance(CLSID_DirectInputDevice8, nullptr, CLSCTX_INPROC_SERVER, IID_IDirectInputDevice8, (LPVOID *)&(pDiD[port]));
    if (res != DI_OK)
    {
      DInputFailure("JoystickDriver::DInputInitialize(): DeviceCoCreateInstance()", res);
      return true;
    }

    res = IDirectInputDevice8_Initialize(pDiD[port], win_drv_hInstance, DIRECTINPUT_VERSION, guid);
    if (res != DI_OK)
    {
      DInputFailure("JoystickDriver::DInputInitialize(): DeviceInitialize()", res);
      return true;
    }
  }

  res = IDirectInputDevice8_SetDataFormat(pDiD[port], &c_dfDIJoystick);
  if (res != DI_OK)
  {
    DInputFailure("JoystickDriver::DInputInitialize(): SetDataFormat()", res);
    return true;
  }

  return false;
}

BOOL FAR PASCAL JoystickDriver::InitJoystickInputCallback(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef)
{
  return ((JoystickDriver *)pvRef)->HandleInitJoystickInputCallback(pdinst);
}

BOOL JoystickDriver::HandleInitJoystickInputCallback(LPCDIDEVICEINSTANCE pdinst)
{
  IDirectInput8 *pdi = _lpDI;

  Service->Log.AddLog("**** Joystick %d **** '%s'\n", num_joy_attached, pdinst->tszProductName);

  if (!DxCreateAndInitDevice(pdi, _lpDID, pdinst->guidInstance, num_joy_attached++))
  {
    if (num_joy_attached == 2)
    {
      return DIENUM_STOP;
    }
    else
    {
      return DIENUM_CONTINUE;
    }
  }
  else
  {
    return DIENUM_STOP;
  }
}

void JoystickDriver::DInputInitialize()
{
  Service->Log.AddLog("JoystickDriver::DInputInitialize()\n");

  if (!_lpDI)
  {
    HRESULT res = CoCreateInstance(CLSID_DirectInput8, nullptr, CLSCTX_INPROC_SERVER, IID_IDirectInput8, (LPVOID *)&_lpDI);
    if (res != DI_OK)
    {
      DInputFailure("JoystickDriver::DInputInitialize(): CoCreateInstance()", res);
      _failed = true;
      return;
    }

    res = IDirectInput8_Initialize(_lpDI, win_drv_hInstance, DIRECTINPUT_VERSION);
    if (res != DI_OK)
    {
      DInputFailure("JoystickDriver::DInputInitialize(): Initialize()", res);
      _failed = true;
      return;
    }

    num_joy_attached = 0;

    res = IDirectInput8_EnumDevices(_lpDI, DI8DEVCLASS_GAMECTRL, InitJoystickInputCallback, this, DIEDFL_ATTACHEDONLY);
    if (res != DI_OK)
    {
      DInputFailure("JoystickDriver::DInputInitialize(): EnumDevices()", res);
      _failed = true;
      return;
    }

    Service->Log.AddLog("njoy: %d\n", num_joy_attached);
  }
}

void JoystickDriver::DInputRelease()
{
  Service->Log.AddLog("JoystickDriver::DInputRelease()\n");

  for (int port = 0; port < MAX_JOY_PORT; port++)
  {
    if (_lpDID[port] != nullptr)
    {
      DInputUnacquire(port);
      IDirectInputDevice8_Release(_lpDID[port]);
      _lpDID[port] = nullptr;
    }
  }

  if (_lpDI != nullptr)
  {
    IDirectInput8_Release(_lpDI);
    _lpDI = nullptr;
  }
}

void JoystickDriver::StateHasChanged(bool active)
{
  if (_failed)
  {
    return;
  }

  _active = active;
  _in_use = _active && _focus;

  for (int port = 0; port < MAX_JOY_PORT; port++)
  {
    if (_lpDID[port] != nullptr)
    {
      DInputAcquire(port);
    }
  }
}

void JoystickDriver::ToggleFocus()
{
  _focus = !_focus;
  StateHasChanged(_active);
}

bool JoystickDriver::CheckJoyMovement(int port, bool *Up, bool *Down, bool *Left, bool *Right, bool *Button1, bool *Button2)
{
  DIJOYSTATE dims;
  HRESULT res;
  int LostCounter = 25;

  *Button1 = *Button2 = *Left = *Right = *Up = *Down = false;

  do
  {
    res = IDirectInputDevice8_Poll(_lpDID[port]);
    if (res != DI_OK)
    {
      if (res != DI_BUFFEROVERFLOW)
      {
        DInputFailure("JoystickDriver::MovementHandler(): Poll()", res);
      }
    }

    res = IDirectInputDevice8_GetDeviceState(_lpDID[port], sizeof(DIJOYSTATE), &dims);
    if (res == DIERR_INPUTLOST)
    {
      DInputAcquire(port);

      if (LostCounter-- < 0)
      {
        DInputFailure("JoystickDriver::MovementHandler(): abort --", res);
        _failed = true;
        return true;
      }
    }
  } while (res == DIERR_INPUTLOST);

  if (res != DI_OK)
  {
    DInputFailure("JoystickDriver::MovementHandler(): GetDeviceState()", res);
    return true;
  }

  if (dims.rgbButtons[0] & 0x80)
  {
    *Button1 = true;
  }

  if (dims.rgbButtons[1] & 0x80)
  {
    *Button2 = true;
  }

  if (dims.rgbButtons[2] & 0x80)
  {
    if (gameport_fire0[port])
    {
      *Button1 = false;
    }
    else
    {
      *Button1 = true;
    }
  }

  if (dims.rgbButtons[3] & 0x80)
  {
    if (gameport_fire1[port])
    {
      *Button2 = false;
    }
    else
    {
      *Button2 = true;
    }
  }

  if (dims.lX != MEDX)
  {
    if (dims.lX > MEDX)
    {
      *Right = true;
    }
    else
    {
      *Left = true;
    }
  }

  if (dims.lY != MEDY)
  {
    if (dims.lY > MEDY)
    {
      *Down = true;
    }
    else
    {
      *Up = true;
    }
  }

  return false;
}

void JoystickDriver::MovementHandler()
{
  int joystickNo;

  if (_failed || !_in_use)
  {
    return;
  }

  for (int gameport = 0; gameport < 2; gameport++)
  {
    if ((gameport_input[gameport] == gameport_inputs::GP_ANALOG0) || (gameport_input[gameport] == gameport_inputs::GP_ANALOG1))
    {
      bool Button1;
      bool Button2;
      bool Left;
      bool Right;
      bool Up;
      bool Down;

      Button1 = Button2 = Left = Right = Up = Down = false;

      if (gameport_input[gameport] == gameport_inputs::GP_ANALOG0)
      {
        joystickNo = 0;
      }
      if (gameport_input[gameport] == gameport_inputs::GP_ANALOG1)
      {
        joystickNo = 1;
      }

      if (_lpDID[joystickNo] == nullptr)
      {
        return;
      }

      if (CheckJoyMovement(joystickNo, &Up, &Down, &Left, &Right, &Button1, &Button2))
      {
        Service->Log.AddLog("JoystickDriver::CheckJoyMovement failed\n");
        return;
      }

      if (gameport_left[gameport] != Left || gameport_right[gameport] != Right || gameport_up[gameport] != Up || gameport_down[gameport] != Down || gameport_fire0[gameport] != Button1 ||
          gameport_fire1[gameport] != Button2)
      {
        gameportJoystickHandler(gameport_input[gameport], Left, Up, Right, Down, Button1, Button2);
      }
    }
  }
}

void JoystickDriver::HardReset()
{
}

void JoystickDriver::EmulationStart()
{
  _failed = false;
  _focus = true;
  _active = false;
  _in_use = false;
  DInputInitialize();
}

void JoystickDriver::EmulationStop()
{
  DInputRelease();
  _failed = true;
}

JoystickDriver::JoystickDriver() : IJoystickDriver()
{
  _failed = false;
  _focus = true;
  _active = false;
  _in_use = false;

  _lpDI = nullptr;
  _lpDID[0] = nullptr;
  _lpDID[1] = nullptr;

  HRESULT res = CoInitialize(nullptr);
  if (res != S_OK)
  {
    Service->Log.AddLog("JoystickDriver::Startup(): Could not initialize COM library: %d\n", res);
  }

  num_joy_attached = 0;
}

JoystickDriver::~JoystickDriver()
{
  CoUninitialize();
}
