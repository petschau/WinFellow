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

#include "defs.h"
#include <windows.h>
#include "gameport.h"
#include "fellow.h"
#include "mousedrv.h"
#include "windrv.h"
#include "GfxDrvCommon.h"

#include <initguid.h>
#include "dxver.h"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif

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
static BOOLE		bLeftButton;
static BOOLE		bRightButton;

int num_mouse_attached = 0;


/*==========================================================================*/
/* Returns textual error message. Adapted from DX SDK                       */
/*==========================================================================*/

STR *mouseDrvDInputErrorString(HRESULT hResult)
{
  switch (hResult)
  {
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

void mouseDrvDInputFailure(STR *header, HRESULT err)
{
  fellowAddLog(header);
  fellowAddLog(mouseDrvDInputErrorString(err));
  fellowAddLog("\n");
}


/*===========================================================================*/
/* Acquire DirectInput mouse device                                          */
/*===========================================================================*/

void mouseDrvDInputAcquire(void)
{
  HRESULT res;

  fellowAddLog("mouseDrvDInputAcquire()\n");

  if (mouse_drv_in_use)
  {
    if (mouse_drv_lpDID != NULL)
    {
      if ((res = IDirectInputDevice_Acquire(mouse_drv_lpDID)) != DI_OK)
      {
        mouseDrvDInputFailure("mouseDrvDInputAcquire(): ", res);
      }
    }
  }
  else
  {
    if (mouse_drv_lpDID != NULL)
    {
      if ((res = IDirectInputDevice_Unacquire(mouse_drv_lpDID)) != DI_OK)
      {
        mouseDrvDInputFailure("mouseDrvDInputUnacquire(): ", res);
      }
    }
  }
}


/*===========================================================================*/
/* Release DirectInput for mouse                                             */
/*===========================================================================*/

void mouseDrvDInputRelease(void)
{
  if (mouse_drv_lpDID != NULL)
  {
    IDirectInputDevice_Release(mouse_drv_lpDID);
    mouse_drv_lpDID = NULL;
  }
  if (mouse_drv_DIevent != NULL)
  {
    CloseHandle(mouse_drv_DIevent);
    mouse_drv_DIevent = NULL;
  }
  if (mouse_drv_lpDI != NULL)
  {
    IDirectInput_Release(mouse_drv_lpDI);
    mouse_drv_lpDI = NULL;
  }
}

BOOL FAR PASCAL GetMouseInfo(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef) 
{ 
  fellowAddLog( "**** mouse %d ****\n", num_mouse_attached++ );
  return DIENUM_CONTINUE; 
}
	
/*===========================================================================*/
/* Initialize DirectInput for mouse                                          */
/*===========================================================================*/

BOOLE mouseDrvDInputInitialize(void)
{
  HRESULT res; 

#define INITDIPROP( diprp, obj, how ) \
	{ diprp.diph.dwSize = sizeof( diprp ); \
	diprp.diph.dwHeaderSize = sizeof( diprp.diph ); \
	diprp.diph.dwObj        = obj; \
	diprp.diph.dwHow        = how; }

  DIPROPDWORD dipdw =
  {
    {
      sizeof(DIPROPDWORD),        /* diph.dwSize */
      sizeof(DIPROPHEADER),       /* diph.dwHeaderSize */
      0,                          /* diph.dwObj */
      DIPH_DEVICE,                /* diph.dwHow */
    },
    DINPUT_BUFFERSIZE            /* dwData */
  };

  fellowAddLog("mouseDrvDInputInitialize()\n");

  /* Create Direct Input object */
  
  mouse_drv_lpDI = NULL;
  mouse_drv_lpDID = NULL;
  mouse_drv_DIevent = NULL;
  mouse_drv_initialization_failed = FALSE;

  res = DirectInput8Create(win_drv_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&mouse_drv_lpDI, NULL);
  if (res != DI_OK)
  {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): DirectInput8Create() ", res );
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  num_mouse_attached = 0;
  res = IDirectInput_EnumDevices( mouse_drv_lpDI, DI8DEVTYPE_MOUSE, GetMouseInfo, NULL, DIEDFL_ALLDEVICES);
  if (res != DI_OK)
  {
    fellowAddLog("Mouse Enum Devices failed %s\n", mouseDrvDInputErrorString( res ));
  }
  
  /* Create Direct Input 1 mouse device */
  
  res = IDirectInput_CreateDevice(mouse_drv_lpDI, GUID_SysMouse, &mouse_drv_lpDID, NULL);
  if (res != DI_OK)
  {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): CreateDevice() ", res );
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }
  
  /* Set data format for mouse device */

  res = IDirectInputDevice_SetDataFormat(mouse_drv_lpDID, &c_dfDIMouse);
  if (res != DI_OK)
  {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): SetDataFormat() ", res);
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  /* Set cooperative level */
  res = IDirectInputDevice_SetCooperativeLevel(mouse_drv_lpDID, gfxDrvCommon->GetHWND(), DISCL_EXCLUSIVE | DISCL_FOREGROUND);
  if (res != DI_OK)
  {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): SetCooperativeLevel() ", res );
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  /* Create event for notification */
  mouse_drv_DIevent = CreateEvent(0, 0, 0, 0);
  if (mouse_drv_DIevent == NULL)
  {
    fellowAddLog("mouseDrvDInputInitialize(): CreateEvent() failed\n");
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
    return FALSE;
  }

  /* Set property for buffered data */
  res = IDirectInputDevice_SetProperty(mouse_drv_lpDID, DIPROP_BUFFERSIZE, &dipdw.diph);
  if (res != DI_OK)
  {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): SetProperty() ", res );
    mouse_drv_initialization_failed = TRUE;
    mouseDrvDInputRelease();
  }

  /* Set event notification */
  res = IDirectInputDevice_SetEventNotification(mouse_drv_lpDID, mouse_drv_DIevent);
  if (res != DI_OK)
  {
    mouseDrvDInputFailure("mouseDrvDInputInitialize(): SetEventNotification() ", res );
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

void mouseDrvToggleFocus(void)
{
  mouse_drv_focus = !mouse_drv_focus;
  mouseDrvStateHasChanged(mouse_drv_active);
#ifdef RETRO_PLATFORM
  if(RP.GetHeadlessMode())
  {
    fellowAddLog("mouseDrvToggleFocus(): mouse focus changed to to %s\n", mouse_drv_focus ? "true" : "false");
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
    fellowAddLog("mouseDrvSetFocus(%s)\n", bNewFocus ? "true" : "false");
    
    mouse_drv_focus = bNewFocus;
    mouseDrvStateHasChanged(mouse_drv_active);

  #ifdef RETRO_PLATFORM
    if(RP.GetHeadlessMode())
      if(!bRequestedByRPHost)
      {
        fellowAddLog("mouseDrvSetFocus(%s): notifiying, as not requested by host.\n", 
          bNewFocus ? "true" : "false");
        RP.SendMouseCapture(mouse_drv_focus ? true : false);
      }
  #endif
  }
}

/*===========================================================================*/
/* Mouse movement handler                                                    */
/*===========================================================================*/

void mouseDrvMovementHandler(void)
{
  if (mouse_drv_in_use)
  {
    static LON lx = 0;
    static LON ly = 0;
    HRESULT res;
	
    DIDEVICEOBJECTDATA rgod[DINPUT_BUFFERSIZE];
    DWORD itemcount = DINPUT_BUFFERSIZE;
	
    do
    {
      res = IDirectInputDevice_GetDeviceData(mouse_drv_lpDID, sizeof(DIDEVICEOBJECTDATA), rgod, &itemcount, 0);
      if (res == DIERR_INPUTLOST)
      {
	mouseDrvDInputAcquire();
      }
    }
    while (res == DIERR_INPUTLOST);
	
    if (res != DI_OK)
    {
      mouseDrvDInputFailure("mouseDrvMovementHandler(): GetDeviceData() ", res );
    }
    else
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
	  
      if( itemcount == 0 )	// exit if there are no objects to examine
      {
	return;
      }
	  
      lx = ly = 0;
      for (i = 0; i <= itemcount; i++)
      {
	if( i != 0 )
	{
	  if(( i == itemcount ) ||					// if there are no other objects
		( rgod[i].dwSequence != oldSequence ))	// or the current objects is a different event
	  {
	    gameportMouseHandler(GP_MOUSE0, lx, ly, bLeftButton, FALSE, bRightButton);			
	    lx = ly = 0;
	  }
	  if( i == itemcount )	// no other objects to examine, exit
	  {
		break;
	  }
	}
	oldSequence = rgod[i].dwSequence;
		
	switch( rgod[i].dwOfs )
	{
	  case DIMOFS_BUTTON0:
	    bLeftButton = (rgod[i].dwData & 0x80);
	    break;
		  
	  case DIMOFS_BUTTON1:
	    bRightButton = (rgod[i].dwData & 0x80);
	    break;
		  
	  case DIMOFS_X:
	    lx += rgod[i].dwData;
	    break;
		  
	  case DIMOFS_Y:
	    ly += rgod[i].dwData;
	    break;
	}				
      }
    }
  }
}

BOOLE mouseDrvGetFocus(void)
{
  return mouse_drv_focus;
}

/*===========================================================================*/
/* Hard Reset                                                                */
/*===========================================================================*/

void mouseDrvHardReset(void)
{
  fellowAddLog("mouseDrvHardReset\n");
}


/*===========================================================================*/
/* Emulation Starting                                                        */
/*===========================================================================*/

BOOLE mouseDrvEmulationStart(void) 
{
  fellowAddLog("mouseDrvEmulationStart\n");
  return mouseDrvDInputInitialize();
}


/*===========================================================================*/
/* Emulation Stopping                                                        */
/*===========================================================================*/

void mouseDrvEmulationStop(void) 
{
  fellowAddLog("mouseDrvEmulationStop\n");
  mouseDrvDInputRelease();
}


/*===========================================================================*/
/* Emulation Startup                                                         */
/*===========================================================================*/

void mouseDrvStartup(void) 
{
  fellowAddLog("mouseDrvStartup\n");
  mouse_drv_active = FALSE;
  mouse_drv_focus = TRUE;
  mouse_drv_in_use = FALSE;
  mouse_drv_initialization_failed = TRUE;
  mouse_drv_lpDI = NULL;
  mouse_drv_lpDID = NULL;
  mouse_drv_DIevent = NULL;
  bLeftButton = FALSE;
  bRightButton = FALSE;
}


/*===========================================================================*/
/* Emulation Shutdown                                                        */
/*===========================================================================*/

void mouseDrvShutdown(void) 
{
  fellowAddLog("mouseDrvShutdown\n");
}

