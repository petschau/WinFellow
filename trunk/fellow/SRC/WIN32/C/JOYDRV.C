/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Joystick driver for Windows                                               */
/* Author: Marco Nova (novamarco@hotmail.com)                                */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

/* ---------------- KNOWN BUGS/FIXLIST ----------------- 
- autofire support
- additional button support for other emulator functions
*/

/* ---------------- CHANGE LOG ----------------- 
Monday, September 11, 2000
- fixed joyDrvMovementHandler again

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
*/

#include "defs.h"
#include <windows.h>
#include "gameport.h"
#include "fellow.h"
#include "joydrv.h"
#include "windrv.h"

#ifdef USE_DX5

#include "dxver.h"

#else

#include <mmsystem.h>

#endif


/*===========================================================================*/
/* Joystick device specific data                                             */
/*===========================================================================*/

BOOLE			joy_drv_failed;

#ifdef USE_DX5

IDirectInput2		*joy_drv_lpDI;
IDirectInputDevice2	*joy_drv_lpDID;

#else

int				num_joy_supported;
int				num_joy_attached;

#endif

BOOLE			joy_drv_active;
BOOLE			joy_drv_focus;
BOOLE			joy_drv_in_use;


// min and max values for the axis
// -------------------------------
//
// these min values are the values that the driver will return when
// the lever of the joystick is at the extreme left (x) or the extreme top (y)
// the max values are the same but for the extreme right (x) and the extreme bottom (y)

#define MINX		0
#define MINY		0
#define MAXX		8000
#define MAXY		8000


// med values for both axis
// ------------------------
//
// the med values are used to determine if the stick is in the center or has been moved
// they should be calculated with MEDX = (MAXX-MINX) / 2 and MINY = (MAXY-MINY) / 2
// but they are precalculated for a very very little speed increase (why should we bother
// with runtime division when we already know the results?)

#define MEDX		4000  // precalculated value should be MAXX / 2
#define MEDY		4000  // precalculated value should be MAXY / 2


// dead zone
// ---------
//
// the dead zone is the zone that will not be evaluated as a joy movement
// it is automatically handled by DirectX and it is calculated around the
// center and it based on min and max values previously setuped

#define DEADX		1000  // 10%
#define DEADY		1000  // 10%



/*==========================================================================*/
/* Returns textual error message. Adapted from DX SDK                       */
/*==========================================================================*/

STR *joyDrvDInputErrorString(HRESULT hResult) {
#ifdef _DEBUG
  STR *UnErr = NULL;
#endif

#ifdef USE_DX5

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

#else

    switch (hResult) {
    case JOYERR_NOERROR:		return "The operation completed successfully.";
    case MMSYSERR_NODRIVER:		return "The joystick driver is not present.";
    case MMSYSERR_INVALPARAM:	return "An invalid parameter was passed.";
    case MMSYSERR_BADDEVICEID:	return "The specified joystick identifier is invalid.";
	case JOYERR_PARMS:			return "bad parameters.";
	case JOYERR_NOCANDO:		return "request not completed.";
    case JOYERR_UNPLUGGED:		return "The specified joystick is not connected to the system.";
  }

#endif

#ifdef _DEBUG
  UnErr = (STR*)malloc( 255 * sizeof( STR ));
  sprintf( UnErr, "Not a DirectInput Error [%08x]", hResult );
  return UnErr;
#else
  return "Not a DirectInput Error";
#endif

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

#ifdef USE_DX5

void joyDrvDInputSetCooperativeLevel(void) {
	DIPROPRANGE diprg; 
	DIPROPDWORD dipdw;
	HRESULT res;
	
	if (joy_drv_failed)
		return;

	res = IDirectInputDevice_SetCooperativeLevel(joy_drv_lpDID,
		gfx_drv_hwnd,
		((joy_drv_focus) ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE) | DISCL_FOREGROUND );
	if (res != DI_OK)
		joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): ", res );

#define INITDIPROP( diprp, obj, how ) \
	{ diprp.diph.dwSize = sizeof( diprp ); \
	diprp.diph.dwHeaderSize = sizeof( diprp.diph ); \
	diprp.diph.dwObj        = obj; \
	diprp.diph.dwHow        = how; }
	
	INITDIPROP( diprg, DIJOFS_X, DIPH_BYOFFSET )
	diprg.lMin              = MINX; 
	diprg.lMax              = MAXX;

	res = IDirectInputDevice_SetProperty(joy_drv_lpDID, DIPROP_RANGE, &diprg.diph );
	if( res != DI_OK ) 
		joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): SetProperty RANGE X : ", res );

	INITDIPROP( diprg, DIJOFS_Y, DIPH_BYOFFSET )
	diprg.lMin              = MINY; 
	diprg.lMax              = MAXY;

	res = IDirectInputDevice_SetProperty(joy_drv_lpDID, DIPROP_RANGE, &diprg.diph );
	if( res != DI_OK ) 
		joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): SetProperty RANGE Y : ", res );

	INITDIPROP( dipdw, DIJOFS_X, DIPH_BYOFFSET )
	dipdw.dwData = DEADX;

	res = IDirectInputDevice_SetProperty(joy_drv_lpDID, DIPROP_DEADZONE, &dipdw.diph );
	if( res != DI_OK ) 
		joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): SetProperty DEADZONE X : ", res );

	INITDIPROP( dipdw, DIJOFS_Y, DIPH_BYOFFSET )
	dipdw.dwData = DEADY;

	res = IDirectInputDevice_SetProperty(joy_drv_lpDID, DIPROP_DEADZONE, &dipdw.diph );
	if( res != DI_OK ) 
		joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): SetProperty DEADZONE Y : ", res );

#undef INITDIPROP
}

#endif


/*===========================================================================*/
/* Unacquire DirectInput joystick device                                     */
/*===========================================================================*/

#ifdef USE_DX5

void joyDrvDInputUnacquire(void) {
  if (!joy_drv_failed) {
    HRESULT res;
	
    if ((res = IDirectInputDevice_Unacquire( joy_drv_lpDID )))
      joyDrvDInputFailure("joyDrvDInputUnacquire(): ", res);
  }
}
#endif


/*===========================================================================*/
/* Acquire DirectInput joystick device                                       */
/*===========================================================================*/

#ifdef USE_DX5

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

#endif


/*===========================================================================*/
/* Initialize DirectInput for joystick                                       */
/*===========================================================================*/

void joyDrvDInputInitialize(void) {

#ifdef USE_DX5

  HRESULT res;
  fellowAddLog("joyDrvDInputInitialize()\n");

  if (!joy_drv_lpDI) {
    
	if ((res = CoCreateInstance(&CLSID_DirectInput,
				 NULL,
				 CLSCTX_INPROC_SERVER,
				 &IID_IDirectInput2,
				 &joy_drv_lpDI)) != DI_OK) {
	  joyDrvDInputFailure("joyDrvDInputInitialize(): InputCoCreateInstance() ", res );
	  joy_drv_failed = TRUE;
	  return;
	}

	if ((res = IDirectInput2_Initialize(joy_drv_lpDI,
				 win_drv_hInstance,
				 DIRECTINPUT_VERSION)) != DI_OK) {
	  joyDrvDInputFailure("joyDrvDInputInitialize(): InputInitialize() ", res );
	  joy_drv_failed = TRUE;
	  return;
	}

  }
	
  if (!joy_drv_lpDID) {

	if ((res = CoCreateInstance(&CLSID_DirectInputDevice,
				 NULL,
				 CLSCTX_INPROC_SERVER,
				 &IID_IDirectInputDevice2,
				 &joy_drv_lpDID)) != DI_OK) {
      joyDrvDInputFailure("joyDrvDInputInitialize(): DeviceCoCreateInstance() ", res );
      joy_drv_failed = TRUE;
      return;
    }

	if ((res = IDirectInputDevice2_Initialize(joy_drv_lpDID,
				 win_drv_hInstance,
				 DIRECTINPUT_VERSION,
				 &GUID_Joystick )) != DI_OK) {
      joyDrvDInputFailure("joyDrvDInputInitialize(): DeviceInitialize() ", res );
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

#else
	
	int i;
	JOYINFOEX joyInfoEx;
	MMRESULT res;
	char szMsg[255];

	fellowAddLog("joyDrvDInputInitialize()\n");

	num_joy_attached = 0;
	num_joy_supported = joyGetNumDevs();
	if( !num_joy_supported )
	{
		joyDrvDInputFailure("joyDrvDInputInitialize(): No Device supported ", 0 );
		joy_drv_failed = TRUE;
		return;
	}

	for( i = JOYSTICKID1; i < num_joy_supported; i++ )
	{
	  ZeroMemory( &joyInfoEx, sizeof( JOYINFOEX ));
	  joyInfoEx.dwSize = sizeof( JOYINFOEX );
	  joyInfoEx.dwFlags = JOY_RETURNBUTTONS;

	  res = joyGetPosEx( i, &joyInfoEx );
	  if( res == JOYERR_NOERROR )
		num_joy_attached++;
	  else
		if(( res != JOYERR_UNPLUGGED ) && ( res != JOYERR_PARMS ))
		  joyDrvDInputFailure("joyDrvDInputInitialize(): joyGetPos ", res );
	}

	if( !num_joy_attached )
	{
		joyDrvDInputFailure("joyDrvDInputInitialize(): No Device attached ", 0 );
		joy_drv_failed = TRUE;
		return;
	}

	sprintf( szMsg, "njoy supported: %d, attached %d\n", num_joy_supported, num_joy_attached );
	fellowAddLog( szMsg );

#endif

}


/*===========================================================================*/
/* Release DirectInput for joystick                                          */
/*===========================================================================*/

void joyDrvDInputRelease(void) {
  fellowAddLog("joyDrvDInputRelease()\n");

#ifdef USE_DX5

  if (joy_drv_lpDID) {
	joyDrvDInputUnacquire();
	IDirectInputDevice_Release( joy_drv_lpDID );
	joy_drv_lpDID = NULL;
  }
  
  if (joy_drv_lpDI) {
	IDirectInput_Release( joy_drv_lpDI );
	joy_drv_lpDI = NULL;
  }

#endif
}

 	
/*===========================================================================*/
/* joystick grab status has changed                                          */
/*===========================================================================*/

void joyDrvStateHasChanged(BOOLE active) {
  if( joy_drv_failed )
	return;
  
  joy_drv_active = active;
  if (joy_drv_active && joy_drv_focus) {
	joy_drv_in_use = TRUE;
  }
  else {
	joy_drv_in_use = FALSE;
  }
#ifdef USE_DX5
  joyDrvDInputAcquire();
#endif
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
  int i;
  
  if ( joy_drv_failed || !joy_drv_in_use )
	return;

  if (((gameport_input[0] == GP_ANALOG0) ||
	(gameport_input[1] == GP_ANALOG0))) {

#ifdef USE_DX5
	
	DIJOYSTATE dims;
	HRESULT res;
	int LostCounter = 25;

#else
	
	JOYINFOEX jinfoex;
	MMRESULT  res;

#endif
	
	BOOLE Button1 = FALSE;
	BOOLE Button2 = FALSE;
	BOOLE Left;
	BOOLE Right;
	BOOLE Up;
	BOOLE Down;
	
	Left = Right = Up = Down = FALSE;
	
#ifdef USE_DX5

	do {

	  res = IDirectInputDevice2_Poll(joy_drv_lpDID);
	  if(res != DI_OK)
		joyDrvDInputFailure( "joyDrvMovementHandler(): Poll() ", res );
	  
	  res = IDirectInputDevice_GetDeviceState(joy_drv_lpDID,
		sizeof(DIJOYSTATE),
		&dims);
	  if (res == DIERR_INPUTLOST) joyDrvDInputAcquire();

	  if( LostCounter-- < 0 )
	  {
		joyDrvDInputFailure("joyDrvMovementHandler(): abort -- ", res );
		joy_drv_failed = TRUE;
		return;
	  }

	} while (res == DIERR_INPUTLOST);
	
	if (res != DI_OK) {
	  joyDrvDInputFailure("joyDrvMovementHandler(): GetDeviceState() ", res );
	  return;
	}

	if (dims.rgbButtons[0] & 0x80)
	  Button1 = TRUE;
	
	if (dims.rgbButtons[1] & 0x80)
	  Button2 = TRUE;
	
	if( dims.lX != MEDX )
	{
	  if( dims.lX > MEDX )
		Right = TRUE;
	  else
		Left = TRUE;
	}
	
	if( dims.lY != MEDY )
	{
	  if( dims.lY > MEDY )
		Down = TRUE;
	  else
		Up = TRUE;
	}

#else

	#define MM_DEADMIN		0x6000
	#define MM_DEADMAX		0x9000

	ZeroMemory( &jinfoex, sizeof( JOYINFOEX ));
	jinfoex.dwSize = sizeof( JOYINFOEX );
	jinfoex.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;

	if(( res = joyGetPosEx( JOYSTICKID1, &jinfoex )) != JOYERR_NOERROR )
	{
	  joyDrvDInputFailure("joyDrvMovementHandler(): joyGetPosEx ", res );
	  return;
	}

	if(( jinfoex.dwButtons & JOY_BUTTON1 ) != 0 )
	  Button1 = TRUE;
	
	if(( jinfoex.dwButtons & JOY_BUTTON2 ) != 0 )
	  Button2 = TRUE;

	if( jinfoex.dwXpos > MM_DEADMAX )
	  Right = TRUE;
	else
	  if( jinfoex.dwXpos < MM_DEADMIN )
		Left = TRUE;
	
	if( jinfoex.dwYpos > MM_DEADMAX )
	  Down = TRUE;
	else
	  if( jinfoex.dwYpos < MM_DEADMIN )
		Up = TRUE;

	#undef MM_DEADMIN
	#undef MM_DEADMAX

#endif
	
	for( i = 0; i < 2; i++ )
	  if(( gameport_input[i] == GP_ANALOG0 )
		|| ( gameport_input[i] == GP_ANALOG1 ))
	  {
		if( gameport_left[i] != Left ||
		  gameport_right[i] != Right ||
		  gameport_up[i] != Up ||
		  gameport_down[i] != Down ||
		  gameport_fire0[i] != Button1 ||
		  gameport_fire1[i] != Button2 )
		{
		  gameportJoystickHandler( gameport_input[i]
			, Left
			, Up
			, Right
			, Down
			, Button1
			, Button2
			);
		}
	  }
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
  joy_drv_failed = FALSE;
  joy_drv_focus = TRUE;
  joy_drv_active = FALSE;
  joy_drv_in_use = FALSE;
  joyDrvDInputInitialize();
}


/*===========================================================================*/
/* Emulation Stopping                                                        */
/*===========================================================================*/

void joyDrvEmulationStop(void) {
  joyDrvDInputRelease();
  joy_drv_failed = TRUE;
}


/*===========================================================================*/
/* Emulation Startup                                                         */
/*===========================================================================*/

void joyDrvStartup(void) {
  joy_drv_failed = FALSE;
  joy_drv_focus = TRUE;
  joy_drv_active = FALSE;
  joy_drv_in_use = FALSE;
#ifdef USE_DX5
  joy_drv_lpDI = NULL;
  joy_drv_lpDID = NULL;
  CoInitialize( NULL );
#else
  num_joy_supported = 0;
  num_joy_attached = 0;  
#endif
}


/*===========================================================================*/
/* Emulation Shutdown                                                        */
/*===========================================================================*/

void joyDrvShutdown(void) {
#ifdef USE_DX5
  CoUninitialize();
#endif
}

