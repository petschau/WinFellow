/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Joystick driver for Windows                                               */
/* Author: Marco Nova (novamarco@hotmail.com)                                */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

/* ---------------- KNOWN BUGS/FIXLIST ----------------- 
- additional button support for other emulator functions
- better autofire support trough gameport_autofire arrays
*/

/* ---------------- CHANGE LOG ----------------- 
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
*/

#include "defs.h"
#include <windows.h>
#include "gameport.h"
#include "fellow.h"
#include "joydrv.h"
#include "windrv.h"

#include "dxver.h"
#include <mmsystem.h>



/*===========================================================================*/
/* Joystick device specific data                                             */
/*===========================================================================*/

#define	MAX_JOY_PORT  2

BOOLE			joy_drv_failed;

#ifdef USE_DX5

IDirectInput2		*joy_drv_lpDI;
IDirectInputDevice2	*joy_drv_lpDID[MAX_JOY_PORT];

#else

IDirectInput		*joy_drv_lpDI;
IDirectInputDevice	*joy_drv_lpDID[MAX_JOY_PORT];

#endif

BOOLE				isNT;

int				num_joy_supported;
int				num_joy_attached;

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


BOOLE IsNTOS()
{
  OSVERSIONINFO osInfo;

  ZeroMemory( &osInfo, sizeof( OSVERSIONINFO ));
  osInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

  if( !GetVersionEx( &osInfo ))
  {
	char szMsg[255];

	sprintf( szMsg, "JoyDrv -- GetVersionEx fail -- [%d]\n", GetLastError());
	fellowAddLog( szMsg );
	return FALSE;
  }

#ifdef USE_DX5

  if(( osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) &&
	( osInfo.dwMajorVersion == 4 ) && ( osInfo.dwMinorVersion == 0 ))

#else

  if( osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT )

#endif
  {
	fellowAddLog( "NT detected\n" );
	return TRUE;
  }

#ifdef _DEBUG
  {
	char szMsg[255];
	sprintf( szMsg, "OS %d.%d build %d desc %s, platform %d\n",
	  osInfo.dwMajorVersion,
	  osInfo.dwMinorVersion,
	  osInfo.dwBuildNumber,
	  (osInfo.szCSDVersion ? osInfo.szCSDVersion : "--"),
	  osInfo.dwPlatformId
	);
	fellowAddLog( szMsg );
  }
#endif

  return FALSE;
}

/*==========================================================================*/
/* Returns textual error message. Adapted from DX SDK                       */
/*==========================================================================*/

STR *joyDrvDInputErrorString(HRESULT hResult) {
#ifdef _DEBUG
  STR *UnErr = NULL;
#endif

  if( !isNT )

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

  else

    switch (hResult) {
    case JOYERR_NOERROR:		return "The operation completed successfully.";
    case MMSYSERR_NODRIVER:		return "The joystick driver is not present.";
    case MMSYSERR_INVALPARAM:	return "An invalid parameter was passed.";
    case MMSYSERR_BADDEVICEID:	return "The specified joystick identifier is invalid.";
	case JOYERR_PARMS:			return "bad parameters.";
	case JOYERR_NOCANDO:		return "request not completed.";
    case JOYERR_UNPLUGGED:		return "The specified joystick is not connected to the system.";
  }

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

void joyDrvDInputSetCooperativeLevel(int port) {
	DIPROPRANGE diprg; 
	DIPROPDWORD dipdw;
	HRESULT res;
	
	if (joy_drv_failed)
	  return;

	res = IDirectInputDevice_SetCooperativeLevel(joy_drv_lpDID[port],
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

	res = IDirectInputDevice_SetProperty(joy_drv_lpDID[port], DIPROP_RANGE, &diprg.diph );
	if( res != DI_OK ) 
	  joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): SetProperty RANGE X : ", res );

	INITDIPROP( diprg, DIJOFS_Y, DIPH_BYOFFSET )
	diprg.lMin              = MINY; 
	diprg.lMax              = MAXY;

	res = IDirectInputDevice_SetProperty(joy_drv_lpDID[port], DIPROP_RANGE, &diprg.diph );
	if( res != DI_OK ) 
	  joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): SetProperty RANGE Y : ", res );

	INITDIPROP( dipdw, DIJOFS_X, DIPH_BYOFFSET )
	dipdw.dwData = DEADX;

	res = IDirectInputDevice_SetProperty(joy_drv_lpDID[port], DIPROP_DEADZONE, &dipdw.diph );
	if( res != DI_OK ) 
	  joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): SetProperty DEADZONE X : ", res );

	INITDIPROP( dipdw, DIJOFS_Y, DIPH_BYOFFSET )
	dipdw.dwData = DEADY;

	res = IDirectInputDevice_SetProperty(joy_drv_lpDID[port], DIPROP_DEADZONE, &dipdw.diph );
	if( res != DI_OK ) 
	  joyDrvDInputFailure("joyDrvDInputSetCooperativeLevel(): SetProperty DEADZONE Y : ", res );

#undef INITDIPROP
}


/*===========================================================================*/
/* Unacquire DirectInput joystick device                                     */
/*===========================================================================*/

void joyDrvDInputUnacquire(int port) {
  if (!joy_drv_failed) {
    HRESULT res;
	
    if ((res = IDirectInputDevice_Unacquire( joy_drv_lpDID[port] )))
      joyDrvDInputFailure("joyDrvDInputUnacquire(): ", res);
  }
}


/*===========================================================================*/
/* Acquire DirectInput joystick device                                       */
/*===========================================================================*/

void joyDrvDInputAcquire(int port) {
  if (joy_drv_in_use) {
    HRESULT res;
		
    joyDrvDInputUnacquire( port );
    /* A new window is sometimes created, so set new hwnd cooperative level */
    joyDrvDInputSetCooperativeLevel( port );
    if ((res = IDirectInputDevice_Acquire( joy_drv_lpDID[port] )) != DI_OK)
      joyDrvDInputFailure("joyDrvDInputAcquire(): ", res);
  }
}

#ifdef USE_DX5

BOOLE DxCreateAndInitDevice( IDirectInput2 *pDi, IDirectInputDevice2 *pDiD[], int port ) {
  HRESULT res;

  if (!pDiD[port]) {
	if ((res = CoCreateInstance(&CLSID_DirectInputDevice,
				 NULL,
				 CLSCTX_INPROC_SERVER,
				 &IID_IDirectInputDevice2,
				 &(pDiD[port]))) != DI_OK) {
	  joyDrvDInputFailure("joyDrvDInputInitialize(): DeviceCoCreateInstance() ", res );
	  return TRUE;
	}

	if ((res = IDirectInputDevice2_Initialize(pDiD[port],
				 win_drv_hInstance,
				 DIRECTINPUT_VERSION,
				 &GUID_Joystick )) != DI_OK) {
	  joyDrvDInputFailure("joyDrvDInputInitialize(): DeviceInitialize() ", res );
	  return TRUE;
	}
  }

  res = IDirectInputDevice_SetDataFormat( pDiD[port], (port == 1 ? &c_dfDIJoystick : &c_dfDIJoystick2 ));
  if (res != DI_OK) {
	joyDrvDInputFailure("joyDrvDInputInitialize(): SetDataFormat() ", res );
	return TRUE;
  }

  return FALSE;
} // DxCreateAndInitDevice

#else

BOOLE DxCreateAndInitDevice( IDirectInput *pDi, IDirectInputDevice *pDiD[], int port ) {
  HRESULT res;

  if (!pDiD[port]) {
	res = IDirectInput_CreateDevice(pDi,
				  &GUID_Joystick,
				  &(pDiD[port]),
				  NULL);
	if (res != DI_OK) {
	  joyDrvDInputFailure("joyDrvDInputInitialize(): CreateDevice() ", res );
	  return TRUE;
	}

	if ((res = IDirectInputDevice_Initialize(pDid[port],
				  win_drv_hInstance,
				  DIRECTINPUT_VERSION,
				  &GUID_Joystick )) != DI_OK) {
	  joyDrvDInputFailure("joyDrvDInputInitialize(): DeviceInitialize() ", res );
	  return TRUE;
	}
  }

  res = IDirectInputDevice_SetDataFormat( pDid[port], (port == 1 ? &c_dfDIJoystick : &c_dfDIJoystick2 ));
  if (res != DI_OK) {
	joyDrvDInputFailure("joyDrvDInputInitialize(): SetDataFormat() ", res );
	return TRUE;
  }

  joyDrvDInputAcquire(port);

  return FALSE;
} // DxCreateAndInitDevice

#endif

/*===========================================================================*/
/* Initialize DirectInput for joystick                                       */
/*===========================================================================*/

void joyDrvDInputInitialize(void) {

  if( !isNT )
  {
	HRESULT res;
	fellowAddLog("joyDrvDInputInitialize()\n");

#ifdef USE_DX5

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
	
#else

	if (!joy_drv_lpDI) {
	  
	  if ((res = DirectInputCreate(win_drv_hInstance,
					DIRECTINPUT_VERSION,
					&joy_drv_lpDI,
					NULL )) != DI_OK) {
		joyDrvDInputFailure("joyDrvDInputInitialize(): DirectInputCreate() ", res );
		joy_drv_failed = TRUE;
		return;
	  }

	  if ((res = IDirectInput_Initialize(joy_drv_lpDI,
					win_drv_hInstance,
					DIRECTINPUT_VERSION)) != DI_OK) {
		joyDrvDInputFailure("joyDrvDInputInitialize(): InputInitialize() ", res );
		joy_drv_failed = TRUE;
		return;
	  }

	}

#endif

	if( DxCreateAndInitDevice( joy_drv_lpDI, joy_drv_lpDID, 0 )) {
		fellowAddLog("joyDrvDInputInitialize(): DxCreateAndInitDevice port 0 failed\n" );
		joy_drv_failed = TRUE;
		return;
	}

	if( DxCreateAndInitDevice( joy_drv_lpDI, joy_drv_lpDID, 1 )) {
		fellowAddLog("joyDrvDInputInitialize(): DxCreateAndInitDevice port 1 failed\n" );
		joy_drv_failed = TRUE;
		return;
	}

	return;

  } else {	

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

  }
}


/*===========================================================================*/
/* Release DirectInput for joystick                                          */
/*===========================================================================*/

void joyDrvDInputRelease(void) {
  fellowAddLog("joyDrvDInputRelease()\n");

  if( !isNT )
  {
	int port;

	for( port = 0; port < MAX_JOY_PORT; port++ )
	  if (joy_drv_lpDID[port]) {
		joyDrvDInputUnacquire(port);
		IDirectInputDevice_Release( joy_drv_lpDID[port] );
		joy_drv_lpDID[port] = NULL;
	  }
  
	if (joy_drv_lpDI) {
	  IDirectInput_Release( joy_drv_lpDI );
	  joy_drv_lpDI = NULL;
	}
  }

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

  if( !isNT ) {
	int port;

	for( port = 0; port < MAX_JOY_PORT; port++ )
	  joyDrvDInputAcquire(port);
  }

}


/*===========================================================================*/
/* joystick toggle focus                                                     */
/*===========================================================================*/

void joyDrvToggleFocus() {
  joy_drv_focus = !joy_drv_focus;
  joyDrvStateHasChanged(joy_drv_active);
}


BOOLE CheckJoyMovement( int port, BOOLE *Up, BOOLE *Down, BOOLE *Left, BOOLE *Right,
						BOOLE *Button1, BOOLE *Button2 ) {
  DIJOYSTATE dims;
  HRESULT res;
  int LostCounter = 25;
  JOYINFOEX jinfoex;
  MMRESULT  mmres;

  *Button1 = *Button2 = *Left = *Right = *Up = *Down = FALSE;

  if( !isNT )
  {
	do {

  #ifdef USE_DX5

	  res = IDirectInputDevice2_Poll(joy_drv_lpDID[port]);
	  if(res != DI_OK)
		joyDrvDInputFailure( "joyDrvMovementHandler(): Poll() ", res );

  #endif
	  
	  res = IDirectInputDevice_GetDeviceState(joy_drv_lpDID[port],
		sizeof(DIJOYSTATE),
		&dims);
	  if (res == DIERR_INPUTLOST)
		joyDrvDInputAcquire( port );

	  if( LostCounter-- < 0 )
	  {
		joyDrvDInputFailure("joyDrvMovementHandler(): abort -- ", res );
		joy_drv_failed = TRUE;
		return TRUE;
	  }

	} while (res == DIERR_INPUTLOST);
  
	if (res != DI_OK) {
	  joyDrvDInputFailure("joyDrvMovementHandler(): GetDeviceState() ", res );
	  return TRUE;
	}

	if (dims.rgbButtons[0] & 0x80)
	  *Button1 = TRUE;
  
	if (dims.rgbButtons[1] & 0x80)
	  *Button2 = TRUE;

	if (dims.rgbButtons[2] & 0x80)
	{
	  if( gameport_fire0[port] )
		*Button1 = FALSE;
	  else
		*Button1 = TRUE;
	}
  
	if (dims.rgbButtons[3] & 0x80)
	{
	  if( gameport_fire1[port] )
		*Button2 = FALSE;
	  else
		*Button2 = TRUE;
	}

	if( dims.lX != MEDX )
	{
	  if( dims.lX > MEDX )
		*Right = TRUE;
	  else
		*Left = TRUE;
	}
  
	if( dims.lY != MEDY )
	{
	  if( dims.lY > MEDY )
		*Down = TRUE;
	  else
		*Up = TRUE;
	}
  } else {

	#define MM_DEADMIN		0x6000
	#define MM_DEADMAX		0x9000

	ZeroMemory( &jinfoex, sizeof( JOYINFOEX ));
	jinfoex.dwSize = sizeof( JOYINFOEX );
	jinfoex.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;

	if(( mmres = joyGetPosEx( (port == 1 ? JOYSTICKID1 : JOYSTICKID1), &jinfoex )) != JOYERR_NOERROR )
	{
	  joyDrvDInputFailure("joyDrvMovementHandler(): joyGetPosEx ", mmres );
	  return TRUE;
	}

	if(( jinfoex.dwButtons & JOY_BUTTON1 ) != 0 )
	  *Button1 = TRUE;
	
	if(( jinfoex.dwButtons & JOY_BUTTON2 ) != 0 )
	  *Button2 = TRUE;

	if(( jinfoex.dwButtons & JOY_BUTTON3 ) != 0 )
	{
	  if( gameport_fire0[port] )
		*Button1 = FALSE;
	  else
		*Button1 = TRUE;
	}
	
	if(( jinfoex.dwButtons & JOY_BUTTON4 ) != 0 )
	{
	  if( gameport_fire1[port] )
		*Button2 = FALSE;
	  else
		*Button2 = TRUE;
	}

	if( jinfoex.dwXpos > MM_DEADMAX )
	  *Right = TRUE;
	else
	  if( jinfoex.dwXpos < MM_DEADMIN )
		*Left = TRUE;
	
	if( jinfoex.dwYpos > MM_DEADMAX )
	  *Down = TRUE;
	else
	  if( jinfoex.dwYpos < MM_DEADMIN )
		*Up = TRUE;

	#undef MM_DEADMIN
	#undef MM_DEADMAX

  }

  return FALSE;
} // CheckJoyMovement

/*===========================================================================*/
/* joystick movement handler                                                 */
/*===========================================================================*/

void joyDrvMovementHandler() {
  int port;
  
  if ( joy_drv_failed || !joy_drv_in_use )
	return;

  for( port = 0; port < 2; port++ )
  {
	if (( gameport_input[port] == GP_ANALOG0) ||
	    ( gameport_input[port] == GP_ANALOG1)) {
	
	  BOOLE Button1;
	  BOOLE Button2;
	  BOOLE Left;
	  BOOLE Right;
	  BOOLE Up;
	  BOOLE Down;
	  
	  Button1 = Button2 = Left = Right = Up = Down = FALSE;
	  
	  if( CheckJoyMovement( port, &Up, &Down, &Left, &Right, &Button1, &Button2 )) {
		fellowAddLog( "DxCheckJoyMovement failed\n" );
		return;
	  }

	  if(( gameport_input[port] == GP_ANALOG0 )
		|| ( gameport_input[port] == GP_ANALOG1 ))
	  {
		if( gameport_left[port] != Left ||
		  gameport_right[port] != Right ||
		  gameport_up[port] != Up ||
		  gameport_down[port] != Down ||
		  gameport_fire0[port] != Button1 ||
		  gameport_fire1[port] != Button2 )
		{
		  gameportJoystickHandler( gameport_input[port]
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

  isNT = IsNTOS();

  joy_drv_lpDI = NULL;
  joy_drv_lpDID[0] = NULL;
  joy_drv_lpDID[1] = NULL;

#ifdef USE_DX5

  if( !isNT )
	CoInitialize( NULL );

#endif

  num_joy_supported = 0;
  num_joy_attached = 0;  
}


/*===========================================================================*/
/* Emulation Shutdown                                                        */
/*===========================================================================*/

void joyDrvShutdown(void) {

#ifdef USE_DX5

  if( !isNT )
	CoUninitialize();

#endif

}

