/* @(#) $Id: RetroPlatform.c,v 1.1 2012-12-07 14:07:13 carfesh Exp $ */
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
#include "fellow.h"

STR szRetroPlatformHostID[CFG_FILENAME_LENGTH] = "";
BOOLE bRetroPlatformMode = FALSE;

LON iRetroPlatformEscapeKey = 0x01;
LON iRetroPlatformEscapeHoldTime = 600;
// LON iRetroPlatformInputMode = 0;
// LON iRetroPlatformLogging = 1;
LON iRetroPlatformScreenMode = 0;


BOOLE RetroPlatformGetMode(void) {
  return bRetroPlatformMode;
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

void RetroPlatformSetMode(BOOLE bRPMode) {
  bRetroPlatformMode = bRPMode;
  fellowAddLog("RetroPlatform: entering RetroPlatform (headless) mode.\n");
}

void RetroPlatformSetScreenMode(const char *szScreenMode) {
  iRetroPlatformScreenMode = atoi(szScreenMode);
  fellowAddLog("RetroPlatform: screen mode configured to %d.\n", iRetroPlatformScreenMode);
}

BOOLE RetroPlatformEnter(void) {

  /*
    BOOLE quit_emulator = FALSE;
    BOOLE debugger_start = FALSE;
    RECT dialogRect;

    do {
      MSG myMsg;
      BOOLE end_loop = FALSE;

      wgui_action = WGUI_NO_ACTION;

      if(!RetroplatformMode())
      {
        wgui_hDialog = CreateDialog(win_drv_hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, wguiDialogProc); 
        SetWindowPos(wgui_hDialog, HWND_TOP, iniGetMainWindowXPos(wgui_ini), iniGetMainWindowYPos(wgui_ini), 0, 0, SWP_NOSIZE | SWP_ASYNCWINDOWPOS);
        wguiStartupPost();
        wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);


        // install history into menu
        wguiInstallHistoryIntoMenu();
        ShowWindow(wgui_hDialog, win_drv_nCmdShow);
      }

      while (!end_loop) {
	if (GetMessage(&myMsg, wgui_hDialog, 0, 0))
	  if (!IsDialogMessage(wgui_hDialog, &myMsg))
	    DispatchMessage(&myMsg);
	switch (wgui_action) {
	case WGUI_START_EMULATION:
	  if (wguiCheckEmulationNecessities() == TRUE) {
	    end_loop = TRUE;
	    cfgManagerSetCurrentConfig(&cfg_manager, wgui_cfg);
	    // check for manual or needed reset
	    fellowPreStartReset(fellowGetPreStartReset() | cfgManagerConfigurationActivate(&cfg_manager));
	    break;
	  }
	  MessageBox(wgui_hDialog, "Specified KickImage does not exist", "Configuration Error", 0);
	  wgui_action = WGUI_NO_ACTION;
	  break;
	case WGUI_QUIT_EMULATOR:
	  end_loop = TRUE;
	  quit_emulator = TRUE;
	  break;
	case WGUI_OPEN_CONFIGURATION:
	  wguiOpenConfigurationFile(wgui_cfg, wgui_hDialog);
	  wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
	  wgui_action = WGUI_NO_ACTION;
	  break;
	case WGUI_SAVE_CONFIGURATION:
	  cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));		  
	  wgui_action = WGUI_NO_ACTION;
	  break;
	case WGUI_SAVE_CONFIGURATION_AS:
	  wguiSaveConfigurationFileAs(wgui_cfg, wgui_hDialog);
	  wguiInsertCfgIntoHistory(iniGetCurrentConfigurationFilename(wgui_ini));
	  wgui_action = WGUI_NO_ACTION;
	  break;
	case WGUI_LOAD_HISTORY0:
	  //cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
	  if (cfgLoadFromFilename(wgui_cfg, iniGetConfigurationHistoryFilename(wgui_ini, 0)) == FALSE) 
	  {
	    wguiDeleteCfgFromHistory(0);
	  } 
	  else 
	  {
	    iniSetCurrentConfigurationFilename(wgui_ini, iniGetConfigurationHistoryFilename(wgui_ini, 0));
	  }
	  wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
	  wgui_action = WGUI_NO_ACTION;
	  break;
	case WGUI_LOAD_HISTORY1:
	  //cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
	  if (cfgLoadFromFilename(wgui_cfg, iniGetConfigurationHistoryFilename(wgui_ini, 1)) == FALSE) 
	  {
	    wguiDeleteCfgFromHistory(1);
	  } 
	  else 
	  {
	    iniSetCurrentConfigurationFilename(wgui_ini, iniGetConfigurationHistoryFilename(wgui_ini, 1));
	    wguiPutCfgInHistoryOnTop(1);
	  } 
	  wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
	  wgui_action = WGUI_NO_ACTION;
	  break;
	case WGUI_LOAD_HISTORY2:
	  //cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
	  if (cfgLoadFromFilename(wgui_cfg, iniGetConfigurationHistoryFilename(wgui_ini, 2)) == FALSE) 
	  {
	    wguiDeleteCfgFromHistory(2);
	  } 
	  else 
	  {
	    iniSetCurrentConfigurationFilename(wgui_ini, iniGetConfigurationHistoryFilename(wgui_ini, 2));
	    wguiPutCfgInHistoryOnTop(2);
	  } 
	  wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
	  wgui_action = WGUI_NO_ACTION;
	  break;
	case WGUI_LOAD_HISTORY3:
	  //cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
	  if (cfgLoadFromFilename(wgui_cfg, iniGetConfigurationHistoryFilename(wgui_ini, 3)) == FALSE) 
	  {
	    wguiDeleteCfgFromHistory(3);
	  } 
	  else 
	  {
	    iniSetCurrentConfigurationFilename(wgui_ini, iniGetConfigurationHistoryFilename(wgui_ini, 3));
	    wguiPutCfgInHistoryOnTop(3);
	  } 
	  wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
	  wgui_action = WGUI_NO_ACTION;
	  break;
	case WGUI_LOAD_STATE:
	  wguiOpenStateFile(wgui_cfg, wgui_hDialog);
	  wgui_action = WGUI_NO_ACTION;
	  break;
	case WGUI_SAVE_STATE:
	  wguiSaveStateFileAs(wgui_cfg, wgui_hDialog);
	  wgui_action = WGUI_NO_ACTION;
	  break;
	case WGUI_DEBUGGER_START:
	  end_loop = TRUE;
	  cfgManagerSetCurrentConfig(&cfg_manager, wgui_cfg);
	  fellowPreStartReset(fellowGetPreStartReset() |
	    cfgManagerConfigurationActivate(&cfg_manager));
	  debugger_start = TRUE;
	default:
	  break;
	}
      }

      // save main window position
      GetWindowRect(wgui_hDialog, &dialogRect);
      iniSetMainWindowPosition(wgui_ini, dialogRect.left, dialogRect.top);

      DestroyWindow(wgui_hDialog);
      if (!quit_emulator && debugger_start) {
	debugger_start = FALSE;
	//wdbgDebugSessionRun(NULL);
	wdebDebug();
      }
      else if (!quit_emulator) winDrvEmulationStart();
    } while (!quit_emulator);
    return quit_emulator;

    */

}

#endif