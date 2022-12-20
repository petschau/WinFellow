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

/** @file
 * Mouse driver for Windows
 */

#include "fellow/api/defs.h"
#include "fellow/application/Gameport.h"
#include "fellow/api/Services.h"
#include "fellow/os/windows/io/MouseDriver.h"
#include "fellow/os/windows/application/WindowsDriver.h"
#include "fellow/os/windows/graphics/GfxDrvCommon.h"

#include <initguid.h>

#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#endif

using namespace fellow::api;

#define DINPUT_BUFFERSIZE 16

const char *MouseDriver::DInputErrorString(HRESULT hResult)
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
    case DIERR_HANDLEEXISTS: return "The device already has an event notification associated with it, or another app has a higher priority level, preventing this call from succeeding.";
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

const char *MouseDriver::DInputUnaquireReturnValueString(HRESULT hResult)
{
  switch (hResult)
  {
    case DI_OK: return "The operation completed successfully.";
    case DI_NOEFFECT: return "The device was not in an acquired state.";
  }

  return "Not a known Unacquire() DirectInput return value.";
}

void MouseDriver::DInputFailure(const char *header, HRESULT err)
{
  Service->Log.AddLog("%s %s\n", header, DInputErrorString(err));
}

void MouseDriver::DInputUnacquireFailure(const char *header, HRESULT err)
{
  Service->Log.AddLog("%s %s\n", header, DInputUnaquireReturnValueString(err));
}

void MouseDriver::DInputAcquireFailure(const char *header, HRESULT err)
{
  if (err == DI_NOEFFECT)
  {
    Service->Log.AddLog("%s %s\n", header, "The device was already in an acquired state.");
  }
  else
  {
    DInputFailure(header, err);
  }
}

void MouseDriver::DInputAcquire()
{
  if (_in_use)
  {
    if (_lpDID != nullptr)
    {
      HRESULT res = IDirectInputDevice_Acquire(_lpDID);
      if (res != DI_OK)
      {
        DInputAcquireFailure("MouseDriver::DInputAcquire():", res);
      }
      else
      {
        _unacquired = false;
      }
    }
  }
  else
  {
    if (_lpDID != nullptr && !_unacquired)
    {
      _unacquired = true;
      HRESULT res = IDirectInputDevice_Unacquire(_lpDID);
      if (res != DI_OK)
      {
        // Should only "fail" if device is not acquired, it is not an error.
        DInputUnacquireFailure("MouseDriver::DInputUnacquire():", res);
      }
    }
  }
}

void MouseDriver::DInputRelease()
{
  if (_lpDID != nullptr)
  {
    IDirectInputDevice_Release(_lpDID);
    _lpDID = nullptr;
  }
  if (_DIevent != nullptr)
  {
    CloseHandle(_DIevent);
    _DIevent = nullptr;
  }
  if (_lpDI != nullptr)
  {
    IDirectInput_Release(_lpDI);
    _lpDI = nullptr;
  }
}

BOOL FAR PASCAL MouseDriver::GetMouseInfoCallback(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef)
{
  return ((MouseDriver *)pvRef)->HandleGetMouseInfoCallback();
}

BOOL MouseDriver::HandleGetMouseInfoCallback()
{
  Service->Log.AddLog("**** mouse %d ****\n", _num_mouse_attached++);
  return DIENUM_CONTINUE;
}

BOOLE MouseDriver::DInputInitialize()
{
#define INITDIPROP(diprp, obj, how)                                                                                                                                                                    \
  {                                                                                                                                                                                                    \
    diprp.diph.dwSize = sizeof(diprp);                                                                                                                                                                 \
    diprp.diph.dwHeaderSize = sizeof(diprp.diph);                                                                                                                                                      \
    diprp.diph.dwObj = obj;                                                                                                                                                                            \
    diprp.diph.dwHow = how;                                                                                                                                                                            \
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

  Service->Log.AddLog("MouseDriver::DInputInitialize()\n");

  /* Create Direct Input object */

  _lpDI = nullptr;
  _lpDID = nullptr;
  _DIevent = nullptr;
  _initialization_failed = FALSE;
  _unacquired = true;

  HRESULT res = DirectInput8Create(win_drv_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&_lpDI, nullptr);
  if (res != DI_OK)
  {
    DInputFailure("MouseDriver::DInputInitialize(): DirectInput8Create()", res);
    _initialization_failed = TRUE;
    DInputRelease();
    return FALSE;
  }

  _num_mouse_attached = 0;
  res = IDirectInput_EnumDevices(_lpDI, DI8DEVTYPE_MOUSE, GetMouseInfoCallback, this, DIEDFL_ALLDEVICES);
  if (res != DI_OK)
  {
    Service->Log.AddLog("MouseDriver enumerate devices failed %s\n", DInputErrorString(res));
  }

  /* Create Direct Input 1 mouse device */

  res = IDirectInput_CreateDevice(_lpDI, GUID_SysMouse, &_lpDID, NULL);
  if (res != DI_OK)
  {
    DInputFailure("MouseDriver::DInputInitialize(): CreateDevice()", res);
    _initialization_failed = TRUE;
    DInputRelease();
    return FALSE;
  }

  /* Set data format for mouse device */

  res = IDirectInputDevice_SetDataFormat(_lpDID, &c_dfDIMouse);
  if (res != DI_OK)
  {
    DInputFailure("MouseDriver::DInputInitialize(): SetDataFormat()", res);
    _initialization_failed = TRUE;
    DInputRelease();
    return FALSE;
  }

  /* Set cooperative level */
  res = IDirectInputDevice_SetCooperativeLevel(_lpDID, gfxDrvCommon->GetHWND(), DISCL_EXCLUSIVE | DISCL_FOREGROUND);
  if (res != DI_OK)
  {
    DInputFailure("MouseDriver::DInputInitialize(): SetCooperativeLevel()", res);
    _initialization_failed = TRUE;
    DInputRelease();
    return FALSE;
  }

  /* Create event for notification */
  _DIevent = CreateEvent(nullptr, 0, 0, nullptr);
  if (_DIevent == nullptr)
  {
    Service->Log.AddLog("MouseDriver::DInputInitialize(): CreateEvent() failed\n");
    _initialization_failed = TRUE;
    DInputRelease();
    return FALSE;
  }

  /* Set property for buffered data */
  res = IDirectInputDevice_SetProperty(_lpDID, DIPROP_BUFFERSIZE, &dipdw.diph);
  if (res != DI_OK)
  {
    DInputFailure("MouseDriver::DInputInitialize(): SetProperty()", res);
    _initialization_failed = TRUE;
    DInputRelease();
  }

  /* Set event notification */
  res = IDirectInputDevice_SetEventNotification(_lpDID, _DIevent);
  if (res != DI_OK)
  {
    DInputFailure("MouseDriver::DInputInitialize(): SetEventNotification()", res);
    _initialization_failed = TRUE;
    DInputRelease();
    return FALSE;
  }

  return TRUE;

#undef INITDIPROP
}

//==============================
// Mouse grab status has changed
//==============================

void MouseDriver::StateHasChanged(BOOLE active)
{
  _active = active;
  if (_active && _focus)
  {
    _in_use = TRUE;
  }
  else
  {
    _in_use = FALSE;
  }
  DInputAcquire();
}

//===================
// Mouse toggle focus
//===================

void MouseDriver::ToggleFocus()
{
  _focus = !_focus;
  StateHasChanged(_active);
#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode())
  {
    Service->Log.AddLog("MouseDriver::ToggleFocus(): mouse focus changed to to %s\n", _focus ? "true" : "false");
    RP.SendMouseCapture(_focus ? true : false);
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
void MouseDriver::SetFocus(const BOOLE bNewFocus, const BOOLE bRequestedByRPHost)
{
  if (bNewFocus != _focus)
  {
    Service->Log.AddLog("MouseDriver::SetFocus(%s)\n", bNewFocus ? "true" : "false");

    _focus = bNewFocus;
    StateHasChanged(_active);

#ifdef RETRO_PLATFORM
    if (RP.GetHeadlessMode())
      if (!bRequestedByRPHost)
      {
        Service->Log.AddLog("MouseDriver::SetFocus(%s): notifiying, as not requested by host.\n", bNewFocus ? "true" : "false");
        RP.SendMouseCapture(_focus ? true : false);
      }
#endif
  }
}

//=======================
// Mouse movement handler
//=======================

void MouseDriver::MovementHandler()
{
  if (_in_use)
  {
    static LON lx = 0;
    static LON ly = 0;
    HRESULT res{};

    DIDEVICEOBJECTDATA rgod[DINPUT_BUFFERSIZE]{};
    DWORD itemcount = DINPUT_BUFFERSIZE;

    do
    {
      res = IDirectInputDevice_GetDeviceData(_lpDID, sizeof(DIDEVICEOBJECTDATA), rgod, &itemcount, 0);
      if (res != DI_OK)
      {
        DInputFailure("MouseDriver::MovementHandler(): GetDeviceData()", res);
      }
      if (res == DIERR_INPUTLOST)
      {
        DInputAcquire();
      }
    } while (res == DIERR_INPUTLOST);

    if (res == DI_OK || res == DI_BUFFEROVERFLOW)
    {
      ULO i = 0;
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
            gameportMouseHandler(gameport_inputs::GP_MOUSE0, lx, ly, bLeftButton, FALSE, bRightButton);
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

BOOLE MouseDriver::GetFocus()
{
  return _focus;
}

void MouseDriver::HardReset()
{
  Service->Log.AddLog("MouseDriver::HardReset()\n");
}

BOOLE MouseDriver::EmulationStart()
{
  Service->Log.AddLog("MouseDriver::EmulationStart()\n");
  return DInputInitialize();
}

void MouseDriver::EmulationStop()
{
  Service->Log.AddLog("MouseDriver::EmulationStop()\n");
  DInputRelease();
}

void MouseDriver::Startup()
{
  Service->Log.AddLog("MouseDriver::Startup()\n");
  _active = FALSE;
  _focus = TRUE;
  _in_use = FALSE;
  _initialization_failed = TRUE;
  _unacquired = true;
  _lpDI = nullptr;
  _lpDID = nullptr;
  _DIevent = nullptr;
  bLeftButton = FALSE;
  bRightButton = FALSE;
}

void MouseDriver::Shutdown()
{
  Service->Log.AddLog("MouseDriver::Shutdown()\n");
}
