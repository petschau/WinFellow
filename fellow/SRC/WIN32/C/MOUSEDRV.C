/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Mouse driver for Windows                                                  */
/* Author: Marco Nova (novamarco@hotmail.com)                                */
/*         Petter Schau (peschau@online.no) (Some extensions)                */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

#include "defs.h"
#include <windows.h>
#include "gameport.h"
#include "fellow.h"
#include "mousedrv.h"
#include "windrv.h"

#define DIRECTINPUT_VERSION 0x0300
#include <initguid.h>
#include <dinput.h>

#define DINPUT_BUFFERSIZE           16


/*===========================================================================*/
/* Mouse specific data                                                       */
/*===========================================================================*/

LPDIRECTINPUT		mouse_drv_lpDI = NULL;
LPDIRECTINPUTDEVICE	mouse_drv_lpDID = NULL;
HANDLE			mouse_drv_DIevent = NULL;
BOOLE			mouse_drv_focus;
BOOLE			mouse_drv_active;
BOOLE			mouse_drv_in_use;
BOOLE			mouse_drv_initialization_failed;
BOOLE			mouse_drv_left_button;
BOOLE			mouse_drv_right_button;
ULO			mouse_drv_x;
ULO			mouse_drv_y;


/*==========================================================================*/
/* Returns textual error message. Adapted from DX SDK                       */
/*==========================================================================*/

STR *mouseDrvDInputErrorString(HRESULT hResult) {
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

void mouseDrvDInputFailure(STR *header, HRESULT err) {
  fellowAddLog(header);
  fellowAddLog(mouseDrvDInputErrorString(err));
  fellowAddLog("\n");
}


/*===========================================================================*/
/* Acquire DirectInput mouse device                                          */
/*===========================================================================*/

void mouseDrvDInputAcquire(void) {
  HRESULT res;

  if (mouse_drv_in_use) {
    if (mouse_drv_lpDID != NULL) {
      if ((res = IDirectInputDevice_Acquire(mouse_drv_lpDID)) != DI_OK)
        mouseDrvDInputFailure("mouseDrvDInputAcquire(): ", res);
    }
  }
  else {
    if (mouse_drv_lpDID != NULL) {
      if ((res = IDirectInputDevice_Unacquire(mouse_drv_lpDID)) != DI_OK)
        mouseDrvDInputFailure("mouseDrvDInputUnacquire(): ", res);
    }
  }
}


/*===========================================================================*/
/* Release DirectInput for mouse                                             */
/*===========================================================================*/

void mouseDrvDInputRelease(void) {
  if (mouse_drv_lpDID != NULL) {
    IDirectInputDevice_Release(mouse_drv_lpDID);
    mouse_drv_lpDID = NULL;
  }
  if (mouse_drv_DIevent != NULL) {
    CloseHandle(mouse_drv_DIevent);
    mouse_drv_DIevent = NULL;
  }
  if (mouse_drv_lpDI != NULL) {
    IDirectInput_Release(mouse_drv_lpDI);
    mouse_drv_lpDI = NULL;
  }
}

	
/*===========================================================================*/
/* Initialize DirectInput for mouse                                          */
/*===========================================================================*/

BOOLE mouseDrvDInputInitialize(void) {
  HRESULT res;
  DIPROPDWORD dipdw =
  {
    {
      sizeof(DIPROPDWORD),        /* diph.dwSize */
      sizeof(DIPROPHEADER),       /* diph.dwHeaderSize */
      0,                          /* diph.dwObj */
      DIPH_DEVICE,                /* diph.dwHow */
    },
    DINPUT_BUFFERSIZE,            /* dwData */
  };
  
  /* Create Direct Input object */
  
  mouse_drv_lpDI = NULL;
  mouse_drv_lpDID = NULL;
  mouse_drv_DIevent = NULL;
  mouse_drv_initialization_failed = FALSE;
  if ((res = DirectInputCreate(win_drv_hInstance,
                               DIRECTINPUT_VERSION,
                               &mouse_drv_lpDI,
                               NULL)) != DI_OK) {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): DirectInputCreate() ", res );
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }
  
  /* Create Direct Input 1 mouse device */
  
  if ((res = IDirectInput_CreateDevice(mouse_drv_lpDI,
                                       &GUID_SysMouse,
                                       &mouse_drv_lpDID,
                                       NULL)) != DI_OK) {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): CreateDevice() ", res );
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }
  
  /* Set data format for mouse device */
  
  if ((res = IDirectInputDevice_SetDataFormat(mouse_drv_lpDID,
                                              &c_dfDIMouse)) != DI_OK) {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): SetDataFormat() ", res );
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  /* Set cooperative level */
  
  if ((res = IDirectInputDevice_SetCooperativeLevel(mouse_drv_lpDID,
						    gfx_drv_hwnd,
						    DISCL_EXCLUSIVE |
						    DISCL_FOREGROUND)) != DI_OK) {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): SetCooperativeLevel() ", res );
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  /* Create event for notification */
  
  if ((mouse_drv_DIevent = CreateEvent(0, 0, 0, 0)) == NULL) {
    fellowAddLog("mouseDrvDInputInitialize(): CreateEvent() failed\n");
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  /* Set property for buffered data */
  
  if ((res = IDirectInputDevice_SetProperty(mouse_drv_lpDID,
				            DIPROP_BUFFERSIZE,
				            &dipdw.diph)) != DI_OK) {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): SetProperty() ", res );
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
  }

  /* Set event notification */

  if ((res = IDirectInputDevice_SetEventNotification(mouse_drv_lpDID,
                                                     mouse_drv_DIevent)) != DI_OK) {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): SetEventNotification() ", res );
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }
  return TRUE;
}


/*===========================================================================*/
/* Mouse grab status has changed                                             */
/*===========================================================================*/

void mouseDrvStateHasChanged(BOOLE active) {
  mouse_drv_active = active;
  if (mouse_drv_active && mouse_drv_focus) {
    mouse_drv_in_use = TRUE;
  }
  else {
    mouse_drv_in_use = FALSE;
  }
  mouseDrvDInputAcquire();
}


/*===========================================================================*/
/* Mouse toggle focus                                                        */
/*===========================================================================*/

void mouseDrvToggleFocus() {
  mouse_drv_focus = !mouse_drv_focus;
  mouseDrvStateHasChanged(mouse_drv_active);
}


/*===========================================================================*/
/* Mouse movement handler                                                    */
/*===========================================================================*/

void mouseDrvMovementHandler() {
  if (mouse_drv_in_use) {
    DIMOUSESTATE dims;
    static BOOLE bLeftButton;
    static BOOLE bRightButton;
    static ULO lx = 0;
    static ULO ly = 0;
    HRESULT res;

    DIDEVICEOBJECTDATA rgod[DINPUT_BUFFERSIZE];
    DWORD itemcount = DINPUT_BUFFERSIZE;

    do {
      res = IDirectInputDevice_GetDeviceData(mouse_drv_lpDID,
	                                     sizeof(DIDEVICEOBJECTDATA),
	                                     rgod,
					     &itemcount,
					     0);
      if (res == DIERR_INPUTLOST) mouseDrvDInputAcquire();
    }
    while (res == DIERR_INPUTLOST);
    
    if (res != DI_OK) {
      mouseDrvDInputFailure("mouseDrvMovementHandler(): GetDeviceData() ", res );
    }
    else {
      ULO i = 0;
      for (i = 0; i < itemcount; i++) {
        if (rgod[i].dwOfs == DIMOFS_BUTTON0) {
	  mouse_drv_left_button = (rgod[i].dwData & 0x80);
	}
        else if (rgod[i].dwOfs == DIMOFS_BUTTON1) {
	  mouse_drv_right_button = (rgod[i].dwData & 0x80);
	}
	else if (rgod[i].dwOfs == DIMOFS_X) {
	  mouse_drv_x = rgod[i].dwData;
	}
	else if (rgod[i].dwOfs == DIMOFS_Y) {
	  mouse_drv_y = rgod[i].dwData;
	}

	}
      }
      
/*      
      bLeftButton   = dims.rgbButtons[0] & 0x80;
      bRightButton  = dims.rgbButtons[1] & 0x80;
      bMiddleButton = dims.rgbButtons[2] & 0x80;
*/    
      if (itemcount > 0)
	gameportMouseHandler(GP_MOUSE0,
			     mouse_drv_x,
                             mouse_drv_y,
                             mouse_drv_left_button,
                             FALSE,
                             mouse_drv_right_button);
  }
}


/*===========================================================================*/
/* Hard Reset                                                                */
/*===========================================================================*/

void mouseDrvHardReset(void) {
}


/*===========================================================================*/
/* Emulation Starting                                                        */
/*===========================================================================*/

BOOLE mouseDrvEmulationStart(void) {
  mouse_drv_left_button = FALSE;
  mouse_drv_right_button = FALSE;
  mouse_drv_x = 0;
  mouse_drv_y = 0;
  return mouseDrvDInputInitialize();
}


/*===========================================================================*/
/* Emulation Stopping                                                        */
/*===========================================================================*/

void mouseDrvEmulationStop(void) {
  mouseDrvDInputRelease();
}


/*===========================================================================*/
/* Emulation Startup                                                         */
/*===========================================================================*/

void mouseDrvStartup(void) {
  mouse_drv_active = FALSE;
  mouse_drv_focus = TRUE;
  mouse_drv_in_use = FALSE;
  mouse_drv_lpDI = NULL;
  mouse_drv_lpDID = NULL;
  mouse_drv_DIevent = NULL;
}


/*===========================================================================*/
/* Emulation Shutdown                                                        */
/*===========================================================================*/

void mouseDrvShutdown(void) {
}

