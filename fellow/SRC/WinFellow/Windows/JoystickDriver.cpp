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
/* ---------------- KNOWN BUGS/FIXLIST -----------------
- additional button support for other emulator functions
- better autofire support trough gameport_autofire arrays
*/

/* ---------------- CHANGE LOG -----------------
Saturday, November 04, 2000
- trying to fix two joy support

Monday, October 2, 2000
- added support for two joystick

Monday, September 18, 2000
- removed calls to CoInitialize and CoUninitialize in joyDrvStartup with Dx3 because they're not used

Sunday, September 17, 2000
- removed some distinction between Dx3 and Dx5
- added multimedia code to use with NT (Nt4 or Nt2k)
- you can now compile with Dx3 settings and it works regarding the OS.
- if you compile with USE_DX5, the multimedia code is used only with Nt4
- added autofire support trough button 3 and 4

Monday, September 11, 2000
- fixed joyDrvMovementHandler if the input is lost for 50 times consecutives, the driver is marked as failed and a new joyDrvEmulationStart should be issued
- fixed joyDrvEmulationStart, some internal variables were initialized only at startup (joyDrvStartup)

Thursday, September 07, 2000
- now use multimedia functions with dx3 mode (instead of pure dinput)
- removed heavy debug log with Dx3

Tuesday, September 05, 2000
- tried to use the CoCreateInstance also for Dx3
- added some heavy output to the log with Dx3 in debug mode

Monday, August 29, 2000
- added dxver.h include, dx version is decided with USE_DX3 or USE_DX5 macro

Monday, July 10, 2000
- conditional compile for DX5 or DX3
- use of the couple CoCreateInstance and Initialize instead of DirectInputCreateEx and CreateDeviceEx with DX5
- added CoInitialize in joyDrvStartup and CoUninitialize in joyDrvShutdown with DX5
- added some more error explanation in Debug compile into joyDrvDInputErrorString when not a DxError encountered (it leaks some memory)

Monday, February 04, 2008
- DirectX November 2007 SDK fixes, DirectInput8
- remove DirectX 3 and NT support

*/

#include "Defs.h"
#include <windows.h>
#include "Gameports.h"
#include "FellowMain.h"
#include "JoystickDriver.h"
#include "WindowsMain.h"
#include "GfxDrvCommon.h"
#include "VirtualHost/Core.h"

#include "dxver.h"
#include <mmsystem.h>

#ifdef _DEBUG
#define JOYDRVDEBUG
#endif

/*===========================================================================*/
/* Joystick device specific data                                             */
/*===========================================================================*/

#define MAX_JOY_PORT 2

BOOLE joy_drv_failed;

IDirectInput8 *joy_drv_lpDI;
IDirectInputDevice8 *joy_drv_lpDID[MAX_JOY_PORT];

int num_joy_supported;
int num_joy_attached;

BOOLE joy_drv_active;
BOOLE joy_drv_focus;
BOOLE joy_drv_in_use;

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

/*==========================================================================*/
/* Returns textual error message. Adapted from DX SDK                       */
/*==========================================================================*/

const char *joyDrvDInputErrorString(HRESULT hResult)
{
#ifdef _DEBUG
  char *UnErr = nullptr;
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
  UnErr = (char *)malloc(255 * sizeof(char));
  sprintf(UnErr, "Not a DirectInput Error [%08x]", hResult);
  return UnErr;
#else
  return "Not a DirectInput Error";
#endif
}

/*==========================================================================*/
/* Logs a sensible error message                                            */
/*==========================================================================*/

void joyDrvDInputFailure(const char *header, HRESULT err)
{
  _core.Log->AddLog("%s %s\n", header, joyDrvDInputErrorString(err));
}

/*===========================================================================*/
/* Set joystick cooperative level                                            */
/*===========================================================================*/

void joyDrvDInputSetCooperativeLevel(int port)
{
  DIPROPRANGE diprg;
  DIPROPDWORD dipdw;

  _core.Log->AddLog("joyDrvDInputSetCooperativeLevel(%d)\n", port);

  if (joy_drv_failed)
  {
    return;
  }

  HRESULT res =
      IDirectInputDevice8_SetCooperativeLevel(joy_drv_lpDID[port], gfxDrvCommon->GetHWND(), ((joy_drv_focus) ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE) | DISCL_FOREGROUND);
  if (res != DI_OK)
  {
    joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel():", res);
  }

#define INITDIPROP(diprp, obj, how)                                                                                                                                           \
  {                                                                                                                                                                           \
    diprp.diph.dwSize = sizeof(diprp);                                                                                                                                        \
    diprp.diph.dwHeaderSize = sizeof(diprp.diph);                                                                                                                             \
    diprp.diph.dwObj = obj;                                                                                                                                                   \
    diprp.diph.dwHow = how;                                                                                                                                                   \
  }

  INITDIPROP(diprg, DIJOFS_X, DIPH_BYOFFSET)
  diprg.lMin = MINX;
  diprg.lMax = MAXX;

  res = IDirectInputDevice8_SetProperty(joy_drv_lpDID[port], DIPROP_RANGE, &diprg.diph);
  if (res != DI_OK)
  {
    joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): SetProperty RANGE X :", res);
  }

  INITDIPROP(diprg, DIJOFS_Y, DIPH_BYOFFSET)
  diprg.lMin = MINY;
  diprg.lMax = MAXY;

  res = IDirectInputDevice8_SetProperty(joy_drv_lpDID[port], DIPROP_RANGE, &diprg.diph);
  if (res != DI_OK)
  {
    joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): SetProperty RANGE Y :", res);
  }

  INITDIPROP(dipdw, DIJOFS_X, DIPH_BYOFFSET)
  dipdw.dwData = DEADX;

  res = IDirectInputDevice8_SetProperty(joy_drv_lpDID[port], DIPROP_DEADZONE, &dipdw.diph);
  if (res != DI_OK)
  {
    joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): SetProperty DEADZONE X :", res);
  }

  INITDIPROP(dipdw, DIJOFS_Y, DIPH_BYOFFSET)
  dipdw.dwData = DEADY;

  res = IDirectInputDevice8_SetProperty(joy_drv_lpDID[port], DIPROP_DEADZONE, &dipdw.diph);
  if (res != DI_OK)
  {
    joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): SetProperty DEADZONE Y :", res);
  }

#undef INITDIPROP
}

/*===========================================================================*/
/* Unacquire DirectInput joystick device                                     */
/*===========================================================================*/

void joyDrvDInputUnacquire(int port)
{
  if (gameportGetAnalogJoystickInUse())
  {
    _core.Log->AddLog("joyDrvDInputUnacquire(%d)\n", port);

    if (!joy_drv_failed)
    {
      HRESULT res = IDirectInputDevice8_Unacquire(joy_drv_lpDID[port]);
      if (res != DI_OK)
      {
        joyDrvDInputFailure("joyDrvDInputUnacquire():", res);
      }
    }
  }
}

/*===========================================================================*/
/* Acquire DirectInput joystick device                                       */
/*===========================================================================*/

void joyDrvDInputAcquire(int port)
{
  if (gameportGetAnalogJoystickInUse())
  {
    _core.Log->AddLog("joyDrvDInputAcquire(%d)\n", port);

    if (joy_drv_in_use)
    {
      // joyDrvDInputUnacquire(port);

      /* A new window is sometimes created, so set new hwnd cooperative level */
      joyDrvDInputSetCooperativeLevel(port);

      HRESULT res = IDirectInputDevice8_Acquire(joy_drv_lpDID[port]);
      if (res != DI_OK)
      {
        joyDrvDInputFailure("joyDrvDInputAcquire():", res);
      }
    }
  }
}

BOOLE joyDrvDxCreateAndInitDevice(IDirectInput8 *pDi, IDirectInputDevice8 *pDiD[], GUID guid, int port)
{
  HRESULT res;

  if (!pDiD[port])
  {
    res = CoCreateInstance(CLSID_DirectInputDevice8, nullptr, CLSCTX_INPROC_SERVER, IID_IDirectInputDevice8, (LPVOID *)&(pDiD[port]));
    if (res != DI_OK)
    {
      joyDrvDInputFailure("joyDrvDInputInitialize(): DeviceCoCreateInstance()", res);
      return TRUE;
    }

    res = IDirectInputDevice8_Initialize(pDiD[port], win_drv_hInstance, DIRECTINPUT_VERSION, guid);
    if (res != DI_OK)
    {
      joyDrvDInputFailure("joyDrvDInputInitialize(): DeviceInitialize()", res);
      return TRUE;
    }
  }

  res = IDirectInputDevice8_SetDataFormat(pDiD[port], &c_dfDIJoystick);
  if (res != DI_OK)
  {
    joyDrvDInputFailure("joyDrvDInputInitialize(): SetDataFormat()", res);
    return TRUE;
  }

  return FALSE;
} // joyDrvDxCreateAndInitDevice

BOOL FAR PASCAL joyDrvInitJoystickInput(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef)
{
  IDirectInput8 *pdi = (IDirectInput8 *)pvRef;

  _core.Log->AddLog("**** Joystick %d **** '%s'\n", num_joy_attached, pdinst->tszProductName);

  if (!joyDrvDxCreateAndInitDevice(pdi, joy_drv_lpDID, pdinst->guidInstance, num_joy_attached++))
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

/*===========================================================================*/
/* Initialize DirectInput for joystick                                       */
/*===========================================================================*/

void joyDrvDInputInitialize()
{
  _core.Log->AddLog("joyDrvDInputInitialize()\n");

  if (!joy_drv_lpDI)
  {
    HRESULT res = CoCreateInstance(CLSID_DirectInput8, nullptr, CLSCTX_INPROC_SERVER, IID_IDirectInput8, (LPVOID *)&joy_drv_lpDI);
    if (res != DI_OK)
    {
      joyDrvDInputFailure("joyDrvDInputInitialize(): CoCreateInstance()", res);
      joy_drv_failed = TRUE;
      return;
    }

    res = IDirectInput8_Initialize(joy_drv_lpDI, win_drv_hInstance, DIRECTINPUT_VERSION);
    if (res != DI_OK)
    {
      joyDrvDInputFailure("joyDrvDInputInitialize(): Initialize()", res);
      joy_drv_failed = TRUE;
      return;
    }

    num_joy_attached = 0;

    res = IDirectInput8_EnumDevices(joy_drv_lpDI, DI8DEVCLASS_GAMECTRL, joyDrvInitJoystickInput, joy_drv_lpDI, DIEDFL_ATTACHEDONLY);
    if (res != DI_OK)
    {
      joyDrvDInputFailure("joyDrvDInputInitialize(): EnumDevices()", res);
      joy_drv_failed = TRUE;
      return;
    }

    _core.Log->AddLog("njoy: %d\n", num_joy_attached);
  }
}

/*===========================================================================*/
/* Release DirectInput for joystick                                          */
/*===========================================================================*/

void joyDrvDInputRelease()
{
  _core.Log->AddLog("joyDrvDInputRelease()\n");

  for (int port = 0; port < MAX_JOY_PORT; port++)
  {
    if (joy_drv_lpDID[port] != nullptr)
    {
      joyDrvDInputUnacquire(port);
      IDirectInputDevice8_Release(joy_drv_lpDID[port]);
      joy_drv_lpDID[port] = nullptr;
    }
  }

  if (joy_drv_lpDI != nullptr)
  {
    IDirectInput8_Release(joy_drv_lpDI);
    joy_drv_lpDI = nullptr;
  }
}

/*===========================================================================*/
/* joystick grab status has changed                                          */
/*===========================================================================*/

void joyDrvStateHasChanged(BOOLE active)
{
  if (joy_drv_failed)
  {
    return;
  }

  joy_drv_active = active;
  if (joy_drv_active && joy_drv_focus)
  {
    joy_drv_in_use = TRUE;
  }
  else
  {
    joy_drv_in_use = FALSE;
  }

  for (int port = 0; port < MAX_JOY_PORT; port++)
  {
    if (joy_drv_lpDID[port] != nullptr)
    {
      joyDrvDInputAcquire(port);
    }
  }
}

/*===========================================================================*/
/* joystick toggle focus                                                     */
/*===========================================================================*/

void joyDrvToggleFocus()
{
  joy_drv_focus = !joy_drv_focus;
  joyDrvStateHasChanged(joy_drv_active);
}

BOOLE joyDrvCheckJoyMovement(int port, BOOLE *Up, BOOLE *Down, BOOLE *Left, BOOLE *Right, BOOLE *Button1, BOOLE *Button2)
{
  DIJOYSTATE dims;
  HRESULT res;
  int LostCounter = 25;

  *Button1 = *Button2 = *Left = *Right = *Up = *Down = FALSE;

  do
  {
    res = IDirectInputDevice8_Poll(joy_drv_lpDID[port]);
    if (res != DI_OK)
    {
      if (res != DI_BUFFEROVERFLOW)
      {
        joyDrvDInputFailure("joyDrvMovementHandler(): Poll()", res);
      }
    }

    res = IDirectInputDevice8_GetDeviceState(joy_drv_lpDID[port], sizeof(DIJOYSTATE), &dims);
    if (res == DIERR_INPUTLOST)
    {
      joyDrvDInputAcquire(port);

      if (LostCounter-- < 0)
      {
        joyDrvDInputFailure("joyDrvMovementHandler(): abort --", res);
        joy_drv_failed = TRUE;
        return TRUE;
      }
    }
  } while (res == DIERR_INPUTLOST);

  if (res != DI_OK)
  {
    joyDrvDInputFailure("joyDrvMovementHandler(): GetDeviceState()", res);
    return TRUE;
  }

  if (dims.rgbButtons[0] & 0x80)
  {
    *Button1 = TRUE;
  }

  if (dims.rgbButtons[1] & 0x80)
  {
    *Button2 = TRUE;
  }

  if (dims.rgbButtons[2] & 0x80)
  {
    if (gameport_fire0[port])
    {
      *Button1 = FALSE;
    }
    else
    {
      *Button1 = TRUE;
    }
  }

  if (dims.rgbButtons[3] & 0x80)
  {
    if (gameport_fire1[port])
    {
      *Button2 = FALSE;
    }
    else
    {
      *Button2 = TRUE;
    }
  }

  if (dims.lX != MEDX)
  {
    if (dims.lX > MEDX)
    {
      *Right = TRUE;
    }
    else
    {
      *Left = TRUE;
    }
  }

  if (dims.lY != MEDY)
  {
    if (dims.lY > MEDY)
    {
      *Down = TRUE;
    }
    else
    {
      *Up = TRUE;
    }
  }

  return FALSE;
} // CheckJoyMovement

/*===========================================================================*/
/* joystick movement handler                                                 */
/*===========================================================================*/

void joyDrvMovementHandler()
{
  int joystickNo;

  if (joy_drv_failed || !joy_drv_in_use)
  {
    return;
  }

  for (int gameport = 0; gameport < 2; gameport++)
  {
    if ((gameport_input[gameport] == GP_ANALOG0) || (gameport_input[gameport] == GP_ANALOG1))
    {
      BOOLE Button1;
      BOOLE Button2;
      BOOLE Left;
      BOOLE Right;
      BOOLE Up;
      BOOLE Down;

      Button1 = Button2 = Left = Right = Up = Down = FALSE;

      if (gameport_input[gameport] == GP_ANALOG0)
      {
        joystickNo = 0;
      }
      if (gameport_input[gameport] == GP_ANALOG1)
      {
        joystickNo = 1;
      }

      if (joy_drv_lpDID[joystickNo] == nullptr)
      {
        return;
      }

      if (joyDrvCheckJoyMovement(joystickNo, &Up, &Down, &Left, &Right, &Button1, &Button2))
      {
        _core.Log->AddLog("joyDrvCheckJoyMovement failed\n");
        return;
      }

      if (gameport_left[gameport] != Left || gameport_right[gameport] != Right || gameport_up[gameport] != Up || gameport_down[gameport] != Down ||
          gameport_fire0[gameport] != Button1 || gameport_fire1[gameport] != Button2)
      {
        gameportJoystickHandler(gameport_input[gameport], Left, Up, Right, Down, Button1, Button2);
      }
    }
  }
}

/*===========================================================================*/
/* Hard Reset                                                                */
/*===========================================================================*/

void joyDrvHardReset()
{
}

/*===========================================================================*/
/* Emulation Starting                                                        */
/*===========================================================================*/

void joyDrvEmulationStart()
{
  joy_drv_failed = FALSE;
  joy_drv_focus = TRUE;
  joy_drv_active = FALSE;
  joy_drv_in_use = FALSE;
  joyDrvDInputInitialize();
}

/*===========================================================================*/
/* Emulation Stopping                                                        */
/*===========================================================================*/

void joyDrvEmulationStop()
{
  joyDrvDInputRelease();
  joy_drv_failed = TRUE;
}

/*===========================================================================*/
/* Emulation Startup                                                         */
/*===========================================================================*/

void joyDrvStartup()
{
  joy_drv_failed = FALSE;
  joy_drv_focus = TRUE;
  joy_drv_active = FALSE;
  joy_drv_in_use = FALSE;

  joy_drv_lpDI = nullptr;
  joy_drv_lpDID[0] = nullptr;
  joy_drv_lpDID[1] = nullptr;

  HRESULT res = CoInitialize(nullptr);
  if (res != S_OK)
  {
    _core.Log->AddLog("joyDrvStartup(): Could not initialize COM library: %d\n", res);
  }

  num_joy_supported = 0;
  num_joy_attached = 0;
}

/*===========================================================================*/
/* Emulation Shutdown                                                        */
/*===========================================================================*/

void joyDrvShutdown()
{
  CoUninitialize();
}
