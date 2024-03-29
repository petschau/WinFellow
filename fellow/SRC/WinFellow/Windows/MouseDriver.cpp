/*=========================================================================*/
/* Fellow                                                                  */
/* Mouse driver for Windows                                                */
/* Author: Marco Nova (novamarco@hotmail.com)                              */
/*         Petter Schau (Some extensions)                                  */
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
- nothing is in list
*/

/* ---------------- CHANGE LOG -----------------
Monday, August 29, 2000
- added dxver.h include, dx version is decided with USE_DX3 or USE_DX5 macro

Sunday, July 9, 2000
- couldn't keep the mouse buttons pressed;
- removed some unuseful logging

Sunday, February 03, 2008: carfesh
- rebuild to use DirectInput8Create instead of DirectInputCreate
*/

/** @file
 * Mouse driver for Windows
 */

#include "Defs.h"
#include <windows.h>
#include "Gameports.h"
#include "FellowMain.h"
#include "MouseDriver.h"
#include "WindowsMain.h"
#include "GfxDrvCommon.h"
#include "VirtualHost/Core.h"

#include <initguid.h>
#include "dxver.h"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif

#define DINPUT_BUFFERSIZE 16

/*===========================================================================*/
/* Mouse specific data                                                       */
/*===========================================================================*/

LPDIRECTINPUT mouse_drv_lpDI = nullptr;
LPDIRECTINPUTDEVICE mouse_drv_lpDID = nullptr;
HANDLE mouse_drv_DIevent = nullptr;
BOOLE mouse_drv_focus;
BOOLE mouse_drv_active;
BOOLE mouse_drv_in_use;
BOOLE mouse_drv_initialization_failed;
bool mouse_drv_unacquired;
static BOOLE bLeftButton;
static BOOLE bRightButton;

int num_mouse_attached = 0;

/*==========================================================================*/
/* Returns textual error message. Adapted from DX SDK                       */
/*==========================================================================*/

const char *mouseDrvDInputErrorString(HRESULT hResult)
{
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
    case DIERR_HANDLEEXISTS:
      return "The device already has an event notification associated with it, or another app has a higher priority level, preventing this call from succeeding.";
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
  }
  return "Not a DirectInput Error";
}

const char *mouseDrvDInputUnaquireReturnValueString(HRESULT hResult)
{
  switch (hResult)
  {
    case DI_OK: return "The operation completed successfully.";
    case DI_NOEFFECT: return "The device was not in an acquired state.";
  }
  return "Not a known Unacquire() DirectInput return value.";
}

/*==========================================================================*/
/* Logs a sensible error message                                            */
/*==========================================================================*/

void mouseDrvDInputFailure(const char *header, HRESULT err)
{
  _core.Log->AddLog("%s %s\n", header, mouseDrvDInputErrorString(err));
}

void mouseDrvDInputUnacquireFailure(const char *header, HRESULT err)
{
  _core.Log->AddLog("%s %s\n", header, mouseDrvDInputUnaquireReturnValueString(err));
}

void mouseDrvDInputAcquireFailure(const char *header, HRESULT err)
{
  if (err == DI_NOEFFECT)
  {
    _core.Log->AddLog("%s %s\n", header, "The device was already in an acquired state.");
  }
  else
  {
    mouseDrvDInputFailure(header, err);
  }
}

/*===========================================================================*/
/* Acquire DirectInput mouse device                                          */
/*===========================================================================*/

void mouseDrvDInputAcquire()
{
  HRESULT res;

#ifdef _DEBUG
  _core.Log->AddLog("mouseDrvDInputAcquire(mouse_drv_in_use=%d, mouse_drv_lpDID=%d, mouse_drv_unacquired=%d)\n", mouse_drv_in_use, mouse_drv_lpDID, mouse_drv_unacquired);
#endif

  if (mouse_drv_in_use)
  {
    if (mouse_drv_lpDID != nullptr)
    {
#ifdef RETRO_PLATFORM
      // call moved here to ensure future compatibility with fullscreen/windowed RP modes
      if (RP.GetHeadlessMode())
      {
        if ((res = IDirectInputDevice_SetCooperativeLevel(mouse_drv_lpDID, RP.GetTopWindowHandle(), DISCL_EXCLUSIVE | DISCL_FOREGROUND)) != DI_OK)
          mouseDrvDInputAcquireFailure("mouseDrvDInputAcquire() - set cooperative level:", res);
      }
#endif

      if ((res = IDirectInputDevice_Acquire(mouse_drv_lpDID)) != DI_OK)
      {
        mouseDrvDInputAcquireFailure("mouseDrvDInputAcquire():", res);
      }
      else
      {
        mouse_drv_unacquired = false;
      }
    }
  }
  else
  {
    if (mouse_drv_lpDID != nullptr && !mouse_drv_unacquired)
    {
      mouse_drv_unacquired = true;
      if ((res = IDirectInputDevice_Unacquire(mouse_drv_lpDID)) != DI_OK)
      {
        // Should only "fail" if device is not acquired, it is not an error.
        mouseDrvDInputUnacquireFailure("mouseDrvDInputUnacquire():", res);
      }
    }
  }
}

/*===========================================================================*/
/* Release DirectInput for mouse                                             */
/*===========================================================================*/

void mouseDrvDInputRelease()
{
  if (mouse_drv_lpDID != nullptr)
  {
    IDirectInputDevice_Release(mouse_drv_lpDID);
    mouse_drv_lpDID = nullptr;
  }
  if (mouse_drv_DIevent != nullptr)
  {
    CloseHandle(mouse_drv_DIevent);
    mouse_drv_DIevent = nullptr;
  }
  if (mouse_drv_lpDI != nullptr)
  {
    IDirectInput_Release(mouse_drv_lpDI);
    mouse_drv_lpDI = nullptr;
  }
}

BOOL FAR PASCAL GetMouseInfo(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef)
{
  _core.Log->AddLog("**** mouse %d ****\n", num_mouse_attached++);
  return DIENUM_CONTINUE;
}

/*===========================================================================*/
/* Initialize DirectInput for mouse                                          */
/*===========================================================================*/

BOOLE mouseDrvDInputInitialize()
{
#define INITDIPROP(diprp, obj, how)                                                                                                                                           \
  {                                                                                                                                                                           \
    diprp.diph.dwSize = sizeof(diprp);                                                                                                                                        \
    diprp.diph.dwHeaderSize = sizeof(diprp.diph);                                                                                                                             \
    diprp.diph.dwObj = obj;                                                                                                                                                   \
    diprp.diph.dwHow = how;                                                                                                                                                   \
  }

  DIPROPDWORD dipdw = {
      {
          sizeof(DIPROPDWORD),  /* diph.dwSize */
          sizeof(DIPROPHEADER), /* diph.dwHeaderSize */
          0,                    /* diph.dwObj */
          DIPH_DEVICE,          /* diph.dwHow */
      },
      DINPUT_BUFFERSIZE /* dwData */
  };

  _core.Log->AddLog("mouseDrvDInputInitialize()\n");

  /* Create Direct Input object */

  mouse_drv_lpDI = nullptr;
  mouse_drv_lpDID = nullptr;
  mouse_drv_DIevent = nullptr;
  mouse_drv_initialization_failed = FALSE;
  mouse_drv_unacquired = true;

  HRESULT res = DirectInput8Create(win_drv_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&mouse_drv_lpDI, nullptr);
  if (res != DI_OK)
  {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): DirectInput8Create()", res);
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  num_mouse_attached = 0;
  res = IDirectInput_EnumDevices(mouse_drv_lpDI, DI8DEVTYPE_MOUSE, GetMouseInfo, NULL, DIEDFL_ALLDEVICES);
  if (res != DI_OK)
  {
    _core.Log->AddLog("Mouse Enum Devices failed %s\n", mouseDrvDInputErrorString(res));
  }

  /* Create Direct Input 1 mouse device */

  res = IDirectInput_CreateDevice(mouse_drv_lpDI, GUID_SysMouse, &mouse_drv_lpDID, NULL);
  if (res != DI_OK)
  {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): CreateDevice()", res);
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  /* Set data format for mouse device */

  res = IDirectInputDevice_SetDataFormat(mouse_drv_lpDID, &c_dfDIMouse);
  if (res != DI_OK)
  {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): SetDataFormat()", res);
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  /* Set cooperative level */
#ifdef RETRO_PLATFORM
  if (!RP.GetHeadlessMode())
  {
    res = IDirectInputDevice_SetCooperativeLevel(mouse_drv_lpDID, gfxDrvCommon->GetHWND(), DISCL_EXCLUSIVE | DISCL_FOREGROUND);
  }
  else
  {
    res = IDirectInputDevice_SetCooperativeLevel(mouse_drv_lpDID, RP.GetTopWindowHandle(), DISCL_EXCLUSIVE | DISCL_FOREGROUND);
  }
#endif
  if (res != DI_OK)
  {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): SetCooperativeLevel()", res);
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  /* Create event for notification */
  mouse_drv_DIevent = CreateEvent(nullptr, 0, 0, nullptr);
  if (mouse_drv_DIevent == nullptr)
  {
    _core.Log->AddLog("mouseDrvDInputInitialize(): CreateEvent() failed\n");
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  /* Set property for buffered data */
  res = IDirectInputDevice_SetProperty(mouse_drv_lpDID, DIPROP_BUFFERSIZE, &dipdw.diph);
  if (res != DI_OK)
  {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): SetProperty()", res);
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
  }

  /* Set event notification */
  res = IDirectInputDevice_SetEventNotification(mouse_drv_lpDID, mouse_drv_DIevent);
  if (res != DI_OK)
  {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): SetEventNotification()", res);
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  return TRUE;

#undef INITDIPROP
}

/*===========================================================================*/
/* Mouse grab status has changed                                             */
/*===========================================================================*/

void mouseDrvStateHasChanged(BOOLE active)
{
  mouse_drv_active = active;
  if (mouse_drv_active && mouse_drv_focus)
  {
    mouse_drv_in_use = TRUE;
  }
  else
  {
    mouse_drv_in_use = FALSE;
  }
  mouseDrvDInputAcquire();
}

/*===========================================================================*/
/* Mouse toggle focus                                                        */
/*===========================================================================*/

void mouseDrvToggleFocus()
{
  mouse_drv_focus = !mouse_drv_focus;
  mouseDrvStateHasChanged(mouse_drv_active);
#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode())
  {
    _core.Log->AddLog("mouseDrvToggleFocus(): mouse focus changed to to %s\n", mouse_drv_focus ? "true" : "false");
    RP.SendMouseCapture(mouse_drv_focus ? true : false);
  }
#endif
}

/**
 * Mouse set focus
 *
 * Used by the RetroPlatform module to control the mouse focus state;
 * the player is notified of the state change only if a change was not requested
 * by the player itself.
 * @callergraph
 */
void mouseDrvSetFocus(const BOOLE bNewFocus, const BOOLE bRequestedByRPHost)
{
  if (bNewFocus != mouse_drv_focus)
  {
    _core.Log->AddLog("mouseDrvSetFocus(bNewFocus=%s, bRequestedByRPHost=%s)\n", bNewFocus ? "true" : "false", bRequestedByRPHost ? "true" : "false");

    mouse_drv_focus = bNewFocus;
    mouseDrvStateHasChanged(mouse_drv_active);

#ifdef RETRO_PLATFORM
    if (RP.GetHeadlessMode())
      if (!bRequestedByRPHost)
      {
        _core.Log->AddLog("mouseDrvSetFocus(%s): notifiying, as not requested by host.\n", bNewFocus ? "true" : "false");
        RP.SendMouseCapture(mouse_drv_focus ? true : false);
      }
#endif
  }
}

/*===========================================================================*/
/* Mouse movement handler                                                    */
/*===========================================================================*/

void mouseDrvMovementHandler()
{
  if (mouse_drv_in_use)
  {
    static int32_t lx = 0;
    static int32_t ly = 0;
    HRESULT res;

    DIDEVICEOBJECTDATA rgod[DINPUT_BUFFERSIZE];
    DWORD itemcount = DINPUT_BUFFERSIZE;

    do
    {
      res = IDirectInputDevice_GetDeviceData(mouse_drv_lpDID, sizeof(DIDEVICEOBJECTDATA), rgod, &itemcount, 0);
      if (res != DI_OK)
      {
        mouseDrvDInputFailure("mouseDrvMovementHandler(): GetDeviceData()", res);
      }
      if (res == DIERR_INPUTLOST)
      {
        mouseDrvDInputAcquire();
      }
    } while (res == DIERR_INPUTLOST);

    if (res == DI_OK || res == DI_BUFFEROVERFLOW)
    {
      uint32_t i = 0;
      DWORD oldSequence = 0;

      /*
      ** Sometimes in a buffered input of a device, some simultaneous events are stored
      ** in different - but contigous - objects in the array. For every event object
      ** there is a sequence number - automatically handled by dinput - stored together
      ** with the event. Simultaneous events are assigned the same sequence number.
      ** Events are always placed in the buffer in chronological order.
      */

      if (itemcount == 0) // exit if there are no objects to examine
      {
        return;
      }

      lx = ly = 0;
      for (i = 0; i <= itemcount; i++)
      {
        if (i != 0)
        {
          if ((i == itemcount) ||                  // if there are no other objects
              (rgod[i].dwSequence != oldSequence)) // or the current objects is a different event
          {
            gameportMouseHandler(GP_MOUSE0, lx, ly, bLeftButton, FALSE, bRightButton);
            lx = ly = 0;
          }
          if (i == itemcount) // no other objects to examine, exit
          {
            break;
          }
        }
        oldSequence = rgod[i].dwSequence;

        switch (rgod[i].dwOfs)
        {
          case DIMOFS_BUTTON0: bLeftButton = (rgod[i].dwData & 0x80); break;

          case DIMOFS_BUTTON1: bRightButton = (rgod[i].dwData & 0x80); break;

          case DIMOFS_X: lx += rgod[i].dwData; break;

          case DIMOFS_Y: ly += rgod[i].dwData; break;
        }
      }
    }
  }
}

BOOLE mouseDrvGetFocus()
{
  return mouse_drv_focus;
}

/*===========================================================================*/
/* Hard Reset                                                                */
/*===========================================================================*/

void mouseDrvHardReset()
{
  _core.Log->AddLog("mouseDrvHardReset\n");
}

/*===========================================================================*/
/* Emulation Starting                                                        */
/*===========================================================================*/

BOOLE mouseDrvEmulationStart()
{
  _core.Log->AddLog("mouseDrvEmulationStart\n");
  return mouseDrvDInputInitialize();
}

/*===========================================================================*/
/* Emulation Stopping                                                        */
/*===========================================================================*/

void mouseDrvEmulationStop()
{
  _core.Log->AddLog("mouseDrvEmulationStop\n");
  mouseDrvDInputRelease();
}

/*===========================================================================*/
/* Emulation Startup                                                         */
/*===========================================================================*/

void mouseDrvStartup()
{
  _core.Log->AddLog("mouseDrvStartup\n");
  mouse_drv_active = FALSE;
  mouse_drv_focus = TRUE;
  mouse_drv_in_use = FALSE;
  mouse_drv_initialization_failed = TRUE;
  mouse_drv_unacquired = true;
  mouse_drv_lpDI = nullptr;
  mouse_drv_lpDID = nullptr;
  mouse_drv_DIevent = nullptr;
  bLeftButton = FALSE;
  bRightButton = FALSE;
}

/*===========================================================================*/
/* Emulation Shutdown                                                        */
/*===========================================================================*/

void mouseDrvShutdown()
{
  _core.Log->AddLog("mouseDrvShutdown\n");
}
