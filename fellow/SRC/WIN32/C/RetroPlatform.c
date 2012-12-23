/* @(#) $Id: RetroPlatform.c,v 1.7 2012-12-23 14:42:26 carfesh Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* This file contains RetroPlatform specific functionality to register as  */
/* guest and interact with the host.                                       */
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

#include "defs.h"

#include "RetroPlatform.h"
#include "RetroPlatformGuestIPC.h"
#include "RetroPlatformIPC.h"

#include "wgui.h"
#include "config.h"
#include "fellow.h"
#include "windrv.h"

STR szRetroPlatformHostID[CFG_FILENAME_LENGTH] = "";
BOOLE bRetroPlatformMode = FALSE;

LON iRetroPlatformEscapeKey = 0x01;
LON iRetroPlatformEscapeHoldTime = 600;
// LON iRetroPlatformInputMode = 0;
// LON iRetroPlatformLogging = 1;
LON iRetroPlatformScreenMode = 0;

static BOOLE bRetroPlatformInitialized;
static RPGUESTINFO RetroPlatformGuestInfo;
HINSTANCE hRetroPlatformWindowInstance = NULL;
static ULO lRetroPlatformMainVersion = -1, lRetroPlatformRevision = -1, lRetroPlatformBuild = -1;
static ULO lRetroPlatformRecursiveDevice;

RetroPlatformActions RetroPlatformAction;

cfg *RetroPlatformConfig; /* RetroPlatform copy of configuration */

BOOLE RetroPlatformGetMode(void) {
  return bRetroPlatformMode;
}

void RetroPlatformSetAction(const RetroPlatformActions lAction)
{
  RetroPlatformAction = lAction;
  fellowAddLog("RetroPlatformSetAction: %s\n", RetroPlatformGetActionName(lAction));
}

void RetroPlatformSetEscapeKey(const char *szEscapeKey) {
  iRetroPlatformEscapeKey = atoi(szEscapeKey);
  fellowAddLog("RetroPlatform: escape key configured to %d.\n", iRetroPlatformEscapeKey);
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

void RetroPlatformSetWindowInstance(HINSTANCE hInstance)
{
  fellowAddLog("RetroPlatform: set window instance to %d.\n", hInstance);
  hRetroPlatformWindowInstance = hInstance;
}

BOOLE RetroPlatformEnter(void) {
  BOOLE quit_emulator = FALSE;

  do {
    BOOLE end_loop = FALSE;
      
    // RetroPlatformAction = RETRO_PLATFORM_NO_ACTION;

    while (!end_loop) {
	    switch (RetroPlatformAction) {
	      case RETRO_PLATFORM_START_EMULATION:
	        if (wguiCheckEmulationNecessities() == TRUE) {
	          end_loop = TRUE;
	          cfgManagerSetCurrentConfig(&cfg_manager, RetroPlatformConfig);
	          // check for manual or needed reset
	          fellowPreStartReset(fellowGetPreStartReset() | cfgManagerConfigurationActivate(&cfg_manager));
	          break;
	        }
	        MessageBox(NULL, "Specified KickImage does not exist", "Configuration Error", 0);
	        RetroPlatformAction = RETRO_PLATFORM_NO_ACTION;
          // end_loop = TRUE;
          // quit_emulator = TRUE;
	        break;
	     case RETRO_PLATFORM_QUIT_EMULATOR:
	        end_loop = TRUE;
	        quit_emulator = TRUE;
	        break;
	      case RETRO_PLATFORM_OPEN_CONFIGURATION:
	        break;
	      case RETRO_PLATFORM_SAVE_CONFIGURATION:
	        break;
	      case RETRO_PLATFORM_SAVE_CONFIGURATION_AS:
	        RetroPlatformAction = RETRO_PLATFORM_NO_ACTION;
	        break;
	      case RETRO_PLATFORM_LOAD_STATE:
	        RetroPlatformAction = RETRO_PLATFORM_NO_ACTION;
	        break;
	      case RETRO_PLATFORM_SAVE_STATE:
	        RetroPlatformAction = RETRO_PLATFORM_NO_ACTION;
	        break;
	      default:
	        break;
	    }
    }
    if (!quit_emulator) 
      winDrvEmulationStart(); 
  } while (!quit_emulator);
  return quit_emulator;
}

const STR *RetroPlatformGetActionName(RetroPlatformActions lAction)
{
  switch(lAction)
  {
    case RETRO_PLATFORM_START_EMULATION:  return TEXT("RETRO_PLATFORM_START_EMULATION");
    default:                              return TEXT("UNIDENTIFIED_RETRO_PLATFORM_ACTION");
  }
}

static const STR *RetroPlatformGetMessageText(ULO iMsg)
{
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

static BOOLE RetroPlatformSendMessage(ULO iMessage, WPARAM wParam, LPARAM lParam,
	LPCVOID pData, DWORD dwDataSize, const RPGUESTINFO *pGuestInfo, LRESULT *plResult)
{
	BOOLE bResult = FALSE;

	bResult = RPSendMessage(iMessage, wParam, lParam, pData, dwDataSize, pGuestInfo, plResult);

  fellowAddLog (TEXT("RetroPlatform sent [%s], %08x, %08x, %08x, %d)\n"),
    RetroPlatformGetMessageText(iMessage), iMessage - WM_APP, wParam, lParam, pData, dwDataSize);
		if (bResult == FALSE)
			fellowAddLog("ERROR %d\n", GetLastError());
	return bResult;
}

static int RetroPlatformGetHostVersion(ULO *lMainVersion, ULO *lRevision, ULO *lBuild)
{
	ULO lResult = 0;
	if (!RetroPlatformSendMessage(RP_IPC_TO_HOST_HOSTVERSION, 0, 0, NULL, 0, &RetroPlatformGuestInfo, &lResult))
		return 0;
	*lMainVersion = RP_HOSTVERSION_MAJOR(lResult);
	*lRevision    = RP_HOSTVERSION_MINOR(lResult);
	*lBuild       = RP_HOSTVERSION_BUILD(lResult);
	return 1;
}

static LRESULT CALLBACK RetroPlatformHostMessageFunction(UINT uMessage, WPARAM wParam, LPARAM lParam,
	LPCVOID pData, DWORD dwDataSize, LPARAM lMsgFunctionParam)
{
	LRESULT lResult;
	lRetroPlatformRecursiveDevice++;
	// lResult = RPHostMsgFunction2(uMessage, wParam, lParam, pData, dwDataSize, lMsgFunctionParam);
	lRetroPlatformRecursiveDevice--;
	return lResult;
}

void RetroPlatformSendFeatures(void)
{
	DWORD dFeatureFlags;
  LRESULT lResult;

	dFeatureFlags = RP_FEATURE_SCREEN1X;
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

void RetroPlatformActivate(const BOOLE bActive, const LPARAM lParam)
{
	RetroPlatformSendMessage(bActive ? RP_IPC_TO_HOST_ACTIVATED : RP_IPC_TO_HOST_DEACTIVATED, 0, lParam, NULL, 0, &RetroPlatformGuestInfo, NULL);
}

void RetroPlatformStartup(void)
{
  ULO lResult;

  fellowAddLog("RetroPlatform startup.\n");
  RetroPlatformConfig = cfgManagerGetCurrentConfig(&cfg_manager);
  RetroPlatformSetAction(RETRO_PLATFORM_START_EMULATION);
  
	lResult = RPInitializeGuest(&RetroPlatformGuestInfo, hRetroPlatformWindowInstance, szRetroPlatformHostID, RetroPlatformHostMessageFunction, 0);
  if (SUCCEEDED (lResult)) {
	  bRetroPlatformInitialized = TRUE;

    RetroPlatformGetHostVersion(&lRetroPlatformMainVersion, &lRetroPlatformRevision, &lRetroPlatformBuild);
    fellowAddLog("RetroPlatformStartup (host ID %s) initialization succeeded. Host version: %d.%d.%d\n", szRetroPlatformHostID, 
      lRetroPlatformMainVersion, lRetroPlatformRevision, lRetroPlatformBuild);

    RetroPlatformSendFeatures();
  } else {
    fellowAddLog("RetroPlatformStartup (host ID %s) failed, error code %08x\n", szRetroPlatformHostID, lResult);
  }
}

void RetroPlatformShutdown(void)
{
}

void RetroPlatformEmulationStart(void)
{
}

void RetroPlatformEmulationStop(void)
{
}

#endif