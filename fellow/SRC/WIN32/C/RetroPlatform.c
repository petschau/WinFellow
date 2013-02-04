/* @(#) $Id: RetroPlatform.c,v 1.52 2013-02-04 18:04:15 carfesh Exp $ */
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
 *  This module contains RetroPlatform specific functionality to register as
 *  RetroPlatform guest and interact with the host (player).
 *  It imitates the full Windows GUI module, implementing the same functionality, 
 *  but supported via the RetroPlatform player as main GUI.
 *  WinFellow's own GUI is not shown, the emulator operates in a headless mode.
 *  The configuration is received as a command line parameter, all control events 
 *  (start, shutdown, reset, ...) are sent via IPC.
 * 
 *  *Important Note:* The Cloanto modules make heavy use of wide character strings.
 *  As WinFellow uses normal strings, conversion is usually required (using, for example,
 *  wsctombs and mbstowcs).
 * 
 *  @todo free allocated elements, cfgmanager, ... in RetroPlatform module
 *  @todo make resolution configurable via config file dynamically instead of from the fixed set available from the GUI
 *  @todo auto-resizing of window based on scaling, clipping and resolution inside emulation; lores, hires 1x, 2x
 *  @todo fullscreen resolution support for RetroPlatform
 *  @todo drive sounds not audible, these are produced by the emulator, not the player - how to determine if setting is active or not?
 *  @todo pick up implementation of input devices (keyboard joysticks) based on VICE implementation once it is available
 *  @bug  reset functionality not fully implemented, test soft- & hard reset
 *  @bug  the sound stops while the window does not have focus, while the rest of the emulation continues
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
#include "kbddrv.h"

#define RETRO_PLATFORM_NUM_GAMEPORTS 2 // gameport 1 & 2
#define RETRO_PLATFORM_KEYSET_COUNT  6 // north, east, south, west, fire, autofire

static const char *RetroPlatformCustomLayoutKeys[RETRO_PLATFORM_KEYSET_COUNT] = { "up", "right", "down", "left", "fire", "fire.autorepeat" };
extern BOOLE kbd_drv_joykey_enabled[2][2];	// For each port, the enabled joykeys

/*
#define RETRO_PLATFORM_JOYKEYLAYOUT_COUNT 4
static const STR *szRetroPlatformJoyKeyLayoutID[RETRO_PLATFORM_JOYKEYLAYOUT_COUNT] = { "KeyboardLayout1", "KeyboardLayout2", "KeyboardLayout3", "KeyboardCustom" };
*/

/// host ID that was passed over by the RetroPlatform player
STR szRetroPlatformHostID[CFG_FILENAME_LENGTH] = "";
/// flag to indicate that emulator operates in "headless" mode
BOOLE bRetroPlatformMode = FALSE;

ULO lRetroPlatformEscapeKey                     = 1;
ULO lRetroPlatformEscapeKeyHoldTime             = 600;
ULONGLONG lRetroPlatformEscapeKeyTargetHoldTime = 0;
ULO lRetroPlatformScreenMode                    = 0;

static BOOLE bRetroPlatformInitialized    = FALSE;
static BOOLE bRetroPlatformEmulationState = FALSE;
static BOOLE bRetroPlatformEmulatorQuit   = FALSE;

static RPGUESTINFO RetroPlatformGuestInfo = { 0 };

HINSTANCE hRetroPlatformWindowInstance = NULL;
HWND      hRetroPlatformGuestWindow = NULL;

static ULO lRetroPlatformMainVersion = -1, lRetroPlatformRevision = -1, lRetroPlatformBuild = -1;
static ULO lRetroPlatformRecursiveDevice = 0;

BOOLE bRetroPlatformMouseCaptureRequestedByHost = FALSE;

cfg *RetroPlatformConfig; ///< RetroPlatform copy of configuration

/** Determine the number of joysticks connected to the system.
 */
int RetroPlatformGetNumberOfConnectedJoysticks(void) {
  JOYINFOEX joyinfoex;
  int njoyId = 0;
  int njoyCount = 0;
  MMRESULT dwResult;   

  while ((dwResult = joyGetPosEx(njoyId++, &joyinfoex)) != JOYERR_PARMS)
    if (dwResult == JOYERR_NOERROR)
      njoyCount++;

  fellowAddLog("RetroPlatformGetNumberOfConnectedJoysticks: detected %d joystick(s).\n", 
    njoyCount);

  return njoyCount;
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
      /* else if(strcmp(szName, "GP_JOYKEY0") == 0) {
        fellowAddLog(" Attaching keyboard layout 1 to gameport..\n");
        gameportSetInput(lGameport, GP_JOYKEY0);
      } */
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
        fellowAddLog (" Unknown joystick input device name, ignoring..\n");
        return FALSE;
      }
      return TRUE;
    default:
      fellowAddLog(" Unsupported input device type detected.\n");
      return FALSE;
  }
}

/** Translate the screenmode configured in the configuration file and pass it along to the RetroPlatform Player.
 */
static void RetroPlatformDetermineScreenModeFromConfig(
  struct RPScreenMode *RetroPlatformScreenMode, cfg *RetroPlatformConfig) {
  DWORD dwScreenMode = RP_SCREENMODE_SCALE_1X;
	int iHeight = cfgGetScreenHeight(RetroPlatformConfig);
  int iWidth = cfgGetScreenWidth(RetroPlatformConfig);

  RetroPlatformScreenMode->hGuestWindow = hRetroPlatformGuestWindow;

  RetroPlatformScreenMode->lTargetHeight = iHeight;
  RetroPlatformScreenMode->lTargetWidth  = iWidth;

  RetroPlatformScreenMode->dwScreenMode = dwScreenMode;

  RetroPlatformScreenMode->lClipLeft = -1;
  RetroPlatformScreenMode->lClipTop = -1;
  RetroPlatformScreenMode->lClipWidth = -1;
  RetroPlatformScreenMode->lClipHeight = -1;
  RetroPlatformScreenMode->dwClipFlags = RP_CLIPFLAGS_NOCLIP;
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
static ULONGLONG RetroPlatformGetTime(void) {
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

/** Verify state of the emulation engine.
 *  @return TRUE, if emulation session if active, FALSE if not.
 */
BOOLE RetroPlatformGetEmulationState(void) {
  return bRetroPlatformEmulationState;
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
BOOLE RetroPlatformGetMode(void) {
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

/** Control status of the RetroPlatform floppy drive LEDs.
 *
 * Only sends status changes to the RetroPlatform host in the form of RP_IPC_TO_HOST_DEVICEACTIVITY messages.
 */
BOOLE RetroPlatformSendFloppyDriveLED(const ULO lFloppyDriveNo, const BOOLE bMotorActive) {
	BOOLE bWriteActivity = diskDMAen == 3 && !((floppy[lFloppyDriveNo].sel | !floppy[lFloppyDriveNo].enabled) & (1 << lFloppyDriveNo));

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

  fellowAddLog("RetroPlatformSendFloppyDriveContent(%d,%s): %s.\n",
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

void RetroPlatformSetEscapeKey(const char *szEscapeKey) {
  lRetroPlatformEscapeKey = atoi(szEscapeKey);
  lRetroPlatformEscapeKey = kbddrv_DIK_to_symbol[lRetroPlatformEscapeKey];
  fellowAddLog("RetroPlatformSetEscapeKey(): escape key configured to %d.\n", lRetroPlatformEscapeKey);
}

void RetroPlatformSetEscapeKeyHoldTime(const char *szEscapeHoldTime) {
  lRetroPlatformEscapeKeyHoldTime = atoi(szEscapeHoldTime);
  fellowAddLog("RetroPlatformSetEscapeKeyHoldTime(): escape hold time configured to %d.\n", lRetroPlatformEscapeKeyHoldTime);
}

void RetroPlatformSetEscapeKeyTargetHoldTime(const BOOLE bEscapeKeyHeld) {
  if(bEscapeKeyHeld) {
    if(lRetroPlatformEscapeKeyTargetHoldTime == 0)
      lRetroPlatformEscapeKeyTargetHoldTime = RetroPlatformGetTime() + lRetroPlatformEscapeKeyHoldTime;
  } 
  else
    lRetroPlatformEscapeKeyTargetHoldTime = 0;
}

void RetroPlatformSetEmulationState(const BOOLE bNewState) {
  if(bRetroPlatformEmulationState != bNewState) {
    bRetroPlatformEmulationState = bNewState;
    fellowAddLog("RetroPlatformSetEmulationState(): state set to %s.\n", bNewState ? "active" : "inactive");
    RetroPlatformSendPowerLEDIntensityPercent(bNewState ? 100 : 0);
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

void RetroPlatformSetScreenMode(const char *szScreenMode) {
  lRetroPlatformScreenMode = atol(szScreenMode);
  fellowAddLog("RetroPlatformSetScreenMode(): screen mode configured to %d.\n", lRetroPlatformScreenMode);
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

	switch (uMessage)
	{
	default:
		fellowAddLog("RetroPlatformHostMessageFunction: Unknown or unsupported command %x\n", uMessage);
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
    fellowRequestEmulationStop();
		return TRUE;
	case RP_IPC_TO_GUEST_TURBO:
		// 0 equals max speed, 4 is default	
    if (wParam & RP_TURBO_CPU)	
      cpuIntegrationSetSpeed((lParam & RP_TURBO_CPU) ? 0 : 4);
    if (wParam & RP_TURBO_FLOPPY)
      floppySetFastDMA(lParam & RP_TURBO_FLOPPY ? TRUE : FALSE);
		return TRUE;
	case RP_IPC_TO_GUEST_PAUSE:
    if(wParam != 0) { // pause emulation
      fellowAddLog("RetroPlatformHostMessageFunction: received pause event.\n");
      gfxDrvRunEventReset();
      RetroPlatformSetEmulationState(FALSE);
      return 1;
    }
    else { // resume emulation
      fellowAddLog("RetroPlatformHostMessageFunction: received resume event, requesting start.\n");
      gfxDrvRunEventSet();
      RetroPlatformSetEmulationState(TRUE);
      return 1;
    }
	case RP_IPC_TO_GUEST_VOLUME:
		/*currprefs.sound_volume = changed_prefs.sound_volume = 100 - wParam;
		set_volume (currprefs.sound_volume, 0);*/
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
				  if(ok)
					  // inputdevice_updateconfig (&currprefs);
				  break;
			  case RP_DEVICECATEGORY_CD:
				  ok = FALSE;
				  break;
			} 
			return ok;
		}
	case RP_IPC_TO_GUEST_SCREENMODE:
		{
			/* struct RPScreenMode *sm = (struct RPScreenMode*)pData;
			set_screenmode (sm, &changed_prefs);
			return (LRESULT)INVALID_HANDLE_VALUE;*/
		}
	case RP_IPC_TO_GUEST_EVENT:
		{
			/* TCHAR out[256];
			TCHAR *s = (WCHAR*)pData;
			int idx = -1;
			for (;;) {
				int ret;
				out[0] = 0;
				ret = cfgfile_modify (idx++, s, _tcslen (s), out, sizeof out / sizeof (TCHAR));
				if (ret >= 0)
					break;
			}*/
			return TRUE;
		}
	case RP_IPC_TO_GUEST_SCREENCAPTURE:
		{
			/*extern int screenshotf (const TCHAR *spath, int mode, int doprepare);
			extern int screenshotmode;
			int ok;
			int ossm = screenshotmode;
			TCHAR *s = (TCHAR*)pData;
			screenshotmode = 0;
			ok = screenshotf (s, 1, 1);
			screenshotmode = ossm;
			return ok ? TRUE : FALSE;*/
		}
	case RP_IPC_TO_GUEST_SAVESTATE:
		{
			TCHAR *s = (TCHAR*)pData;
			DWORD ret = FALSE;
			/* if (s == NULL) {
				savestate_initsave (NULL, 0, TRUE, true);
				return 1;
			}
			if (vpos == 0) {
				savestate_initsave (_T(""), 1, TRUE, true);
				save_state (s, _T("AmigaForever"));
				ret = 1;
			} else {
				//savestate_initsave (s, 1, TRUE);
				//ret = -1;
			}*/
			return ret;
		}
	case RP_IPC_TO_GUEST_LOADSTATE:
		{
			WCHAR *s = (WCHAR*)pData;
			DWORD ret = FALSE;
      /* DWORD attr = GetFileAttributes (s);
			if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
				savestate_state = STATE_DORESTORE;
				_tcscpy (savestate_fname, s);
				ret = -1;
			} */
			return ret;
		}
	case RP_IPC_TO_GUEST_DEVICEREADWRITE:
		{
			DWORD ret = FALSE;
      /* int device = LOBYTE(wParam);
			if (device == RP_DEVICECATEGORY_FLOPPY) {
				int num = HIBYTE(wParam);
				if (lParam == RP_DEVICE_READONLY || lParam == RP_DEVICE_READWRITE) {
					ret = disk_setwriteprotect (&currprefs, num, currprefs.floppyslots[num].df, lParam == RP_DEVICE_READONLY);
				}
			} */
			return ret ? (LPARAM)1 : 0;
		}
	case RP_IPC_TO_GUEST_FLUSH:
		return 1;
	case RP_IPC_TO_GUEST_QUERYSCREENMODE:
		{
      /*
			screenmode_request = true;
      */
			return 1;
		}
	case RP_IPC_TO_GUEST_GUESTAPIVERSION:
		{
			return MAKELONG(3, 3);
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

	dFeatureFlags = RP_FEATURE_POWERLED | RP_FEATURE_SCREEN1X;
  // dFeatureFlags = RP_FEATURE_POWERLED | RP_FEATURE_SCREEN1X | RP_FEATURE_FULLSCREEN;
  dFeatureFlags |= RP_FEATURE_PAUSE | RP_FEATURE_TURBO_CPU| RP_FEATURE_TURBO_FLOPPY;
  // dFeatureFlags |= RP_FEATURE_PAUSE | RP_FEATURE_TURBO_CPU | RP_FEATURE_TURBO_FLOPPY | RP_FEATURE_VOLUME | RP_FEATURE_SCREENCAPTURE;
	dFeatureFlags |= RP_FEATURE_SCANLINES;
  // dFeatureFlags |= RP_FEATURE_STATE | RP_FEATURE_SCANLINES | RP_FEATURE_DEVICEREADWRITE;
	// dFeatureFlags |= RP_FEATURE_SCALING_SUBPIXEL | RP_FEATURE_SCALING_STRETCH;
	dFeatureFlags |= RP_FEATURE_INPUTDEVICE_MOUSE;
	dFeatureFlags |= RP_FEATURE_INPUTDEVICE_JOYSTICK;
	// dFeatureFlags |= RP_FEATURE_INPUTDEVICE_GAMEPAD;
	// dFeatureFlags |= RP_FEATURE_INPUTDEVICE_JOYPAD;
	// dFeatureFlags |= RP_FEATURE_INPUTDEVICE_ANALOGSTICK;
	// dFeatureFlags |= RP_FEATURE_INPUTDEVICE_LIGHTPEN;

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

  // begin with the basics - the Windows mouse
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
  int num_joy_attached;

  num_joy_attached = RetroPlatformGetNumberOfConnectedJoysticks();

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
  if(num_joy_attached > 0) 
    if(!RetroPlatformSendInputDevice(RP_HOSTINPUT_JOYSTICK, 
      RP_FEATURE_INPUTDEVICE_JOYSTICK | RP_FEATURE_INPUTDEVICE_GAMEPAD,
      0,
      L"GP_ANALOG0",
      L"Analog Joystick 1")) bResult = FALSE;

  if(num_joy_attached > 1)
    if(!RetroPlatformSendInputDevice(RP_HOSTINPUT_JOYSTICK, 
      RP_FEATURE_INPUTDEVICE_JOYSTICK | RP_FEATURE_INPUTDEVICE_GAMEPAD,
      0,
      L"GP_ANALOG1",
      L"Analog Joystick 2")) bResult = FALSE;

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
  RetroPlatformSendMouseCapture(FALSE);
}

void RetroPlatformEmulationStop(void) {
}

void RetroPlatformEndOfFrame(void) {
  if(lRetroPlatformEscapeKeyTargetHoldTime != 0) {
    ULONGLONG t;

    t = RetroPlatformGetTime();
    if(t >= lRetroPlatformEscapeKeyTargetHoldTime) {
      fellowAddLog("RetroPlatform: Escape key held longer than hold time, releasing devices...\n");
      lRetroPlatformEscapeKeyTargetHoldTime = 0;

      RetroPlatformPostMessage(RP_IPC_TO_HOST_ESCAPED, 0, 0, &RetroPlatformGuestInfo);
    }
  }
}

#endif