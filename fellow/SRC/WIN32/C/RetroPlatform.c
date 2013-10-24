/* $Id$ */
/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Cloanto RetroPlatform GUI integration                                   */
/*                                                                         */
/* Author: Torsten Enderling (carfesh@gmx.net)                             */
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

#ifdef RETRO_PLATFORM

/** @file
 *  Cloanto RetroPlatform GUI integration.
 *
 * This module contains RetroPlatform specific functionality to register as
 * RetroPlatform guest and interact with the host (player).
 * It imitates the full Windows GUI module, implementing the same functionality, 
 * but supported via the RetroPlatform player as main GUI.
 * WinFellow's own GUI is not shown, the emulator operates in a headless mode.
 * The configuration is received as a command line parameter, all control events 
 * (start, shutdown, reset, ...) are sent via IPC.
 * 
 * *Important Note:* The Cloanto modules make heavy use of unicode strings.
 * As WinFellow uses ANSI strings, conversion is usually required (using, for example,
 * wsctombs and mbstowcs).
 * 
 * When looking at an RP9 package, the RetroPlatform WinFellow plug-in has a list of
 * criteria to decide if WinFellow is a compatible emulator that is offered as choice.
 * It verifies that
 * - a valid model-specific INI file exists for the configured Amiga model
 * - no extended ADF files are used (file size = 901.120 for all ADFs)
 * 
 * The plug-in will block the start of WinFellow on a number of criteria:
 * - if filesystems are used
 * - hardfiles using a non-standard geometry, or RDB hardfiles
 *
 * The RetroPlatform WinFellow plug-in will start WinFellow with the following 
 * command-line arguments:
 * 
 * -rphost:
 * ID of the RetroPlatform host, used to initiate IPC communication.
 *
 * -datapath:
 * the path where WinFellow specific runtime information should be stored.
 * 
 * -f:
 * The -f parameter provides an initial configuration, that is created in the following order:
 * -# the WinFellow plug-in's shared.ini is applied
 * -# a model-specific INI is applied on top of that
 * -# the WinFellow plug-in overrides a number of things:
 *    - if "No interlace" is checked in the compatibility settings, fellow.gfx_deinterlace is set to no, otherwise to yes
 *    - if "Turbo CPU" is checked, cpu_speed is set to 0
 *    - if the user has enabled "Always speed up drives where possible", "Turbo floppy" is set to yes in the RP9,
 *       and  "Always use more accurate (slower) emulation" in the Option dialog is NOT set, fellow.floppy_fast_dma is set to yes,
 *       otherwise to no
 *    - gfx_width, gfx_height, gfx_offset_left and gfx_offset_top are added into the configuration depending on the
 *      settings of the RP9; the numbers assume the maximum pixel density (horizontal values super hires, vertical
 *      values interlace), so depending on the mode displayed, conversion is required; the clipping offsets need to
*       be adjusted (384 to the top, 52 to the left)
 * -# the WinFellow plug-in's override.ini is applied on top of that, to apply any settings that always must be active
 *
 * -rpescapekey:
 * the DirectInput keycode of the escape key
 *
 * -rpescapeholdtime:
 * the time in milliseconds after which the escape key should actually escape
 *
 * -rpscreenmode: 
 * the initial screenmode the guest should activate at startup (e.g. 1X, 2X, ...).
 * It is the numerical equivalent of the RP_SCREENMODE_* flags (see RetroPlatformIPC.h).
 */

#include "defs.h"

#include "RetroPlatform.h"
#include "RetroPlatformGuestIPC.h"
#include "RetroPlatformIPC.h"

#include "config.h"
#include "fellow.h"
#include "windrv.h"
#include "floppy.h"
#include "gfxdrv.h"
#include "mousedrv.h"
#include "joydrv.h"
#include "CpuIntegration.h"
#include "BUS.H"
#include "kbddrv.h"
#include "FHFILE.H"
#include "dxver.h"    /// needed for DirectInput based joystick detection code
#include "sounddrv.h" /// needed for DirectSound volume control

#define RETRO_PLATFORM_NUM_GAMEPORTS 2 ///< gameport 1 & 2
#define RETRO_PLATFORM_KEYSET_COUNT  6 ///< north, east, south, west, fire, autofire

#define RETRO_PLATFORM_HARDDRIVE_BLINK_MSECS 100

static const char *RetroPlatformCustomLayoutKeys[RETRO_PLATFORM_KEYSET_COUNT] = { "up", "right", "down", "left", "fire", "fire.autorepeat" };
extern BOOLE kbd_drv_joykey_enabled[2][2];	///< For each port, the enabled joykeys

#define	MAX_JOY_PORT  2 ///< maximum number of physically attached joysticks; the value originates from joydrv.c, as we emulate the enumeration behaviour
int RetroPlatformNumberOfJoysticksAttached;

/// host ID that was passed over by the RetroPlatform player
STR szRetroPlatformHostID[CFG_FILENAME_LENGTH] = "";
/// flag to indicate that emulator operates in "headless" mode
bool bRetroPlatformMode = false;

ULO lRetroPlatformEscapeKey                          = 1;
ULO lRetroPlatformEscapeKeyHoldTime                  = 600;
ULONGLONG lRetroPlatformEscapeKeyHeldSince           = 0;
ULONGLONG lRetroPlatformEscapeKeySimulatedTargetTime = 0;
ULO lRetroPlatformScreenMode                         = 0;

static BOOLE bRetroPlatformInitialized    = FALSE;
static BOOLE bRetroPlatformEmulationState = FALSE;
static bool  bRetroPlatformEmulationPaused = FALSE;
static BOOLE bRetroPlatformEmulatorQuit   = FALSE;
static BOOLE bRetroPlatformMouseCaptureRequestedByHost = FALSE;

static ULO lRetroPlatformMainVersion = -1, lRetroPlatformRevision = -1, lRetroPlatformBuild = -1;
static LON lRetroPlatformClippingOffsetLeftRP = 0, lRetroPlatformClippingOffsetTopRP = 0;
static LON lRetroPlatformScreenWidthRP = 0, lRetroPlatformScreenHeightRP = 0;
static bool bRetroPlatformScreenWindowed = true;
static bool bRetroPlatformClippingAutomatic = true;
static DISPLAYSCALE RetroPlatformDisplayScale = DISPLAYSCALE_1X;

static RPGUESTINFO RetroPlatformGuestInfo = { 0 };

HINSTANCE hRetroPlatformWindowInstance = NULL;
HWND      hRetroPlatformGuestWindow = NULL;

cfg *RetroPlatformConfig; ///< RetroPlatform copy of configuration


/** Joystick enumeration function.
 */
BOOL FAR PASCAL RetroPlatformEnumerateJoystick(LPCDIDEVICEINSTANCE pdinst, 
  LPVOID pvRef) {
  STR strHostInputID[CFG_FILENAME_LENGTH];
  WCHAR szHostInputID[CFG_FILENAME_LENGTH];
  WCHAR szHostInputName[CFG_FILENAME_LENGTH];
                                    
  fellowAddLog( "**** Joystick %d **** '%s'\n",
    ++RetroPlatformNumberOfJoysticksAttached, pdinst->tszProductName);

  sprintf(strHostInputID, "GP_ANALOG%d", RetroPlatformNumberOfJoysticksAttached - 1);
  mbstowcs(szHostInputID, strHostInputID, CFG_FILENAME_LENGTH);
  mbstowcs(szHostInputName, pdinst->tszProductName, CFG_FILENAME_LENGTH);

  RetroPlatformSendInputDevice(RP_HOSTINPUT_JOYSTICK, 
    RP_FEATURE_INPUTDEVICE_JOYSTICK | RP_FEATURE_INPUTDEVICE_GAMEPAD,
    0, szHostInputID, szHostInputName);

  if(RetroPlatformNumberOfJoysticksAttached == 2)
    return DIENUM_STOP;
  else  
    return DIENUM_CONTINUE; 
}

/** Determine the number of joysticks connected to the system.
 */
int RetroPlatformEnumerateJoysticks(void) {
  int njoyCount = 0;

  HRESULT hResult;
  IDirectInput8 *RP_lpDI = NULL;

  fellowAddLog("RetroPlatformEnumerateJoysticks()\n");

  if (!RP_lpDI) {
    hResult = CoCreateInstance(CLSID_DirectInput8,
			   NULL,
			   CLSCTX_INPROC_SERVER,
			   IID_IDirectInput8,
			   (LPVOID*) &RP_lpDI);
    if (hResult != DI_OK) {
      fellowAddLog("RetroPlatformEnumerateJoysticks(): CoCreateInstance() failed, errorcode %d\n", 
        hResult);
     return 0;
    }

    hResult = IDirectInput8_Initialize(RP_lpDI,
				   win_drv_hInstance,
				   DIRECTINPUT_VERSION);
    if (hResult != DI_OK) {
      fellowAddLog("RetroPlatformEnumerateJoysticks(): Initialize() failed, errorcode %d\n", 
        hResult);
      return 0;
    }

    RetroPlatformNumberOfJoysticksAttached = 0;

    hResult = IDirectInput8_EnumDevices(RP_lpDI, DI8DEVCLASS_GAMECTRL,
				    RetroPlatformEnumerateJoystick, RP_lpDI, DIEDFL_ATTACHEDONLY);
    if (hResult != DI_OK) {
      fellowAddLog("RetroPlatformEnumerateJoysticks(): EnumDevices() failed, errorcode %d\n", 
        hResult);
      return 0;
    }

    njoyCount = RetroPlatformNumberOfJoysticksAttached;

    if (RP_lpDI != NULL) {
	    IDirectInput8_Release(RP_lpDI);
      RP_lpDI = NULL;
    }
  }

  fellowAddLog("RetroPlatformEnumerateJoysticks: detected %d joystick(s).\n", 
    njoyCount);

  return njoyCount;
}

/** Set clipping offset that is applied to the left of the picture.
 */
void RetroPlatformSetClippingOffsetLeft(const ULO lOffsetLeft) {
  bRetroPlatformClippingAutomatic = false;

  lRetroPlatformClippingOffsetLeftRP = lOffsetLeft;

#ifdef _DEBUG
  fellowAddLog("RetroPlatformSetClippingOffsetLeft(%u)\n", lOffsetLeft);
#endif
}

/** Set clipping offset that is applied to the top of the picture
 */
void RetroPlatformSetClippingOffsetTop(const ULO lOffsetTop) {
  bRetroPlatformClippingAutomatic = false;

  lRetroPlatformClippingOffsetTopRP = lOffsetTop;

#ifdef _DEBUG
  fellowAddLog("RetroPlatformSetClippingOffsetTop(%u)\n", lOffsetTop);
#endif
}

/** configure keyboard layout to custom key mappings
 *
 * Gameport 0 is statically mapped to internal keyboard layout GP_JOYKEY0, 
 * gameport 1 to GP_JOYKEY1 as we reconfigure them anyway
 */
void RetroPlatformSetCustomKeyboardLayout(const ULO lGameport, const STR *pszKeys) {
  int l[RETRO_PLATFORM_KEYSET_COUNT], n;
  STR *psz;
  size_t ln;

  fellowAddLog(" Configuring keyboard layout %d to %s.\n", lGameport, pszKeys);

  // keys not (always) configured via custom layouts
  kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_FIRE1_ACTIVE     : EVENT_JOY0_FIRE1_ACTIVE,     0);
  kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_AUTOFIRE0_ACTIVE : EVENT_JOY0_AUTOFIRE0_ACTIVE, 0);
  kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_AUTOFIRE1_ACTIVE : EVENT_JOY0_AUTOFIRE1_ACTIVE, 0);
  
  while(*pszKeys) {
	  for (; *pszKeys == ' '; pszKeys++); // skip spaces

	  for (n = 0; n < RETRO_PLATFORM_KEYSET_COUNT; n++) {
		  ln = strlen(RetroPlatformCustomLayoutKeys[n]);
		  if (strnicmp(pszKeys, RetroPlatformCustomLayoutKeys[n], ln) == 0 && *(pszKeys + ln) == '=')
			  break;
	  }
	  if (n < RETRO_PLATFORM_KEYSET_COUNT) {
		  pszKeys += ln + 1;

		  l[n] = kbddrv_DIK_to_symbol[strtoul(pszKeys, &psz, 0)]; // convert DIK_* DirectInput key codes to symbolic keycodes
		 
      // perform the individual mappings
      if(strnicmp(RetroPlatformCustomLayoutKeys[n], "up", strlen("up")) == 0)
        kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_UP_ACTIVE : EVENT_JOY0_UP_ACTIVE, l[n]);
      else if(strnicmp(RetroPlatformCustomLayoutKeys[n], "down", strlen("down")) == 0)
        kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_DOWN_ACTIVE : EVENT_JOY0_DOWN_ACTIVE, l[n]);
      else if(strnicmp(RetroPlatformCustomLayoutKeys[n], "left", strlen("left")) == 0)
        kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_LEFT_ACTIVE : EVENT_JOY0_LEFT_ACTIVE, l[n]);
      else if(strnicmp(RetroPlatformCustomLayoutKeys[n], "right", strlen("right")) == 0)
        kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_RIGHT_ACTIVE : EVENT_JOY0_RIGHT_ACTIVE, l[n]);
      else if(strnicmp(RetroPlatformCustomLayoutKeys[n], "fire", strlen("fire")) == 0)
        kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_FIRE0_ACTIVE : EVENT_JOY0_FIRE0_ACTIVE, l[n]);
      else if(strnicmp(RetroPlatformCustomLayoutKeys[n], "fire.autorepeat", strlen("fire.autorepeat")) == 0)
        kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_AUTOFIRE0_ACTIVE : EVENT_JOY0_AUTOFIRE0_ACTIVE, l[n]);
	  }
	  for (; *pszKeys != ' ' && *pszKeys != 0; pszKeys++); // reach next key definition
	}

  for(n = 0; n < RETRO_PLATFORM_KEYSET_COUNT; n++)
    fellowAddLog(" Direction %s mapped to key %s.\n", 
      RetroPlatformCustomLayoutKeys[n], kbdDrvKeyString(l[n]));
}

/** Attach input devices to gameports during runtime of the emulator.
 * 
 * The device is selected in the RetroPlatform player and passed to the emulator
 * in form of an IPC message.
 */
static BOOLE RetroPlatformConnectInputDeviceToPort(const ULO lGameport, 
  const ULO lDeviceType, DWORD dwFlags, const STR *szName) {

	if (lGameport < 0 || lGameport >= RETRO_PLATFORM_NUM_GAMEPORTS)
		return FALSE;

  fellowAddLog("RetroPlatformConnectInputDeviceToPort(): port %d, device type %d, flags %d, name '%s'\n", 
    lGameport, lDeviceType, dwFlags, szName);

  switch(lDeviceType) {
    case RP_INPUTDEVICE_EMPTY:
      fellowAddLog(" Removing input device from gameport..\n");
      gameportSetInput(lGameport, GP_NONE);
      kbd_drv_joykey_enabled[lGameport][lGameport] = FALSE;
      return TRUE;
    case RP_INPUTDEVICE_MOUSE:
      fellowAddLog(" Attaching mouse device to gameport..\n");
      gameportSetInput(lGameport, GP_MOUSE0);
      return TRUE;
    case RP_INPUTDEVICE_JOYSTICK:
      if(strcmp(szName, "GP_ANALOG0") == 0) {
        fellowAddLog(" Attaching joystick 1 to gameport..\n");
        gameportSetInput(lGameport, GP_ANALOG0);
      }
      else if(strcmp(szName, "GP_ANALOG1") == 0) {
        fellowAddLog(" Attaching joystick 2 to gameport..\n");
        gameportSetInput(lGameport, GP_ANALOG1);
      }
      else if(_strnicmp(szName, "GP_JOYKEYCUSTOM", strlen("GP_JOYKEYCUSTOM")) == 0) { // custom layout
        RetroPlatformSetCustomKeyboardLayout(lGameport, szName + strlen("GP_JOYKEYCUSTOM") + 1);
        gameportSetInput(lGameport, (lGameport == 1) ? GP_JOYKEY1 : GP_JOYKEY0);
        if(lGameport == 0) {
          kbd_drv_joykey_enabled[lGameport][0] = TRUE;
          kbd_drv_joykey_enabled[lGameport][1] = FALSE;
        }
        else if(lGameport == 1) {
          kbd_drv_joykey_enabled[lGameport][0] = FALSE;
          kbd_drv_joykey_enabled[lGameport][1] = TRUE;
        }
      }
      else {
        fellowAddLog (" WARNING: Unknown joystick input device name, ignoring..\n");
        return FALSE;
      }
      return TRUE;
    default:
      fellowAddLog(" WARNING: Unsupported input device type detected.\n");
      return FALSE;
  }
}

/** Translate a RetroPlatform IPC message code into readable text.
 */
static const STR *RetroPlatformGetMessageText(ULO iMsg) {
  switch(iMsg) {
    case RP_IPC_TO_HOST_REGISTER:           return TEXT("RP_IPC_TO_HOST_REGISTER");
    case RP_IPC_TO_HOST_FEATURES:           return TEXT("RP_IPC_TO_HOST_FEATURES");
    case RP_IPC_TO_HOST_CLOSED:             return TEXT("RP_IPC_TO_HOST_CLOSED");
    case RP_IPC_TO_HOST_ACTIVATED:          return TEXT("RP_IPC_TO_HOST_ACTIVATED");
    case RP_IPC_TO_HOST_DEACTIVATED:        return TEXT("RP_IPC_TO_HOST_DEACTIVATED");
    case RP_IPC_TO_HOST_SCREENMODE:         return TEXT("RP_IPC_TO_HOST_SCREENMODE");
    case RP_IPC_TO_HOST_POWERLED:           return TEXT("RP_IPC_TO_HOST_POWERLED");
    case RP_IPC_TO_HOST_DEVICES:            return TEXT("RP_IPC_TO_HOST_DEVICES");
    case RP_IPC_TO_HOST_DEVICEACTIVITY:     return TEXT("RP_IPC_TO_HOST_DEVICEACTIVITY");
    case RP_IPC_TO_HOST_MOUSECAPTURE:       return TEXT("RP_IPC_TO_HOST_MOUSECAPTURE");
    case RP_IPC_TO_HOST_HOSTAPIVERSION:     return TEXT("RP_IPC_TO_HOST_HOSTAPIVERSION");
    case RP_IPC_TO_HOST_PAUSE:              return TEXT("RP_IPC_TO_HOST_PAUSE");
    case RP_IPC_TO_HOST_DEVICECONTENT:      return TEXT("RP_IPC_TO_HOST_DEVICECONTENT");
    case RP_IPC_TO_HOST_TURBO:              return TEXT("RP_IPC_TO_HOST_TURBO");
    case RP_IPC_TO_HOST_PING:               return TEXT("RP_IPC_TO_HOST_PING");
    case RP_IPC_TO_HOST_VOLUME:             return TEXT("RP_IPC_TO_HOST_VOLUME");
    case RP_IPC_TO_HOST_ESCAPED:            return TEXT("RP_IPC_TO_HOST_ESCAPED");
    case RP_IPC_TO_HOST_PARENT:             return TEXT("RP_IPC_TO_HOST_PARENT");
    case RP_IPC_TO_HOST_DEVICESEEK:         return TEXT("RP_IPC_TO_HOST_DEVICESEEK");
    case RP_IPC_TO_HOST_CLOSE:              return TEXT("RP_IPC_TO_HOST_CLOSE");
    case RP_IPC_TO_HOST_DEVICEREADWRITE:    return TEXT("RP_IPC_TO_HOST_DEVICEREADWRITE");
    case RP_IPC_TO_HOST_HOSTVERSION:        return TEXT("RP_IPC_TO_HOST_HOSTVERSION");
    case RP_IPC_TO_HOST_INPUTDEVICE:        return TEXT("RP_IPC_TO_HOST_INPUTDEVICE");
    case RP_IPC_TO_GUEST_CLOSE:             return TEXT("RP_IPC_TO_GUEST_CLOSE");
    case RP_IPC_TO_GUEST_SCREENMODE:        return TEXT("RP_IPC_TO_GUEST_SCREENMODE");
    case RP_IPC_TO_GUEST_SCREENCAPTURE:     return TEXT("RP_IPC_TO_GUEST_SCREENCAPTURE");
    case RP_IPC_TO_GUEST_PAUSE:             return TEXT("RP_IPC_TO_GUEST_PAUSE");
    case RP_IPC_TO_GUEST_DEVICECONTENT:     return TEXT("RP_IPC_TO_GUEST_DEVICECONTENT");
    case RP_IPC_TO_GUEST_RESET:             return TEXT("RP_IPC_TO_GUEST_RESET");
    case RP_IPC_TO_GUEST_TURBO:             return TEXT("RP_IPC_TO_GUEST_TURBO");
    case RP_IPC_TO_GUEST_PING:              return TEXT("RP_IPC_TO_GUEST_PING");
    case RP_IPC_TO_GUEST_VOLUME:            return TEXT("RP_IPC_TO_GUEST_VOLUME");
    case RP_IPC_TO_GUEST_ESCAPEKEY:         return TEXT("RP_IPC_TO_GUEST_ESCAPEKEY");
    case RP_IPC_TO_GUEST_EVENT:             return TEXT("RP_IPC_TO_GUEST_EVENT");
    case RP_IPC_TO_GUEST_MOUSECAPTURE:      return TEXT("RP_IPC_TO_GUEST_MOUSECAPTURE");
    case RP_IPC_TO_GUEST_SAVESTATE:         return TEXT("RP_IPC_TO_GUEST_SAVESTATE");
    case RP_IPC_TO_GUEST_LOADSTATE:         return TEXT("RP_IPC_TO_GUEST_LOADSTATE");
    case RP_IPC_TO_GUEST_FLUSH:             return TEXT("RP_IPC_TO_GUEST_FLUSH");
    case RP_IPC_TO_GUEST_DEVICEREADWRITE:   return TEXT("RP_IPC_TO_GUEST_DEVICEREADWRITE");
    case RP_IPC_TO_GUEST_QUERYSCREENMODE:   return TEXT("RP_IPC_TO_GUEST_QUERYSCREENMODE");
    case RP_IPC_TO_GUEST_GUESTAPIVERSION :  return TEXT("RP_IPC_TO_GUEST_GUESTAPIVERSION");
    default: return TEXT("UNKNOWN");
  }
}

/** Determine a timestamp for the current time.
 */
ULONGLONG RetroPlatformGetTime(void) {
  SYSTEMTIME st;
  FILETIME ft;
  ULARGE_INTEGER li;

  GetSystemTime(&st);
  if (!SystemTimeToFileTime (&st, &ft))
    return 0;
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;
  return li.QuadPart / 10000;
}

/** Send an IPC message to RetroPlatform host.
 * @return TRUE is sucessfully sent, FALSE otherwise.
 */
static BOOLE RetroPlatformSendMessage(ULO iMessage, WPARAM wParam, LPARAM lParam,
	LPCVOID pData, DWORD dwDataSize, const RPGUESTINFO *pGuestInfo, LRESULT *plResult) {
	BOOLE bResult;

	bResult = RPSendMessage(iMessage, wParam, lParam, pData, dwDataSize, pGuestInfo, plResult);
  
#ifdef _DEBUG
  if(bResult)
    fellowAddLog("RetroPlatform sent message ([%s], %08x, %08x, %08x, %d)\n",
      RetroPlatformGetMessageText(iMessage), iMessage - WM_APP, wParam, lParam, pData);
  else
		fellowAddLog("RetroPlatform could not send message, error: %d\n", GetLastError());
#endif

  return bResult;
}

ULO RetroPlatformGetClippingOffsetLeftAdjusted(void) {
  ULO lRetroPlatformClippingOffsetLeft = lRetroPlatformClippingOffsetLeftRP;

  switch(RetroPlatformGetDisplayScale())
  {
    case DISPLAYSCALE_1X:
      if(lRetroPlatformClippingOffsetLeft >= RETRO_PLATFORM_OFFSET_ADJUST_LEFT)
        lRetroPlatformClippingOffsetLeft = (lRetroPlatformClippingOffsetLeft - RETRO_PLATFORM_OFFSET_ADJUST_LEFT) / 2;
      else if(lRetroPlatformClippingOffsetLeft >= RETRO_PLATFORM_OFFSET_ADJUST_LEFT_FALLBACK)
        lRetroPlatformClippingOffsetLeft = (lRetroPlatformClippingOffsetLeft - RETRO_PLATFORM_OFFSET_ADJUST_LEFT_FALLBACK) / 2;
      break;
    case DISPLAYSCALE_2X:
      if(lRetroPlatformClippingOffsetLeft >= RETRO_PLATFORM_OFFSET_ADJUST_LEFT)
        lRetroPlatformClippingOffsetLeft = lRetroPlatformClippingOffsetLeft - RETRO_PLATFORM_OFFSET_ADJUST_LEFT;
      else if(lRetroPlatformClippingOffsetLeft >= RETRO_PLATFORM_OFFSET_ADJUST_LEFT_FALLBACK)
        lRetroPlatformClippingOffsetLeft = lRetroPlatformClippingOffsetLeft - RETRO_PLATFORM_OFFSET_ADJUST_LEFT_FALLBACK;
      break;
    default:
      fellowAddLog("RetroPlatformGetClippingOffsetLeftAdjusted(): WARNING: unknown display scaling factor 0x%x\n.",
        RetroPlatformGetDisplayScale());
      break;
  }

#ifdef _DEBUGVERBOSE
  fellowAddLog("RetroPlatformGetClippingOffsetLeftAdjusted(): left offset adjusted from %u to %u\n", 
    lRetroPlatformClippingOffsetLeftRP, lRetroPlatformClippingOffsetLeft);
#endif

  return lRetroPlatformClippingOffsetLeft;
}

ULO RetroPlatformGetClippingOffsetTopAdjusted(void) {
  ULO lRetroPlatformClippingOffsetTop = lRetroPlatformClippingOffsetTopRP;

  if(lRetroPlatformClippingOffsetTop >= RETRO_PLATFORM_OFFSET_ADJUST_TOP)
    lRetroPlatformClippingOffsetTop -= RETRO_PLATFORM_OFFSET_ADJUST_TOP;

  switch(RetroPlatformGetDisplayScale())
  {
    case DISPLAYSCALE_1X:
      break;
    case DISPLAYSCALE_2X:
      lRetroPlatformClippingOffsetTop *= 2;
      break;
    default:
      fellowAddLog("RetroPlatformGetClippingOffsetTopAdjusted(): WARNING: unknown display scaling factor 0x%x.\n",
        RetroPlatformGetDisplayScale());
      break;
  }

#ifdef _DEBUGVERBOSE
  fellowAddLog("RetroPlatformGetClippingOffsetTopAdjusted(): top offset adjusted from %u to %u\n", 
    lRetroPlatformClippingOffsetTopRP, lRetroPlatformClippingOffsetTop);
#endif
  
  return lRetroPlatformClippingOffsetTop;
}

ULO RetroPlatformGetClippingOffsetLeft(void) {
  return lRetroPlatformClippingOffsetLeftRP;
}

ULO RetroPlatformGetClippingOffsetTop(void) {
  return lRetroPlatformClippingOffsetTopRP;
}

bool RetroPlatformGetClippingAutomatic(void) {
  return bRetroPlatformClippingAutomatic;
}

void RetroPlatformSetClippingAutomatic(const bool bAutomatic) {
  if(bAutomatic != bRetroPlatformClippingAutomatic) {
    bRetroPlatformClippingAutomatic = bAutomatic;

    fellowAddLog("RetroPlatformSetClippingAutomatic(): automatic clipping is now %s.\n", bAutomatic ? "enabled" : "disabled");
  }
}

ULO RetroPlatformGetScreenHeightAdjusted(void) {
  ULO lScreenHeight = lRetroPlatformScreenHeightRP;

  if(RetroPlatformGetDisplayScale() == DISPLAYSCALE_2X)
    lScreenHeight *= 2;

  return lScreenHeight;
}

ULO RetroPlatformGetScreenWidthAdjusted(void) {
  ULO lScreenWidth = 0;
  
  if(lRetroPlatformScreenWidthRP > 768) {
    // target width is for super-hires mode display; for now, just divide by two to have the hires resolution
    
    switch(RetroPlatformGetDisplayScale())
    {
      case DISPLAYSCALE_1X:
        lScreenWidth  = lRetroPlatformScreenWidthRP / 2;
        break;
      case DISPLAYSCALE_2X:
        lScreenWidth = lRetroPlatformScreenWidthRP;
        break;
      default:
        fellowAddLog("RetroPlatformGetScreenWidthAdjusted(): WARNING: unknown display scaling factor 0x%x.\n",
          RetroPlatformGetDisplayScale());
        break;
    }  
  }
  else
    lScreenWidth = lRetroPlatformScreenWidthRP;

  return lScreenWidth;
}

ULO RetroPlatformGetScreenWidth(void) {
  return lRetroPlatformScreenWidthRP;
}

bool RetroPlatformGetScreenWindowed(void) {
  return bRetroPlatformScreenWindowed;
}

ULO RetroPlatformGetScreenHeight(void) {
   return lRetroPlatformScreenHeightRP;
}

ULO RetroPlatformGetScreenMode(void) {
  return lRetroPlatformScreenMode;
}

/** Translate the screenmode configured in the configuration file and pass it along to the RetroPlatform Player.
 */
static void RetroPlatformDetermineScreenModeFromConfig(
  struct RPScreenMode *RetroPlatformScreenMode, cfg *RetroPlatformConfig) {
  DWORD dwScreenMode = 0;

  if(RetroPlatformGetDisplayScale() == DISPLAYSCALE_1X)
    dwScreenMode |= RP_SCREENMODE_SCALE_1X;
  if(RetroPlatformGetDisplayScale() == DISPLAYSCALE_2X)
    dwScreenMode |= RP_SCREENMODE_SCALE_2X;

  if(RetroPlatformGetScreenWindowed())
    dwScreenMode |= RP_SCREENMODE_DISPLAY_WINDOW;
  else
    dwScreenMode |= RP_SCREENMODE_DISPLAY_FULLSCREEN_1;

  RetroPlatformScreenMode->dwScreenMode  = dwScreenMode;
  RetroPlatformScreenMode->hGuestWindow  = hRetroPlatformGuestWindow;
  RetroPlatformScreenMode->lTargetHeight = RetroPlatformGetScreenHeight();
  RetroPlatformScreenMode->lTargetWidth  = RetroPlatformGetScreenWidth();
  RetroPlatformScreenMode->lClipLeft     = RetroPlatformGetClippingOffsetLeft();
  RetroPlatformScreenMode->lClipTop      = RetroPlatformGetClippingOffsetTop();
  RetroPlatformScreenMode->lClipWidth    = RetroPlatformGetScreenWidth();
  RetroPlatformScreenMode->lClipHeight   = RetroPlatformGetScreenHeight();
  RetroPlatformScreenMode->dwClipFlags   = 0;
}

bool RetroPlatformGetEmulationPaused(void)
{
  return bRetroPlatformEmulationPaused;
}

/** Verify state of the emulation engine.
 *  @return TRUE, if emulation session if active, FALSE if not.
 */
BOOLE RetroPlatformGetEmulationState(void) {
  return bRetroPlatformEmulationState;
}


DISPLAYSCALE RetroPlatformGetDisplayScale(void) {
  return RetroPlatformDisplayScale;
}

BOOLE RetroPlatformGetMouseCaptureRequestedByHost(void) {
  return bRetroPlatformMouseCaptureRequestedByHost;
}

/** Determine the RetroPlatform host version.
 * 
 * @param[out] lpMainVersion main version number
 * @param[out] lpRevision revision number
 * @param[out] lpBuild build number
 * @return TRUE is successful, FALSE otherwise.
 */
static BOOLE RetroPlatformGetHostVersion(ULO *lpMainVersion, ULO *lpRevision, 
  ULO *lpBuild) {
	ULO lResult = 0;

	if (!RetroPlatformSendMessage(RP_IPC_TO_HOST_HOSTVERSION, 0, 0, NULL, 0, &RetroPlatformGuestInfo, (LRESULT*) &lResult))
		return FALSE;

	*lpMainVersion = RP_HOSTVERSION_MAJOR(lResult);
	*lpRevision    = RP_HOSTVERSION_MINOR(lResult);
	*lpBuild       = RP_HOSTVERSION_BUILD(lResult);
	return TRUE;
}

/** Verify if the emulator is operating in RetroPlatform mode.
 * 
 * Checks the value of the bRetroPlatformMode flag. It is set to TRUE, if a
 * RetroPlatform host ID has been passed along as a commandline parameter.
 * @return TRUE if WinFellow was called from Cloanto RetroPlatform, FALSE if not.
 */
inline bool RetroPlatformGetMode(void) {
  return bRetroPlatformMode;
}

/** Asynchronously post a message to the RetroPlatform host.
 * 
 * A message is posted to the host asynchronously, i.e. without waiting for
 * results.
 */
static BOOLE RetroPlatformPostMessage(ULO iMessage, WPARAM wParam, LPARAM lParam, const RPGUESTINFO *pGuestInfo) {
  BOOLE bResult;

  bResult = RPPostMessage(iMessage, wParam, lParam, pGuestInfo);

#ifdef _DEBUG
#ifndef RETRO_PLATFORM_LOG_VERBOSE
  if(iMessage != RP_IPC_TO_HOST_DEVICESEEK && iMessage != RP_IPC_TO_HOST_DEVICEACTIVITY) {
#endif !RETRO_PLATFORM_LOG_VERBOSE

  if(bResult)
    fellowAddLog("RetroPlatform posted message ([%s], %08x, %08x, %08x)\n",
      RetroPlatformGetMessageText(iMessage), iMessage - WM_APP, wParam, lParam);
  else
		fellowAddLog("RetroPlatform could not post message, error: %d\n", GetLastError());

#ifndef RETRO_PLATFORM_LOG_VERBOSE
  }
#endif

#endif // _DEBUG
	return bResult;
}

/** Post message to the player to signalize that the guest wants to escape the mouse cursor.
 */
void RetroPlatformPostEscaped(void) {
  RetroPlatformPostMessage(RP_IPC_TO_HOST_ESCAPED, 0, 0, &RetroPlatformGuestInfo);
}

/** Control status of the RetroPlatform hard drive LEDs.
 *
 * Sends LED status changes to the RetroPlatform host in the form of RP_IPC_TO_HOST_DEVICEACTIVITY messages,
 * so that hard drive read and write activity can be displayed, and detected (undo functionality uses write
 * messages as fallback method to detect changed floppy images).
 * @param[in] lHardDriveNo   hard drive index (0-...)
 * @param[in] bActive        flag indicating disk access (active/inactive)
 * @param[in] bWriteActivity flag indicating type of access (write/read)
 * @return TRUE if message sent successfully, FALSE otherwise. 
 * @callergraph
 */
void RetroPlatformPostHardDriveLED(const ULO lHardDriveNo, const BOOLE bActive, const BOOLE bWriteActivity) {
  static int oldleds[FHFILE_MAX_DEVICES];
  static ULONGLONG lastsent[FHFILE_MAX_DEVICES];
	int state;

	if(!bRetroPlatformInitialized)
    return;

	state = bActive ? 1 : 0;
	state |= bWriteActivity ? 2 : 0;

	if (state == oldleds[lHardDriveNo])
		return;
  else
    oldleds[lHardDriveNo] = state;

  if (bActive && (lastsent[lHardDriveNo] + RETRO_PLATFORM_HARDDRIVE_BLINK_MSECS < RetroPlatformGetTime()) 
  || (bActive && bWriteActivity)) {
		RetroPlatformPostMessage (RP_IPC_TO_HOST_DEVICEACTIVITY, MAKEWORD (RP_DEVICECATEGORY_HD, lHardDriveNo),
			MAKELONG (RETRO_PLATFORM_HARDDRIVE_BLINK_MSECS, bWriteActivity ? RP_DEVICEACTIVITY_WRITE : RP_DEVICEACTIVITY_READ), 
      &RetroPlatformGuestInfo);
    lastsent[lHardDriveNo] = RetroPlatformGetTime();
	}
  else
    return;
}

/** Control status of the RetroPlatform floppy drive LEDs.
 *
 * Sends LED status changes to the RetroPlatform host in the form of RP_IPC_TO_HOST_DEVICEACTIVITY messages,
 * so that floppy read and write activity can be displayed, and detected (undo functionality uses write
 * messages as fallback method to detect changed floppy images).
 * @param[in] lFloppyDriveNo floppy drive index (0-3)
 * @param[in] bMotorActive   state of floppy drive motor (active/inactive)
 * @param[in] bWriteActivity type of access (write/read)
 * @return TRUE if message sent successfully, FALSE otherwise. 
 * @callergraph
 */
BOOLE RetroPlatformSendFloppyDriveLED(const ULO lFloppyDriveNo, const BOOLE bMotorActive, const BOOLE bWriteActivity) {
	if(lFloppyDriveNo > 3) 
    return FALSE;

  return RetroPlatformPostMessage(RP_IPC_TO_HOST_DEVICEACTIVITY, MAKEWORD (RP_DEVICECATEGORY_FLOPPY, lFloppyDriveNo),
			MAKELONG (bMotorActive ? -1 : 0, (bWriteActivity) ? RP_DEVICEACTIVITY_WRITE : RP_DEVICEACTIVITY_READ) , &RetroPlatformGuestInfo);
}

/** Send content of floppy drive to RetroPlatform host.
 * The read-only state is determined and sent here, however at this point 
 * it is usually wrong, as floppySetDiskImage only reflects the ability to write
 * to the file in the writeprot flag.
 * The actual state within the config is configured in a separate call within 
 * cfgManagerConfigurationActivate - therefore an update message is sent later.
 * @param[in] lFloppyDriveNo floppy drive index (0-3)
 * @param[in] szImageName ANSI string containing the floppy image name
 * @param[in] bWriteProtected flag indicating the read-only state of the drive
 * @return TRUE if message sent successfully, FALSE otherwise.
 * @sa RetroPlatformSendFloppyDriveReadOnly
 * @callergraph
 */
BOOLE RetroPlatformSendFloppyDriveContent(const ULO lFloppyDriveNo, const STR *szImageName, const BOOLE bWriteProtected) {
  BOOLE bResult;
  struct RPDeviceContent rpDeviceContent = { 0 };

  if (!bRetroPlatformInitialized)
		return FALSE;

  if(!floppy[lFloppyDriveNo].enabled)
    return FALSE;

	rpDeviceContent.btDeviceCategory = RP_DEVICECATEGORY_FLOPPY;
	rpDeviceContent.btDeviceNumber = lFloppyDriveNo;
	rpDeviceContent.dwInputDevice = 0;
	if (szImageName)
		mbstowcs(rpDeviceContent.szContent, szImageName, CFG_FILENAME_LENGTH);
  else
    wcscpy(rpDeviceContent.szContent, L"");
  rpDeviceContent.dwFlags = (bWriteProtected ? RP_DEVICEFLAGS_RW_READONLY : RP_DEVICEFLAGS_RW_READWRITE);

#ifdef _DEBUG
	fellowAddLog("RP_IPC_TO_HOST_DEVICECONTENT cat=%d num=%d type=%d '%s'\n",
	  rpDeviceContent.btDeviceCategory, rpDeviceContent.btDeviceNumber, 
    rpDeviceContent.dwInputDevice, rpDeviceContent.szContent);
#endif

  bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_DEVICECONTENT, 0, 0, 
    &rpDeviceContent, sizeof(struct RPDeviceContent), &RetroPlatformGuestInfo,
    NULL);

  fellowAddLog("RetroPlatformSendFloppyDriveContent(%d, '%s'): %s.\n",
    lFloppyDriveNo, szImageName, bResult ? "successful" : "failed");
  
  return bResult;
}

/** Send actual write protection state of drive to RetroPlatform host.
 * Ignores drives that are not enabled.
 * @param[in] lFloppyDriveNo floppy drive index (0-3)
 * @param[in] bWriteProtected flag indicating the read-only state of the drive
 * @return TRUE if message sent successfully, FALSE otherwise.
 * @callergraph
 */
BOOLE RetroPlatformSendFloppyDriveReadOnly(const ULO lFloppyDriveNo, const BOOLE bWriteProtected) {
  BOOLE bResult;

  if(!bRetroPlatformInitialized)
    return FALSE;

  if(!floppy[lFloppyDriveNo].enabled)
    return FALSE;

  bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_DEVICEREADWRITE, 
    MAKEWORD(RP_DEVICECATEGORY_FLOPPY, lFloppyDriveNo), 
    bWriteProtected ? RP_DEVICE_READONLY : RP_DEVICE_READWRITE, NULL, 
    0, &RetroPlatformGuestInfo, NULL);

  fellowAddLog("RetroPlatformSendFloppyDriveReadOnly(): %s.\n",
    bResult ? "successful" : "failed");
    
  return bResult;
}

/** Send floppy turbo mode state to RetroPlatform host.
 * @param[in] bTurbo flag indicating state of turbo mode
 * @return TRUE if message sent successfully, FALSE otherwise.
 * @callergraph
 */
BOOLE RetroPlatformSendFloppyTurbo(const BOOLE bTurbo) {
  BOOLE bResult;

  if(!bRetroPlatformInitialized)
    return FALSE;

  bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_TURBO, RP_TURBO_FLOPPY, 
    bTurbo ? RP_TURBO_FLOPPY : 0, NULL, 0, &RetroPlatformGuestInfo, NULL);

  fellowAddLog("RetroPlatformSendFloppyDriveTurbo(): %s.\n",
    bResult ? "successful" : "failed");
    
  return bResult;
}

/**
 * Send floppy drive seek events to RetroPlatform host.
 * 
 * Will notify the RetroPlatform player about changes in the drive head position.
 * @param[in] lFloppyDriveNo index of floppy drive
 * @param[in] lTrackNo index of floppy track
 * @return TRUE if message sent successfully, FALSE otherwise.
 * @callergraph
 */
BOOLE RetroPlatformSendFloppyDriveSeek(const ULO lFloppyDriveNo, const ULO lTrackNo) {
	if (!bRetroPlatformInitialized)
		return FALSE;

	return RetroPlatformPostMessage(RP_IPC_TO_HOST_DEVICESEEK, 
    MAKEWORD (RP_DEVICECATEGORY_FLOPPY, lFloppyDriveNo), lTrackNo, &RetroPlatformGuestInfo);
}

/** 
 * Send gameport activity to RetroPlatform host.
 */
BOOLE RetroPlatformSendGameportActivity(const ULO lGameport, const ULO lGameportMask) {
	if (!bRetroPlatformInitialized)
		return FALSE;

	if (lGameport >= RETRO_PLATFORM_NUM_GAMEPORTS)
		return FALSE;

	RetroPlatformPostMessage(RP_IPC_TO_HOST_DEVICEACTIVITY, MAKEWORD(RP_DEVICECATEGORY_INPUTPORT, lGameport),
		lGameportMask, &RetroPlatformGuestInfo);

  return TRUE;
}

/** Send content of hard drive to RetroPlatform host.
 * @param[in] lHardDriveNo hard drive index (0-...)
 * @param[in] szImageName ANSI string containing the floppy image name
 * @param[in] bWriteProtected flag indicating the read-only state of the drive
 * @return TRUE if message sent successfully, FALSE otherwise.
 * @callergraph
 */
BOOLE RetroPlatformSendHardDriveContent(const ULO lHardDriveNo, const STR *szImageName, const BOOLE bWriteProtected) {
  BOOLE bResult;
  struct RPDeviceContent rpDeviceContent = { 0 };

  if (!bRetroPlatformInitialized)
		return FALSE;

	rpDeviceContent.btDeviceCategory = RP_DEVICECATEGORY_HD;
	rpDeviceContent.btDeviceNumber = lHardDriveNo;
	rpDeviceContent.dwInputDevice = 0;
	if (szImageName)
		mbstowcs(rpDeviceContent.szContent, szImageName, CFG_FILENAME_LENGTH);
  else
    wcscpy(rpDeviceContent.szContent, L"");
  rpDeviceContent.dwFlags = (bWriteProtected ? RP_DEVICEFLAGS_RW_READONLY : RP_DEVICEFLAGS_RW_READWRITE);

#ifdef _DEBUG
	fellowAddLog("RP_IPC_TO_HOST_DEVICECONTENT cat=%d num=%d type=%d '%s'\n",
	  rpDeviceContent.btDeviceCategory, rpDeviceContent.btDeviceNumber, 
    rpDeviceContent.dwInputDevice, rpDeviceContent.szContent);
#endif

  bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_DEVICECONTENT, 0, 0, 
    &rpDeviceContent, sizeof(struct RPDeviceContent), &RetroPlatformGuestInfo,
    NULL);

  fellowAddLog("RetroPlatformSendHardDriveContent(%d, '%s'): %s.\n",
    lHardDriveNo, szImageName, bResult ? "successful" : "failed");
  
  return bResult;
}

/** Control status of power LED in RetroPlatform player.
 *
 * Examines the current on/off state of the emulator session and sends it to the RetroPlatform player.
 * @param[in] wIntensityPercent intensity of the power LED in percent, with 0 being off, 100 being full intensity.
 * @return TRUE, if valid value was passed, FALSE if invalid value.
 */
static BOOLE RetroPlatformSendPowerLEDIntensityPercent(const WPARAM wIntensityPercent) {
  if(wIntensityPercent <= 100 && wIntensityPercent >= 0)
    return RetroPlatformPostMessage(RP_IPC_TO_HOST_POWERLED, 
      wIntensityPercent, 0, &RetroPlatformGuestInfo);
  else
    return FALSE;
}

/** Set RetroPlatform escape key.
 * 
 * Called during parsing of the command-line parameters, which is why the keyboard modules
 * have to be initialized before the config modules, as we use the key mappings here.
 */
void RetroPlatformSetEscapeKey(const char *szEscapeKey) {
  lRetroPlatformEscapeKey = atoi(szEscapeKey);
  lRetroPlatformEscapeKey = kbddrv_DIK_to_symbol[lRetroPlatformEscapeKey];
  fellowAddLog("RetroPlatformSetEscapeKey(): escape key configured to %s.\n", kbdDrvKeyString(lRetroPlatformEscapeKey));
}

void RetroPlatformSetEscapeKeyHoldTime(const char *szEscapeHoldTime) {
  lRetroPlatformEscapeKeyHoldTime = atoi(szEscapeHoldTime);
  fellowAddLog("RetroPlatformSetEscapeKeyHoldTime(): escape hold time configured to %d.\n", lRetroPlatformEscapeKeyHoldTime);
}

ULONGLONG RetroPlatformSetEscapeKeyHeld(const bool bEscapeKeyHeld) {
  ULONGLONG t = 0;

  if(bEscapeKeyHeld) {
    if(lRetroPlatformEscapeKeyHeldSince == 0) {
      lRetroPlatformEscapeKeyHeldSince = RetroPlatformGetTime();
    }
  }
  else {
    if(lRetroPlatformEscapeKeyHeldSince) {
      t = RetroPlatformGetTime() - lRetroPlatformEscapeKeyHeldSince;
      lRetroPlatformEscapeKeyHeldSince = 0;
    }
  }
  return t;
}

void RetroPlatformSetEscapeKeySimulatedTargetTime(const ULONGLONG tTargetTime) {
  lRetroPlatformEscapeKeySimulatedTargetTime = tTargetTime;
}

void RetroPlatformSetEmulationState(const BOOLE bNewState) {
  if(bRetroPlatformEmulationState != bNewState) {
    bRetroPlatformEmulationState = bNewState;
    fellowAddLog("RetroPlatformSetEmulationState(): state set to %s.\n", bNewState ? "active" : "inactive");
    RetroPlatformSendPowerLEDIntensityPercent(bNewState ? 100 : 0);
  }
}

void RetroPlatformSetEmulationPaused(const bool bPaused) {
  if(bPaused != bRetroPlatformEmulationPaused) {
    bRetroPlatformEmulationPaused = bPaused;

    fellowAddLog("RetroPlatformSetEmulationPaused(): emulation is now %s.\n",
      bPaused ? "paused" : "active");
  }
}

void RetroPlatformSetHostID(const char *szHostID) {
  strncpy(szRetroPlatformHostID, szHostID, CFG_FILENAME_LENGTH);
  fellowAddLog("RetroPlatformSetHostID(): host ID configured to %s.\n", szRetroPlatformHostID);
}

void RetroPlatformSetMode(const BOOLE bRPMode) {
  bRetroPlatformMode = bRPMode;
  fellowAddLog("RetroPlatformSetMode(): entering RetroPlatform (headless) mode.\n");
}

/** Set screen height.
 */
void RetroPlatformSetScreenHeight(const ULO lHeight) {
  lRetroPlatformScreenHeightRP = lHeight;

  fellowAddLog("RetroPlatformSetScreenHeight(): height configured to %u\n", 
    lRetroPlatformScreenHeightRP);
}

/** Set screen width.
 */
void RetroPlatformSetScreenWidth(const ULO lWidth) {
  lRetroPlatformScreenWidthRP = lWidth;

  fellowAddLog("RetroPlatformSetScreenWidth(): width configured to %u\n", 
    lRetroPlatformScreenWidthRP);
}

void RetroPlatformSetScreenWindowed(const bool bWindowed) {  
  bRetroPlatformScreenWindowed = bWindowed;

  if(RetroPlatformConfig != NULL) {
    cfgSetScreenWindowed(RetroPlatformConfig, bWindowed);
  }

  fellowAddLog("RetroPlatformSetScreenWindowed(): configured to %s\n",
    bWindowed ? "true" : "false");
}

void RetroPlatformSetDisplayScale(const DISPLAYSCALE displayscale) {
  RetroPlatformDisplayScale = displayscale;

  if(RetroPlatformConfig != NULL) {
    cfgSetDisplayScale(RetroPlatformConfig, displayscale);
    drawSetDisplayScale(displayscale);
  }

  fellowAddLog("RetroPlatformSetDisplayScale(): display scale configured to %s\n",
    displayscale == DISPLAYSCALE_1X ? "1x" : "2x");
}

void RetroPlatformSetScreenMode(const char *szScreenMode) {
  ULO lScalingFactor = 0;

  lRetroPlatformScreenMode = atol(szScreenMode);
  fellowAddLog("RetroPlatformSetScreenMode(): screen mode configured to %u.\n", lRetroPlatformScreenMode);

  lScalingFactor = RP_SCREENMODE_SCALE(lRetroPlatformScreenMode);

  switch(lScalingFactor)
  {
    case RP_SCREENMODE_SCALE_1X:
      RetroPlatformSetDisplayScale(DISPLAYSCALE_1X);
      break;
    case RP_SCREENMODE_SCALE_2X:
      RetroPlatformSetDisplayScale(DISPLAYSCALE_2X);
      break;
    default:
      fellowAddLog("RetroPlatformSetScreenMode(): WARNING: unknown display scaling factor 0x%x\n",
        lScalingFactor);
  }
}

void RetroPlatformSetScreenModeStruct(struct RPScreenMode *sm) {
  ULO lScalingFactor = 0, lDisplay = 0;

  lScalingFactor = RP_SCREENMODE_SCALE  (sm->dwScreenMode);
  lDisplay       = RP_SCREENMODE_DISPLAY(sm->dwScreenMode);

#ifdef _DEBUG
  fellowAddLog("RetroPlatformSetScreenModeStruct(): dwScreenMode=0x%x, dwClipFlags=0x%x, lTargetWidth=%u, lTargetHeight=%u\n", 
    sm->dwScreenMode, sm->dwClipFlags, sm->lTargetWidth, sm->lTargetHeight);

  fellowAddLog("RetroPlatformSetScreenModeStruct(): lClipWidth=%u, lClipHeight=%u, lClipLeft=%u, lClipTop=%u\n", 
    sm->lClipWidth, sm->lClipHeight, sm->lClipLeft, sm->lClipTop);

  fellowAddLog("RetroPlatformSetScreenModeStruct(): lScalingFactor=0x%x, lDisplay=0x%x\n",
    lScalingFactor, lDisplay);
#endif

  if(lDisplay == 0) {
    RetroPlatformSetScreenWindowed(true);

    switch(lScalingFactor)
    {
      case RP_SCREENMODE_SCALE_1X:
        RetroPlatformSetDisplayScale(DISPLAYSCALE_1X);
        break;
      case RP_SCREENMODE_SCALE_2X:
        RetroPlatformSetDisplayScale(DISPLAYSCALE_2X);
        break;
      default:
        fellowAddLog("RetroPlatformSetScreenModeStruct(): WARNING: unknown windowed display scaling factor 0x%x.\n", lScalingFactor);
    }
  }

  if(lDisplay == 1) {
    RetroPlatformSetScreenWindowed(false);

    switch(lScalingFactor)
    {
      case RP_SCREENMODE_SCALE_MAX:
        // automatically scale to max - set in conjunction with fullscreen mode
        RetroPlatformSetDisplayScale(DISPLAYSCALE_1X);
        break;
      default:
        fellowAddLog("RetroPlatformSetScreenModeStruct(): WARNING: unknown fullscreen 1 display scaling factor 0x%x.\n", lScalingFactor);
    }
  }

  RetroPlatformSetClippingOffsetLeft(sm->lClipLeft);
  RetroPlatformSetClippingOffsetTop (sm->lClipTop);
  RetroPlatformSetScreenHeight      (sm->lClipHeight);
  cfgSetScreenHeight(RetroPlatformConfig, sm->lClipHeight);
  RetroPlatformSetScreenWidth       (sm->lClipWidth);
  cfgSetScreenWidth(RetroPlatformConfig, sm->lClipWidth);

  // Resume emulation, as graph module will crash otherwise if emulation is paused.
  // As the pause mode is not changed, after the restart of the session it will be
  // paused again.
  gfxDrvRunEventSet();

  gfxDrvRegisterRetroPlatformScreenMode();

  fellowRequestEmulationStop();
}

void RetroPlatformSetWindowInstance(HINSTANCE hInstance) {
  fellowAddLog("RetroPlatformSetWindowInstance():  window instance set to %d.\n", hInstance);
  hRetroPlatformWindowInstance = hInstance;
}

/** host message function that is used as callback to receive IPC messages from the host.
 */
static LRESULT CALLBACK RetroPlatformHostMessageFunction(UINT uMessage, WPARAM wParam, LPARAM lParam,
LPCVOID pData, DWORD dwDataSize, LPARAM lMsgFunctionParam) {

#ifdef _DEBUG
  fellowAddLog("RetroPlatformHostMessageFunction(%s [%d], %08x, %08x, %08x, %d, %08x)\n",
    RetroPlatformGetMessageText(uMessage), uMessage - WM_APP, wParam, lParam, pData, dwDataSize, lMsgFunctionParam);
#endif

  if (uMessage == RP_IPC_TO_GUEST_DEVICECONTENT) {
    struct RPDeviceContent *dc = (struct RPDeviceContent*)pData;

#ifdef _DEBUG
    fellowAddLog(" Cat=%d Num=%d Flags=%08x '%s'\n",
      dc->btDeviceCategory, dc->btDeviceNumber, dc->dwFlags, dc->szContent);
#endif
  }

  switch(uMessage)
  {
  default:
    fellowAddLog("RetroPlatformHostMessageFunction: Unknown or unsupported command 0x%x\n", uMessage);
    break;
  case RP_IPC_TO_GUEST_PING:
    return TRUE;
  case RP_IPC_TO_GUEST_CLOSE:
    fellowAddLog("RetroPlatformHostMessageFunction: received close event.\n");
    fellowRequestEmulationStop();
    gfxDrvRunEventSet();
    bRetroPlatformEmulatorQuit = TRUE;
    return TRUE;
  case RP_IPC_TO_GUEST_RESET:
    if(wParam == RP_RESET_HARD)
      fellowPreStartReset(TRUE);
    RetroPlatformSetEmulationPaused(false);
    gfxDrvRunEventSet();
    fellowRequestEmulationStop();
    return TRUE;
  case RP_IPC_TO_GUEST_TURBO:
    if (wParam & RP_TURBO_CPU) {
      static ULO lOriginalSpeed = 0;
      
      if(lParam & RP_TURBO_CPU) {
        fellowAddLog("RetroPlatformHostMessageFunction(): enabling CPU turbo mode...\n");
        lOriginalSpeed = cfgGetCPUSpeed(RetroPlatformConfig);
        cpuIntegrationSetSpeed(0);
        cpuIntegrationCalculateMultiplier();
        busDetermineCpuInstructionEventHandler();
        fellowRequestEmulationStop();
      }
      else {
        fellowAddLog("RetroPlatformHostMessageFunction(): disabling CPU turbo mode, reverting back to speed level %u...\n",
          lOriginalSpeed);
        cpuIntegrationSetSpeed(lOriginalSpeed);
        cpuIntegrationCalculateMultiplier();
        busDetermineCpuInstructionEventHandler();
        fellowRequestEmulationStop();
      }
    }
    if (wParam & RP_TURBO_FLOPPY)
      floppySetFastDMA(lParam & RP_TURBO_FLOPPY ? TRUE : FALSE);
    return TRUE;
  case RP_IPC_TO_GUEST_PAUSE:
    if(wParam != 0) { // pause emulation
      fellowAddLog("RetroPlatformHostMessageFunction: received pause event.\n");
      gfxDrvRunEventReset();
      RetroPlatformSetEmulationPaused(true);
      RetroPlatformSetEmulationState(FALSE);
      return 1;
    }
    else { // resume emulation
      fellowAddLog("RetroPlatformHostMessageFunction: received resume event, requesting start.\n");
      gfxDrvRunEventSet();
      RetroPlatformSetEmulationPaused(false);
      RetroPlatformSetEmulationState(TRUE);
      return 1;
    }
  case RP_IPC_TO_GUEST_VOLUME:
    soundDrvDSoundSetCurrentSoundDeviceVolume(wParam);
    return TRUE;
  case RP_IPC_TO_GUEST_ESCAPEKEY:
    lRetroPlatformEscapeKey         = wParam;
    lRetroPlatformEscapeKeyHoldTime = lParam;
    return TRUE;
  case RP_IPC_TO_GUEST_MOUSECAPTURE:
    fellowAddLog("hostmsgfunction: mousecapture: %d.\n", wParam & RP_MOUSECAPTURE_CAPTURED);
    mouseDrvSetFocus(wParam & RP_MOUSECAPTURE_CAPTURED ? TRUE : FALSE, TRUE);
    return TRUE;
  case RP_IPC_TO_GUEST_DEVICECONTENT:
    {
      struct RPDeviceContent *dc = (struct RPDeviceContent*)pData;
      STR n[CFG_FILENAME_LENGTH] = "";
      int num = dc->btDeviceNumber;
      int ok = FALSE;
      wcstombs(n, dc->szContent, CFG_FILENAME_LENGTH);
      switch (dc->btDeviceCategory) {
	case RP_DEVICECATEGORY_FLOPPY:
	  if (n == NULL || n[0] == 0) {
            fellowAddLog("RetroPlatformHostMessageFunction: remove floppy disk from drive %d.\n", num);
            floppyImageRemove(num);
          }
          else {
            fellowAddLog("RetroPlatformHostMessageFunction: set floppy image for drive %d to %s.\n",
              num, n);
            floppySetDiskImage(num, n);
          }
	  ok = TRUE;
	  break;
        case RP_DEVICECATEGORY_INPUTPORT:
          ok = RetroPlatformConnectInputDeviceToPort(num, dc->dwInputDevice, dc->dwFlags, n);
          break;
        case RP_DEVICECATEGORY_CD:
          ok = FALSE;
          break;
	    } 
	    return ok;
    }
  case RP_IPC_TO_GUEST_SCREENCAPTURE:
    {		
      struct RPScreenCapture *rpsc = (struct RPScreenCapture*)pData;
      STR szScreenFiltered[CFG_FILENAME_LENGTH] = "", szScreenRaw[CFG_FILENAME_LENGTH] = "";

      wcstombs(szScreenFiltered, rpsc->szScreenFiltered, CFG_FILENAME_LENGTH);
      wcstombs(szScreenRaw,      rpsc->szScreenRaw,      CFG_FILENAME_LENGTH);
      		
      if (szScreenFiltered[0] || szScreenRaw[0]) {
        BOOLE bResult = TRUE;
	DWORD dResult = 0;
	fellowAddLog("RetroPlatformHostMessageFunction(): screenshot request received; filtered '%s', raw '%s'\n", 
          szScreenFiltered, szScreenRaw);

        if(szScreenFiltered[0]) {
          bResult = gfxDrvTakeScreenShot(TRUE, szScreenFiltered);
        }

        if(szScreenRaw[0]) {
          bResult &= gfxDrvTakeScreenShot(FALSE, szScreenRaw);
        }

        if(bResult) {
          dResult |= RP_GUESTSCREENFLAGS_MODE_PAL;
          return dResult;
        }
        else 
          return RP_SCREENCAPTURE_ERROR;		
      }
    }
    return RP_SCREENCAPTURE_ERROR;
  case RP_IPC_TO_GUEST_SCREENMODE:
    {
      struct RPScreenMode *sm = (struct RPScreenMode *) pData;
      RetroPlatformSetScreenModeStruct(sm);
      return (LRESULT)INVALID_HANDLE_VALUE;
    }
  case RP_IPC_TO_GUEST_DEVICEREADWRITE:	
    {		
      DWORD ret = FALSE;
      int device = LOBYTE(wParam);
      if(device == RP_DEVICECATEGORY_FLOPPY) {
	int num = HIBYTE(wParam);
	if (lParam == RP_DEVICE_READONLY || lParam == RP_DEVICE_READWRITE) {
          floppySetReadOnly(num, lParam == RP_DEVICE_READONLY ? TRUE : FALSE);				
          ret = TRUE;			
        }		
      } 		
      return ret ? (LPARAM) 1 : 0;	
    }
  case RP_IPC_TO_GUEST_FLUSH:
    return 1;
  case RP_IPC_TO_GUEST_GUESTAPIVERSION:  
    {
      return MAKELONG(3, 4);
    }
  }
  return FALSE;
}

BOOLE RetroPlatformSendActivate(const BOOLE bActive, const LPARAM lParam) {
  BOOLE bResult;

	bResult = RetroPlatformSendMessage(bActive ? RP_IPC_TO_HOST_ACTIVATED : RP_IPC_TO_HOST_DEACTIVATED, 
    0, lParam, NULL, 0, &RetroPlatformGuestInfo, NULL);

  fellowAddLog("RetroPlatformSendActive(): %s.\n", bResult ? "successful" : "failed");

  return bResult;
}

/** Notify the player that the user request to close the emulation session.
 *
 *  The player will examine if changes to the package were performed that
 *  require user feedback (media changed where undo is enabled, parameters
 *  like e.g. clipping were changed, ...).
 *  The user can choose what to commit and proceed with quitting, or cancel.
 *  The player can then either notify the emulator to quit via an IPC message
 *  RP_IPC_TO_GUEST_CLOSE, or do nothing and let the session continue.
 */
BOOLE RetroPlatformSendClose(void) {
  BOOLE bResult;

  bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_CLOSE, 0, 0, NULL, 0, 
    &RetroPlatformGuestInfo, NULL);

  fellowAddLog("RetroPlatformSendClose(): %s.\n", bResult ? "sucessful" : "failed");

  return bResult;
}

/** Send enable/disable messages to the RetroPlatform player.
 * 
 * These are sent on WM_ENABLE messages.
 */
BOOLE RetroPlatformSendEnable(const BOOLE bEnabled) {
  LRESULT lResult;
  BOOLE bResult;

  if (!bRetroPlatformInitialized)
		return FALSE;

	bResult = RetroPlatformSendMessage(bEnabled ? RP_IPC_TO_HOST_ENABLED : RP_IPC_TO_HOST_DISABLED,
    0, 0, NULL, 0, &RetroPlatformGuestInfo, &lResult);

  fellowAddLog("RetroPlatformSendEnable() %s, result was %d.\n", 
    bResult ? "successful" : "failed", lResult);
  
  return bResult;
}

/** Send list of features supported by the guest to the RetroPlatform host.
 *
 * An RP_IPC_TO_HOST_FEATURES message is sent to the host, with flags indicating the 
 * features supported by the guest.
 * @return TRUE if message was sent successfully, FALSE otherwise.
 */
static BOOLE RetroPlatformSendFeatures(void) {
  DWORD dFeatureFlags;
  LRESULT lResult;
  BOOLE bResult;

  dFeatureFlags =  RP_FEATURE_POWERLED | RP_FEATURE_SCREEN1X | RP_FEATURE_PAUSE;
  dFeatureFlags |= RP_FEATURE_TURBO_FLOPPY | RP_FEATURE_TURBO_CPU;
  dFeatureFlags |= RP_FEATURE_VOLUME | RP_FEATURE_SCANLINES | RP_FEATURE_DEVICEREADWRITE;
  dFeatureFlags |= RP_FEATURE_INPUTDEVICE_MOUSE | RP_FEATURE_INPUTDEVICE_JOYSTICK;
  dFeatureFlags |= RP_FEATURE_SCREEN2X;

#ifdef _DEBUG
  dFeatureFlags |= RP_FEATURE_SCREENCAPTURE;
  dFeatureFlags |= RP_FEATURE_FULLSCREEN;
#endif

  // currently missing features: RP_FEATURE_FULLSCREEN, RP_FEATURE_SCREENCAPTURE,
  // RP_FEATURE_STATE, RP_FEATURE_SCALING_SUBPIXEL, RP_FEATURE_SCALING_STRETCH
  // RP_FEATURE_INPUTDEVICE_GAMEPAD, RP_FEATURE_INPUTDEVICE_JOYPAD, 
  // RP_FEATURE_INPUTDEVICE_ANALOGSTICK, RP_FEATURE_INPUTDEVICE_LIGHTPEN

  bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_FEATURES, dFeatureFlags, 
    0, NULL, 0, &RetroPlatformGuestInfo, &lResult);
  
  fellowAddLog("RetroPlatformSendFeatures() %s, result was %d.\n", 
    bResult ? "successful" : "failed", lResult);
 
  return bResult;
}

/** Send list of enabled floppy drives to the RetroPlatform host.
 *
 * An RP_IPC_TO_HOST_DEVICES message is sent to the host, indicating the floppy drives 
 * enabled in the guest. Must be called after the activation of the config, and before
 * sending the screen mode.
 * @return TRUE if message was sent successfully, FALSE otherwise.
 */
static BOOLE RetroPlatformSendEnabledFloppyDrives(void) {
	DWORD dFeatureFlags;
  LRESULT lResult;
  BOOLE bResult;
  int i;

  dFeatureFlags = 0;
  for(i = 0; i < 4; i++) {
#ifdef _DEBUG
    fellowAddLog("floppy drive %d is %s.\n", i, floppy[i].enabled ? "enabled" : "disabled");
#endif
  if(floppy[i].enabled)
    dFeatureFlags |= 1 << i;
  }

  bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_DEVICES, RP_DEVICECATEGORY_FLOPPY, 
    dFeatureFlags, NULL, 0, &RetroPlatformGuestInfo, &lResult);
 
  fellowAddLog("RetroPlatformSendEnabledFloppyDrives() %s, result was %d.\n", 
    bResult ? "successful" : "failed", lResult);

  return bResult;
}

/** Send list of enabled hard drives to the RetroPlatform host.
 *
 * An RP_IPC_TO_HOST_DEVICES message is sent to the host, indicating the hard drives 
 * enabled in the guest. Must be called after the activation of the config, and before
 * sending the screen mode.
 * @return TRUE if message was sent successfully, FALSE otherwise.
 */
static BOOLE RetroPlatformSendEnabledHardDrives(void) {
	DWORD dFeatureFlags = 0;
  LRESULT lResult;
  BOOLE bResult;
  ULO i;

  fellowAddLog("RetroPlatformSendEnabledHardDrives(): %d hard drives are enabled.\n", 
    cfgGetHardfileCount(RetroPlatformConfig));

	for(i = 0; i < cfgGetHardfileCount(RetroPlatformConfig); i++)
			dFeatureFlags |= 1 << i;

  bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_DEVICES, RP_DEVICECATEGORY_HD, 
    dFeatureFlags, NULL, 0, &RetroPlatformGuestInfo, &lResult);
 
  fellowAddLog("RetroPlatformSendEnabledHardDrives() %s, result was %d.\n", 
    bResult ? "successful" : "failed", lResult);

  return bResult;
}

static BOOLE RetroPlatformSendGameports(const ULO lNumGameports) {
  LRESULT lResult;
  BOOLE bResult;

  bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_DEVICES, 
    RP_DEVICECATEGORY_INPUTPORT, (1 << lNumGameports) - 1, NULL, 0, 
    &RetroPlatformGuestInfo, &lResult);

  fellowAddLog("RetroPlatformSendGameports() %s, result was %d.\n", 
    bResult ? "successful" : "failed", lResult);
   
  return bResult;
}

/** Send a single input device to the RetroPlatform player.
 */
BOOLE RetroPlatformSendInputDevice(const DWORD dwHostInputType,
  const DWORD dwInputDeviceFeatures, const DWORD dwFlags,
  const WCHAR *szHostInputID, const WCHAR *szHostInputName) {
  LRESULT lResult;
  BOOLE bResult;
  STR szHostInputNameA[CFG_FILENAME_LENGTH];
	struct RPInputDeviceDescription rpInputDevDesc;

  wcscpy(rpInputDevDesc.szHostInputID, szHostInputID);
  wcscpy(rpInputDevDesc.szHostInputName, szHostInputName);
  rpInputDevDesc.dwHostInputType = dwHostInputType;
  rpInputDevDesc.dwHostInputVendorID = 0;
  rpInputDevDesc.dwHostInputProductID = 0;
  rpInputDevDesc.dwInputDeviceFeatures = dwInputDeviceFeatures;
  rpInputDevDesc.dwFlags = dwFlags;

  wcstombs(szHostInputNameA, szHostInputName, CFG_FILENAME_LENGTH);

  bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_INPUTDEVICE, 0, 0, 
    &rpInputDevDesc, sizeof rpInputDevDesc, &RetroPlatformGuestInfo, &lResult);
  
#ifdef _DEBUG
    fellowAddLog("RetroPlatformSendInputDevice() - '%s' %s, result was %d.\n", 
      szHostInputNameA, bResult ? "successful" : "failed", lResult);
#endif
   
  return bResult;
}

/** Send list of available input device options to the RetroPlatform player.
 *
 * The emulator is supposed to enumerate the Windows devices and identify
 * them via unique IDs; joysticks are sent after enumeration during
 * emulator session start, other devices are sent here
 */
static BOOLE RetroPlatformSendInputDevices(void) {
  BOOLE bResult = TRUE;

  // Windows mouse
  if(!RetroPlatformSendInputDevice(RP_HOSTINPUT_MOUSE, 
    RP_FEATURE_INPUTDEVICE_MOUSE | RP_FEATURE_INPUTDEVICE_LIGHTPEN,
    RP_HOSTINPUTFLAGS_MOUSE_SMART,
    L"GP_MOUSE0",
    L"Windows Mouse")) bResult = FALSE;

  // report custom keyboard layout support
  if(!RetroPlatformSendInputDevice(RP_HOSTINPUT_KEYBOARD, 
    RP_FEATURE_INPUTDEVICE_JOYSTICK,
    0,
    L"GP_JOYKEYCUSTOM",
    L"KeyboardCustom")) bResult = FALSE;

  // report available joysticks
  RetroPlatformEnumerateJoysticks();

  // end enumeration
  if(!RetroPlatformSendInputDevice(RP_HOSTINPUT_END, 
    0,
    0,
    L"RP_END",
    L"END")) bResult = FALSE;

  fellowAddLog("RetroPlatformSendInputDevices() %s.\n", bResult ? "successful" : "failed");
 
  return bResult;
}

BOOLE RetroPlatformSendMouseCapture(const BOOLE bActive) {
  BOOLE bResult;
  WPARAM wFlags = (WPARAM) 0;

	if (!bRetroPlatformInitialized)
		return FALSE;

	if (bActive)
		wFlags |= RP_MOUSECAPTURE_CAPTURED;

	bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_MOUSECAPTURE, wFlags, 0, NULL, 0,
    &RetroPlatformGuestInfo, NULL);

  fellowAddLog("RetroPlatformSendMouseCapture(): %s.\n",
    bResult ? "successful" : "failed");

  return bResult;
}

/** Send screen mode to the player.
 *
 * This step finalizes the transfer of guest features to the player and will enable the emulation.
 */
BOOLE RetroPlatformSendScreenMode(HWND hWnd) {
  BOOLE bResult;
  struct RPScreenMode RetroPlatformScreenMode = { 0 };

  if (!bRetroPlatformInitialized)
    return FALSE;

  hRetroPlatformGuestWindow = hWnd;
  RetroPlatformDetermineScreenModeFromConfig(&RetroPlatformScreenMode, RetroPlatformConfig);

  bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_SCREENMODE, 0, 0, 
    &RetroPlatformScreenMode, sizeof RetroPlatformScreenMode, 
    &RetroPlatformGuestInfo, NULL);

  fellowAddLog("RetroPlatformSendScreenMode(): %s.\n",
    bResult ? "successful" : "failed");

  return bResult;
}

ULO RetroPlatformGetEscapeKey(void) {
  return lRetroPlatformEscapeKey;
}

ULO RetroPlatformGetEscapeKeyHoldTime(void) {
  return lRetroPlatformEscapeKeyHoldTime;
}

ULONGLONG RetroPlatformGetEscapeKeyHeldSince(void) {
  return lRetroPlatformEscapeKeyHeldSince;
}

ULONGLONG RetroPlatformGetEscapeKeySimulatedTargetTime(void) {
  return lRetroPlatformEscapeKeySimulatedTargetTime;
}

HWND RetroPlatformGetParentWindowHandle(void) {
  LRESULT lResult;
  BOOLE bResult;

  if (!bRetroPlatformInitialized)
    return NULL;

  bResult = RetroPlatformSendMessage(RP_IPC_TO_HOST_PARENT, 0, 0, NULL, 0, 
    &RetroPlatformGuestInfo, &lResult);
  
  if(bResult) {
    fellowAddLog("RetroPlatformGetParentWindowHandle(): parent window handle returned was %d.\n", 
      lResult);  
    return (HWND)lResult;
  }
  else
    return NULL;
}

void RetroPlatformStartup(void) {
  ULO lResult;

  RetroPlatformConfig = cfgManagerGetCurrentConfig(&cfg_manager);
  
  lResult = RPInitializeGuest(&RetroPlatformGuestInfo, hRetroPlatformWindowInstance, 
    szRetroPlatformHostID, RetroPlatformHostMessageFunction, 0);

  if (SUCCEEDED (lResult)) {
    bRetroPlatformInitialized = TRUE;

    RetroPlatformGetHostVersion(&lRetroPlatformMainVersion, 
      &lRetroPlatformRevision, &lRetroPlatformBuild);

    fellowAddLog("RetroPlatformStartup(%s) successful. Host version: %d.%d.%d\n", 
      szRetroPlatformHostID, lRetroPlatformMainVersion, lRetroPlatformRevision, lRetroPlatformBuild);

    RetroPlatformSendFeatures();
  } 
  else
    fellowAddLog("RetroPlatformStartup(%s) failed, error code %08x\n", 
      szRetroPlatformHostID, lResult);
}

/** Verifies that the prerequisites to start the emulation are available.
 *
 * Validates that the configuration contains a path to a Kickstart ROM, and that the file can
 * be opened successfully for reading.
 * @return TRUE, when Kickstart ROM can be opened successfully for reading; FALSE otherwise
 */
BOOLE RetroPlatformCheckEmulationNecessities(void) {
  if(strcmp(cfgGetKickImage(RetroPlatformConfig), "") != 0) {
    FILE *F = fopen(cfgGetKickImage(RetroPlatformConfig), "rb");
    if (F != NULL)
    {
      fclose(F);
      return TRUE;
    }
    return FALSE;
  }
  else 
    return FALSE;
}

/** The main control function when operating in RetroPlatform headless mode.
 * 
 * This function performs the start of the emulator session. On a reset event,
 * winDrvEmulationStart will exit without bRetroPlatformEmulatorQuit being set.
 */
void RetroPlatformEnter(void) {
  if (RetroPlatformCheckEmulationNecessities() == TRUE) {
    cfgManagerSetCurrentConfig(&cfg_manager, RetroPlatformConfig);
    // check for manual or needed reset
    fellowPreStartReset(fellowGetPreStartReset() | cfgManagerConfigurationActivate(&cfg_manager));

    RetroPlatformSendEnabledFloppyDrives();
    RetroPlatformSendEnabledHardDrives();
    RetroPlatformSendGameports(RETRO_PLATFORM_NUM_GAMEPORTS);
    RetroPlatformSendInputDevices();

    while(!bRetroPlatformEmulatorQuit) {
      RetroPlatformSetEmulationState(TRUE);
      winDrvEmulationStart();
      RetroPlatformSetEmulationState(FALSE);
    }
  }
  else
    MessageBox(NULL, "Specified KickImage does not exist", "Configuration Error", 0);
}

void RetroPlatformShutdown(void) {
  if(!bRetroPlatformInitialized)
    return;

  RetroPlatformSendScreenMode(NULL);
  RetroPlatformPostMessage(RP_IPC_TO_HOST_CLOSED, 0, 0, &RetroPlatformGuestInfo);
  RPUninitializeGuest(&RetroPlatformGuestInfo);
  bRetroPlatformInitialized = FALSE;
}

void RetroPlatformEmulationStart(void) {
  RetroPlatformSendScreenMode(gfx_drv_hwnd);
  RetroPlatformSendMouseCapture(FALSE);
}

void RetroPlatformEmulationStop(void) {
}

#endif