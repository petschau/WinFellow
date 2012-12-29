/* @(#) $Id: RetroPlatform.c,v 1.16 2012-12-29 09:50:26 carfesh Exp $ */
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
 */

#include "defs.h"

#include "RetroPlatform.h"
#include "RetroPlatformGuestIPC.h"
#include "RetroPlatformIPC.h"

#include "wgui.h"
#include "config.h"
#include "fellow.h"
#include "windrv.h"
#include "floppy.h"

// #define RETRO_PLATFORM_SUPPORT_PAUSE

STR szRetroPlatformHostID[CFG_FILENAME_LENGTH] = ""; ///< host ID that was passed over by the RetroPlatform player
BOOLE bRetroPlatformMode = FALSE;                    ///< flag to indicate that emulator operates in "headless" mode

LON iRetroPlatformEscapeKey = 0x01;
LON iRetroPlatformEscapeHoldTime = 600;
// LON iRetroPlatformInputMode = 0;
// LON iRetroPlatformLogging = 1;
LON iRetroPlatformScreenMode = 0;

static BOOLE bRetroPlatformInitialized;
static BOOLE bRetroPlatformEmulationState = FALSE;
static RPGUESTINFO RetroPlatformGuestInfo;
HINSTANCE hRetroPlatformWindowInstance = NULL;
HWND      hRetroPlatformGuestWindow = NULL;
static ULO lRetroPlatformMainVersion = -1, lRetroPlatformRevision = -1, lRetroPlatformBuild = -1;
static ULO lRetroPlatformRecursiveDevice;

#define RETROPLATFORMNUMEVENTTYPES 2
enum RetroPlatformEventTypes {
  EmulationClose  = 0, 
  EmulationResume = 1
};

HANDLE hRetroPlatformEventEmulationClose, hRetroPlatformEventEmulationResume;

cfg *RetroPlatformConfig; ///< RetroPlatform copy of configuration

/** Verify state of the emulation engine.
 *  @return TRUE, if emulation session if active, FALSE if not.
 */
BOOLE RetroPlatformGetEmulationState(void) {
  return bRetroPlatformEmulationState;
}

static const STR *RetroPlatformGetMessageText(ULO iMsg) {
	switch(iMsg)
	{
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

BOOLE RetroPlatformGetMode(void) {
  return bRetroPlatformMode;
}

static BOOLE RetroPlatformPostMessage(ULO iMessage, WPARAM wParam, LPARAM lParam, const RPGUESTINFO *pGuestInfo) {
	BOOLE bResult;

	bResult = RPPostMessage(iMessage, wParam, lParam, pGuestInfo);

  if(bResult)
    fellowAddLog("RetroPlatform posted message ([%s], %08x, %08x, %08x)\n",
      RetroPlatformGetMessageText(iMessage), iMessage - WM_APP, wParam, lParam);
  else
		fellowAddLog("RetroPlatform could not post message, error: %d\n", GetLastError());

	return bResult;
}

void RetroPlatformSetEscapeKey(const char *szEscapeKey) {
  iRetroPlatformEscapeKey = atoi(szEscapeKey);
  fellowAddLog("RetroPlatform: escape key configured to %d.\n", iRetroPlatformEscapeKey);
}

void RetroPlatformSetEmulationState(const BOOLE bNewState) {
  if(bRetroPlatformEmulationState != bNewState) {
    bRetroPlatformEmulationState = bNewState;
    fellowAddLog("RetroPlatformSetEmulationState(): state set to %s.\n", bNewState ? "active" : "inactive");
    RetroPlatformSendPowerLEDIntensityPercent(bNewState ? 0x100 : 0);
  }
}

void RetroPlatformSetEscapeHoldTime(const char *szEscapeHoldTime) {
  iRetroPlatformEscapeHoldTime = atoi(szEscapeHoldTime);
  fellowAddLog("RetroPlatform: escape hold time configured to %d.\n", iRetroPlatformEscapeHoldTime);
}

void RetroPlatformSetHostID(const char *szHostID) {
  strncpy(szRetroPlatformHostID, szHostID, CFG_FILENAME_LENGTH);
  fellowAddLog("RetroPlatform: host ID configured to %s.\n", szRetroPlatformHostID);
}

void RetroPlatformSetMode(const BOOLE bRPMode) {
  bRetroPlatformMode = bRPMode;
  fellowAddLog("RetroPlatform: entering RetroPlatform (headless) mode.\n");
}

void RetroPlatformSetScreenMode(const char *szScreenMode) {
  iRetroPlatformScreenMode = atoi(szScreenMode);
  fellowAddLog("RetroPlatform: screen mode configured to %d.\n", iRetroPlatformScreenMode);
}

void RetroPlatformSetWindowInstance(HINSTANCE hInstance) {
  fellowAddLog("RetroPlatform: set window instance to %d.\n", hInstance);
  hRetroPlatformWindowInstance = hInstance;
}

static BOOLE RetroPlatformSendMessage(ULO iMessage, WPARAM wParam, LPARAM lParam,
	LPCVOID pData, DWORD dwDataSize, const RPGUESTINFO *pGuestInfo, LRESULT *plResult)
{
	BOOLE bResult;

	bResult = RPSendMessage(iMessage, wParam, lParam, pData, dwDataSize, pGuestInfo, plResult);
  
  if(bResult)
    fellowAddLog("RetroPlatform sent message ([%s], %08x, %08x, %08x, %d)\n",
      RetroPlatformGetMessageText(iMessage), iMessage - WM_APP, wParam, lParam, pData);
  else
		fellowAddLog("RetroPlatform could not send message, error: %d\n", GetLastError());
	
  return bResult;
}

/** Determine the RetroPlatform host version.
 * 
 * @param[out] lpMainVersion main version number
 * @param[out] lpRevisionrevision number
 * @param[out] lpBuild build number
 * @return TRUE is successful, FALSE otherwise.
 */
static BOOLE RetroPlatformGetHostVersion(ULO *lpMainVersion, ULO *lpRevision, ULO *lpBuild) {
	ULO lResult = 0;
	if (!RetroPlatformSendMessage(RP_IPC_TO_HOST_HOSTVERSION, 0, 0, NULL, 0, &RetroPlatformGuestInfo, &lResult))
		return FALSE;
	*lpMainVersion = RP_HOSTVERSION_MAJOR(lResult);
	*lpRevision    = RP_HOSTVERSION_MINOR(lResult);
	*lpBuild       = RP_HOSTVERSION_BUILD(lResult);
	return TRUE;
}

static LRESULT CALLBACK RetroPlatformHostMessageFunction2(UINT uMessage, WPARAM wParam, LPARAM lParam,
	LPCVOID pData, DWORD dwDataSize, LPARAM lMsgFunctionParam)
{
	fellowAddLog("RetroPlatformHostMessageFunction2(%s [%d], %08x, %08x, %08x, %d, %08x)\n",
	  RetroPlatformGetMessageText(uMessage), uMessage - WM_APP, wParam, lParam, pData, dwDataSize, lMsgFunctionParam);
	if (uMessage == RP_IPC_TO_GUEST_DEVICECONTENT) {
		struct RPDeviceContent *dc = (struct RPDeviceContent*)pData;
		fellowAddLog(" Cat=%d Num=%d Flags=%08x '%s'\n",
			dc->btDeviceCategory, dc->btDeviceNumber, dc->dwFlags, dc->szContent);
  }

	switch (uMessage)
	{
	default:
		fellowAddLog("RetroPlatformHostMessageFunction2: Unknown or unsupported command %x\n", uMessage);
		break;
	case RP_IPC_TO_GUEST_PING:
		return TRUE;
	case RP_IPC_TO_GUEST_CLOSE:
    fellowAddLog("RetroPlatformHostMessageFunction2: received close event.\n");
    if(RetroPlatformGetEmulationState()) {
      fellowAddLog("emulation engine active, stopping...\n");
      fellowRequestEmulationStop();
    }
    fellowAddLog("requesting close of emulator.\n");
    SetEvent(hRetroPlatformEventEmulationClose);
		return TRUE;
	case RP_IPC_TO_GUEST_RESET:
    if(wParam == RP_RESET_SOFT)
      fellowSoftReset();
    else
      fellowHardReset();
		return TRUE;
	case RP_IPC_TO_GUEST_TURBO:
		/*{
			if (wParam & RP_TURBO_CPU)
				warpmode ((lParam & RP_TURBO_CPU) ? 1 : 0);
			if (wParam & RP_TURBO_FLOPPY)
				changed_prefs.floppy_speed = (lParam & RP_TURBO_FLOPPY) ? 0 : 100;
			config_changed = 1;
		}*/
		return TRUE;
	case RP_IPC_TO_GUEST_PAUSE:
#ifdef RETRO_PLATFORM_SUPPORT_PAUSE
    if(wParam != 0) { // pause emulation
      fellowAddLog("RetroPlatformHostMessageFunction2: received pause event.\n");
      if(RetroPlatformGetEmulationState()) {
        fellowAddLog("emulation engine active, stopping...\n");
        fellowRequestEmulationStop();
      }
      return 1;
    }
    else { // resume emulation
      fellowAddLog("RetroPlatformHostMessageFunction2: received resume event, requesting start.\n");
      SetEvent(hRetroPlatformEventEmulationResume);
      return 1;
    }
#else
    return 1;
#endif
	case RP_IPC_TO_GUEST_VOLUME:
		/*currprefs.sound_volume = changed_prefs.sound_volume = 100 - wParam;
		set_volume (currprefs.sound_volume, 0);*/
		return TRUE;
	case RP_IPC_TO_GUEST_ESCAPEKEY:
		/*rp_rpescapekey = wParam;
		rp_rpescapeholdtime = lParam; */
		return TRUE;
	case RP_IPC_TO_GUEST_MOUSECAPTURE:
		/*{
			if (wParam & RP_MOUSECAPTURE_CAPTURED)
				setmouseactive (1);
			else
				setmouseactive (0);
		}*/
		return TRUE;
	case RP_IPC_TO_GUEST_DEVICECONTENT:
		{
			struct RPDeviceContent *dc = (struct RPDeviceContent*)pData;
			STR *n = (STR *)dc->szContent;
			int num = dc->btDeviceNumber;
			int ok = FALSE;
			switch (dc->btDeviceCategory)
			{
			case RP_DEVICECATEGORY_FLOPPY:
				if (n == NULL || n[0] == 0)
					floppyImageRemove(num);
				else
          floppySetDiskImage(num, n);
				ok = TRUE;
				break;
     /*
			case RP_DEVICECATEGORY_INPUTPORT:
				ok = port_insert (num, dc->dwInputDevice, dc->dwFlags, n);
				if (ok)
					inputdevice_updateconfig (&currprefs);
				break;
			case RP_DEVICECATEGORY_CD:
				ok = cd_insert (num, n);
				break;
        */
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

static LRESULT CALLBACK RetroPlatformHostMessageFunction(UINT uMessage, WPARAM wParam, LPARAM lParam,
	LPCVOID pData, DWORD dwDataSize, LPARAM lMsgFunctionParam)
{
	LRESULT lResult;
	lRetroPlatformRecursiveDevice++;
	lResult = RetroPlatformHostMessageFunction2(uMessage, wParam, lParam, pData, dwDataSize, lMsgFunctionParam);
	lRetroPlatformRecursiveDevice--;
	return lResult;
}

void RetroPlatformSendClose(void) {
	RetroPlatformSendMessage(RP_IPC_TO_HOST_CLOSE, 0, 0, NULL, 0, &RetroPlatformGuestInfo, NULL);
}

void RetroPlatformSendFeatures(void)
{
	DWORD dFeatureFlags;
  LRESULT lResult;

	dFeatureFlags = RP_FEATURE_POWERLED | RP_FEATURE_SCREEN1X;
#ifdef RETRO_PLATFORM_SUPPORT_PAUSE
  dFeatureFlags |= RP_FEATURE_PAUSE;
#endif
  // dFeatureFlags = RP_FEATURE_POWERLED | RP_FEATURE_SCREEN1X | RP_FEATURE_FULLSCREEN;

	/*dFeatureFlags |= RP_FEATURE_PAUSE | RP_FEATURE_TURBO_CPU | RP_FEATURE_TURBO_FLOPPY | RP_FEATURE_VOLUME | RP_FEATURE_SCREENCAPTURE;
	dFeatureFlags |= RP_FEATURE_STATE | RP_FEATURE_SCANLINES | RP_FEATURE_DEVICEREADWRITE;
	dFeatureFlags |= RP_FEATURE_SCALING_SUBPIXEL | RP_FEATURE_SCALING_STRETCH;
	dFeatureFlags |= RP_FEATURE_INPUTDEVICE_MOUSE;
	dFeatureFlags |= RP_FEATURE_INPUTDEVICE_JOYSTICK;
	dFeatureFlags |= RP_FEATURE_INPUTDEVICE_GAMEPAD;
	dFeatureFlags |= RP_FEATURE_INPUTDEVICE_JOYPAD;
	dFeatureFlags |= RP_FEATURE_INPUTDEVICE_ANALOGSTICK;
	dFeatureFlags |= RP_FEATURE_INPUTDEVICE_LIGHTPEN;*/

	if(RetroPlatformSendMessage(RP_IPC_TO_HOST_FEATURES, dFeatureFlags, 0, NULL, 0, &RetroPlatformGuestInfo, &lResult))
    fellowAddLog("RetroPlatformSendFeatures successful, result was %d.\n", lResult);
  else
    fellowAddLog("RetroPlatformSendFeatures failed, result was %d.\n", lResult);
}

static void RetroPlatformDetermineScreenModeFromConfig(struct RPScreenMode *RetroPlatformScreenMode, cfg *RetroPlatformConfig)
{
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

void RetroPlatformSendActivate(const BOOLE bActive, const LPARAM lParam)
{
	RetroPlatformSendMessage(bActive ? RP_IPC_TO_HOST_ACTIVATED : RP_IPC_TO_HOST_DEACTIVATED, 0, lParam, NULL, 0, &RetroPlatformGuestInfo, NULL);
}

/** Control status of power LED in RetroPlatform player.
 *
 * Examines the current on/off state of the emulator session and sends it to the RetroPlatform player.
 */
void RetroPlatformSendPowerLEDIntensityPercent(const WPARAM wIntensityPercent) {
  if(wIntensityPercent <= 0x100 && wIntensityPercent >= 0)
    RetroPlatformSendMessage(RP_IPC_TO_HOST_POWERLED, wIntensityPercent, 0, NULL, 0, &RetroPlatformGuestInfo, NULL);
}

void RetroPlatformSendScreenMode(HWND hWnd)
{
	struct RPScreenMode RetroPlatformScreenMode = { 0 };

	if (!bRetroPlatformInitialized)
		return;
	hRetroPlatformGuestWindow = hWnd;
	RetroPlatformDetermineScreenModeFromConfig(&RetroPlatformScreenMode, RetroPlatformConfig);
	RetroPlatformSendMessage(RP_IPC_TO_HOST_SCREENMODE, 0, 0, &RetroPlatformScreenMode, sizeof RetroPlatformScreenMode, &RetroPlatformGuestInfo, NULL); 
}

HWND RetroPlatformGetParentWindowHandle(void)
{
	LRESULT lResult;
	if (!bRetroPlatformInitialized)
		return NULL;
	RetroPlatformSendMessage(RP_IPC_TO_HOST_PARENT, 0, 0, NULL, 0, &RetroPlatformGuestInfo, &lResult);
  fellowAddLog("RetroPlatformGetParentWindowHandle: parent window handle returned was %d.\n", lResult);
	return (HWND)lResult;
}

void RetroPlatformStartup(void)
{
  ULO lResult;

  fellowAddLog("RetroPlatform startup.\n");
  RetroPlatformConfig = cfgManagerGetCurrentConfig(&cfg_manager);
  
	lResult = RPInitializeGuest(&RetroPlatformGuestInfo, hRetroPlatformWindowInstance, szRetroPlatformHostID, RetroPlatformHostMessageFunction, 0);
  if (SUCCEEDED (lResult)) {
	  bRetroPlatformInitialized = TRUE;

    RetroPlatformGetHostVersion(&lRetroPlatformMainVersion, &lRetroPlatformRevision, &lRetroPlatformBuild);
    fellowAddLog("RetroPlatformStartup (host ID %s) initialization succeeded. Host version: %d.%d.%d\n", szRetroPlatformHostID, 
      lRetroPlatformMainVersion, lRetroPlatformRevision, lRetroPlatformBuild);

    RetroPlatformSendFeatures();

    hRetroPlatformEventEmulationClose  = CreateEvent(NULL, FALSE, FALSE, "Close Emulation");
    hRetroPlatformEventEmulationResume = CreateEvent(NULL, FALSE, FALSE, "Resume Emulation");
  } 
  else {
    fellowAddLog("RetroPlatformStartup (host ID %s) failed to initialize, error code %08x\n", szRetroPlatformHostID, lResult);
  }
}

/** Verifies that the prerequisites to start the emulation are available.
 *
 * Validates that the configuration contains a path to a Kickstart ROM, and that the file can
 * be opened successfully for reading.
 * @return TRUE, when Kickstart ROM can be opened successfully for reading; FALSE otherwise
 */
BOOLE RetroPlatformCheckEmulationNecessities(void) 
{
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

ULO RetroPlatformInitializeMultiEventArray(HANDLE *hMultiEvents, 
  enum RetroPlatformEventTypes *eEventMapping) {
  ULO lEventCount = 0;

  hMultiEvents[lEventCount] = hRetroPlatformEventEmulationClose;
  eEventMapping[lEventCount++] = EmulationClose;
  hMultiEvents[lEventCount] = hRetroPlatformEventEmulationResume;
  eEventMapping[lEventCount++] = EmulationResume;

  return lEventCount;
}

/** The main control function when operating in RetroPlatform headless mode.
 * 
 * This function performs the initial initialization and start of the emulator session.
 * Once the first session has ended, it will wait in a loop for emulator control events
 * received from the RetroPlatform player via the RetroPlatformHostMessageFunction
 * Those events can be to resume emulation, and to quit the emulator.
 * Pausing of the emulator is handled directly within the host message function.
 * 
 * In contrast to the main Windows GUI, this method keeps the emulator session,
 * including it's window handle intact, which is important as the handle is known
 * to the RetroPlatform player and should not change. 
 * The main GUI destroys the session and creates a new one whenever the GUI mode is entered.
 * 
 * Once the main loop is exited (quit event), the emulator session will be destroyed.
 */
void RetroPlatformEnter(void) {
  BOOLE bQuitEmulator = FALSE;
  BOOLE bFirstStart = TRUE;
  DWORD dwResult;
  ULO   lEventCount;

  HANDLE hMultiEvents[RETROPLATFORMNUMEVENTTYPES] = {0};
  enum RetroPlatformEventTypes eEventMapping[RETROPLATFORMNUMEVENTTYPES];

  do {
    if(bFirstStart) {
      lEventCount = RetroPlatformInitializeMultiEventArray(hMultiEvents, eEventMapping);

      if (RetroPlatformCheckEmulationNecessities() == TRUE) {
	      cfgManagerSetCurrentConfig(&cfg_manager, RetroPlatformConfig);
	      // check for manual or needed reset
	      fellowPreStartReset(fellowGetPreStartReset() | cfgManagerConfigurationActivate(&cfg_manager));

        if (!fellowEmulationStart()) {
          bQuitEmulator = TRUE;
          MessageBox(NULL, "Emulation session failed to start up", "", 0);
        } 
	    }
      else {
	      MessageBox(NULL, "Specified KickImage does not exist", "Configuration Error", 0);
	      bQuitEmulator = TRUE;
      }
      bFirstStart = FALSE;

      if(!bQuitEmulator) {
        RetroPlatformSetEmulationState(TRUE),
        winDrvEmulationStartRP();
        RetroPlatformSetEmulationState(FALSE);
      }
    }

    if (!bQuitEmulator) {
      fellowAddLog("RetroPlatformEnter(): entering emulator control event loop, awaiting events...\n");

      dwResult = WaitForMultipleObjects(lEventCount, hMultiEvents, FALSE, INFINITE);

      fellowAddLog("RetroPlatformEnter(): received an emulator control event, dwResult=%d.\n", dwResult);

      if ((dwResult >= WAIT_OBJECT_0) && (dwResult <= (WAIT_OBJECT_0 + lEventCount))) {
        switch (eEventMapping[dwResult - WAIT_OBJECT_0]) {
          case EmulationResume:
            fellowAddLog("RetroPlatformEnter(): received emulation resume event.\n");
            if(!RetroPlatformGetEmulationState()) {
              RetroPlatformSetEmulationState(TRUE);
              winDrvEmulationStartRP();
              RetroPlatformSetEmulationState(FALSE);
            }
            break;
          case EmulationClose:
            fellowAddLog("RetroPlatformEnter(): received emulation close event.\n");
            bQuitEmulator = TRUE;
            break;
          default:
            fellowAddLog("RetroPlatformEnter(): received unknown event.\n");
            bQuitEmulator = TRUE;
        }
      }
    }
  } while(!bQuitEmulator);

  fellowEmulationStop();
}

void RetroPlatformShutdown(void) {
  if(!bRetroPlatformInitialized)
    return;

  if(hRetroPlatformEventEmulationResume != NULL)
    CloseHandle(hRetroPlatformEventEmulationResume);
  if(hRetroPlatformEventEmulationClose != NULL)
    CloseHandle(hRetroPlatformEventEmulationClose);

  RetroPlatformSendScreenMode(NULL);
  RetroPlatformPostMessage(RP_IPC_TO_HOST_CLOSED, 0, 0, &RetroPlatformGuestInfo);
  RPUninitializeGuest(&RetroPlatformGuestInfo);
  bRetroPlatformInitialized = FALSE;
}

void RetroPlatformEmulationStart(void) {
}

void RetroPlatformEmulationStop(void) {
}

#endif