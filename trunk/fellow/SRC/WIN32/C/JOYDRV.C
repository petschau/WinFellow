/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Joystick driver for Windows                                               */
/* Author: Marco Nova (novamarco@hotmail.com)                                */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

#include "defs.h"
#include <windows.h>
#include "gameport.h"
#include "fellow.h"
#include "joydrv.h"
#include "windrv.h"

#define DIRECTINPUT_VERSION 0x0500
#include <dinput.h>


/*===========================================================================*/
/* Joystick device specific data                                             */
/*===========================================================================*/

BOOLE			joy_drv_failed;
IDirectInput		*joy_drv_lpDI;
IDirectInputDevice	*joy_drv_lpDID;
BOOLE                   joy_drv_active;
BOOLE			joy_drv_focus;
BOOLE			joy_drv_in_use;


/*==========================================================================*/
/* Returns textual error message. Adapted from DX SDK                       */
/*==========================================================================*/

STR *joyDrvDInputErrorString(HRESULT hResult) {
  switch (hResult) {
    case DI_OK:				return "The operation completed successfully.";
    case DI_BUFFEROVERFLOW:		return "The device buffer overflowed and some input was lost.";
    case DI_POLLEDDEVICE:		return "The device is a polled device.";
    case DIERR_ACQUIRED:		return "The operation cannot be performed while the device is acquired.";
    case DIERR_ALREADYINITIALIZED:	return "This object is already initialized.";
    case DIERR_BADDRIVERVER:		return "The object could not be created due to an incompatible driver version or mismatched or incomplete driver components.";
    case DIERR_BETADIRECTINPUTVERSION:	return "The application was written for an unsupported prerelease version of DirectInput.";
    case DIERR_DEVICENOTREG:		return "The device or device instance is not registered with DirectInput.";
    case DIERR_GENERIC:			return "An undetermined error occurred inside the DirectInput subsystem.";
    case DIERR_HANDLEEXISTS:		return "The device already has an event notification associated with it.";
    case DIERR_INPUTLOST:		return "Access to the input device has been lost. It must be re-acquired.";
    case DIERR_INVALIDPARAM:		return "An invalid parameter was passed to the returning function, or the object was not in a state that permitted the function to be called.";
    case DIERR_NOAGGREGATION:		return "This object does not support aggregation.";
    case DIERR_NOINTERFACE:		return "The specified interface is not supported by the object.";
    case DIERR_NOTACQUIRED:		return "The operation cannot be performed unless the device is acquired.";
    case DIERR_NOTINITIALIZED:		return "This object has not been initialized.";
    case DIERR_OBJECTNOTFOUND:		return "The requested object does not exist.";
    case DIERR_OLDDIRECTINPUTVERSION:	return "The application requires a newer version of DirectInput.";
    case DIERR_OUTOFMEMORY:		return "The DirectInput subsystem couldn't allocate sufficient memory to complete the caller's request.";
    case DIERR_UNSUPPORTED:		return "The function called is not supported at this time.";
    case E_PENDING:			return "Data is not yet available.";
  }
  return "Not a DirectInput Error";
}


/*==========================================================================*/
/* Logs a sensible error message                                            */
/*==========================================================================*/

void joyDrvDInputFailure(STR *header, HRESULT err) {
  fellowAddLog(header);
  fellowAddLog(joyDrvDInputErrorString(err));
  fellowAddLog("\n");
}


/*===========================================================================*/
/* Set joystick cooperative level                                            */
/*===========================================================================*/

void joyDrvDInputSetCooperativeLevel(void) {
  if (!joy_drv_failed) {
    HRESULT res = IDirectInputDevice_SetCooperativeLevel(joy_drv_lpDID,
							 gfx_drv_hwnd,
							 ((joy_drv_focus) ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE) | DISCL_FOREGROUND );
    if (res != DI_OK)
      joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): ", res );
  }
}


/*===========================================================================*/
/* Unacquire DirectInput joystick device                                     */
/*===========================================================================*/

void joyDrvDInputUnacquire(void) {
  if (!joy_drv_failed) {
    HRESULT res;
	
    if ((res = IDirectInputDevice_Unacquire( joy_drv_lpDID )))
      joyDrvDInputFailure("joyDrvDInputUnacquire(): ", res);
  }
}


/*===========================================================================*/
/* Acquire DirectInput joystick device                                       */
/*===========================================================================*/

void joyDrvDInputAcquire(void) {
  if (joy_drv_in_use) {
    HRESULT res;
		
    joyDrvDInputUnacquire();
    /* A new window is sometimes created, so set new hwnd cooperative level */
    joyDrvDInputSetCooperativeLevel();
    if ((res = IDirectInputDevice_Acquire( joy_drv_lpDID )) != DI_OK)
      joyDrvDInputFailure("joyDrvDInputAcquire(): ", res);
  }
}


/*===========================================================================*/
/* Initialize DirectInput for joystick                                       */
/*===========================================================================*/

void joyDrvDInputInitialize(void) {
  HRESULT res;
  fellowAddLog("joyDrvDInputInitialize()\n");

  if (!joy_drv_lpDI) {
    if ((res = DirectInputCreate(win_drv_hInstance,
				 DIRECTINPUT_VERSION,
				 &joy_drv_lpDI,
				 NULL )) != DI_OK) {
      joyDrvDInputFailure("joyDrvDInputInitialize(): DirectInputCreate() ", res );
      return;
    }
  }

  if (!joy_drv_lpDID) {
    res = IDirectInput_CreateDevice(joy_drv_lpDI,
				    &GUID_Joystick,
				    &joy_drv_lpDID,
				    NULL);
    if (res != DI_OK) {
      joyDrvDInputFailure("joyDrvDInputInitialize(): CreateDevice() ", res );
      joy_drv_failed = TRUE;
      return;
    }
  }

  res = IDirectInputDevice_SetDataFormat( joy_drv_lpDID, &c_dfDIJoystick );
  if (res != DI_OK) {
    joyDrvDInputFailure("joyDrvDInputInitialize(): SetDataFormat() ", res );
    joy_drv_failed = TRUE;
    return;
  }
  joyDrvDInputAcquire();
}


/*===========================================================================*/
/* Release DirectInput for joystick                                          */
/*===========================================================================*/

void joyDrvDInputRelease(void) {
  fellowAddLog("joyDrvDInputRelease()\n");
  if (joy_drv_lpDID) {
    joyDrvDInputUnacquire();
    IDirectInputDevice_Release( joy_drv_lpDID );
    joy_drv_lpDID = NULL;
  }

  if (joy_drv_lpDI) {
    IDirectInput_Release( joy_drv_lpDI );
    joy_drv_lpDI = NULL;
  }
}

 	
/*===========================================================================*/
/* joystick grab status has changed                                          */
/*===========================================================================*/

void joyDrvStateHasChanged(BOOLE active) {
  joy_drv_active = active;
  if (joy_drv_active && joy_drv_focus) {
    joy_drv_in_use = TRUE;
  }
  else {
    joy_drv_in_use = FALSE;
  }
  joyDrvDInputAcquire();
}


/*===========================================================================*/
/* joystick toggle focus                                                     */
/*===========================================================================*/

void joyDrvToggleFocus() {
  joy_drv_focus = !joy_drv_focus;
  joyDrvStateHasChanged(joy_drv_active);
}


/*===========================================================================*/
/* joystick movement handler                                                 */
/*===========================================================================*/

void joyDrvMovementHandler() {
  if (joy_drv_in_use)
    if (((gameport_input[0] == GP_ANALOG0) ||
	 (gameport_input[1] == GP_ANALOG0))) {
      DIJOYSTATE dims;
      BOOLE Button1 = FALSE;
      BOOLE Button2 = FALSE;
      BOOLE Button3 = FALSE;
      HRESULT res;

      do {
        res = IDirectInputDevice_GetDeviceState(joy_drv_lpDID,
						sizeof(DIJOYSTATE),
						&dims);
	if (res == DIERR_INPUTLOST) joyDrvDInputAcquire();
      } while (res == DIERR_INPUTLOST);

      if (res != DI_OK) {
	joyDrvDInputFailure("joyDrvMovementHandler(): GetDeviceState() ", res );
	return;
      }

      if (dims.rgbButtons[0] & 0x80) {
	fellowAddLog("Button1\n");
	Button1 = TRUE;
      }

      if (dims.rgbButtons[1] & 0x80) {
        fellowAddLog( "Button2\n");
	Button2 = TRUE;
      }

      if (dims.rgbButtons[2] & 0x80) {
	fellowAddLog( "Button3\n");
	Button3 = TRUE;
      }

      gameportJoyHandler( dims.lX, dims.lY, Button1, Button2, Button3 );
    }
}


/*===========================================================================*/
/* Hard Reset                                                                */
/*===========================================================================*/

void joyDrvHardReset(void) {
}


/*===========================================================================*/
/* Emulation Starting                                                        */
/*===========================================================================*/

void joyDrvEmulationStart(void) {
  joyDrvDInputInitialize();
}


/*===========================================================================*/
/* Emulation Stopping                                                        */
/*===========================================================================*/

void joyDrvEmulationStop(void) {
  joyDrvDInputRelease();
}


/*===========================================================================*/
/* Emulation Startup                                                         */
/*===========================================================================*/

void joyDrvStartup(void) {
  joy_drv_failed = FALSE;
  joy_drv_focus = TRUE;
  joy_drv_active = FALSE;
  joy_drv_in_use = FALSE;
  joy_drv_lpDI = NULL;
  joy_drv_lpDID = NULL;
}


/*===========================================================================*/
/* Emulation Shutdown                                                        */
/*===========================================================================*/

void joyDrvShutdown(void) {
}

