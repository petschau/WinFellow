/* @(#) $Id: WGUI.C,v 1.38 2013-01-13 18:31:09 peschau Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/* Windows GUI code                                                        */
/* Author: Petter Schau                                                    */
/* Author: Worfje (worfje@gmx.net)                                         */
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

/** @file
 *  Window GUI code
 */

#include "defs.h"

#ifdef WGUI

#include <windows.h>
#include <windowsx.h>
#include <windef.h>

#ifdef _FELLOW_DEBUG_CRT_MALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#ifdef _FELLOW_DEBUG_CRT_MALLOC
#include <crtdbg.h>
#endif

#include <process.h>
#include <commctrl.h>
#include <shlobj.h>
#include <prsht.h>
#include "gui_general.h"
#include "gui_debugger.h"
#include "commoncontrol_wrap.h"
#include "wgui.h"
#include "windrv.h"
#include "sound.h"
#include "listtree.h"
#include "gameport.h"
#include "fhfile.h"
#include "config.h"
#include "draw.h"
#include "wdbg.h"
#include "fhfile.h"
#include "ini.h"
#include "kbd.h"
#include "kbddrv.h"
#ifdef FELLOW_SUPPORT_CAPS
#include "caps_win32.h"
#endif
#include "floppy.h"
#include "fellow.h"
#include "GFXDRV.H"
#include "fileops.h"


HWND wgui_hDialog;                           /* Handle of the main dialog box */
cfg *wgui_cfg;                               /* GUI copy of configuration */
ini *wgui_ini;								 /* GUI copy of initdata */
STR extractedfilename[CFG_FILENAME_LENGTH];
STR extractedpathname[CFG_FILENAME_LENGTH];

wgui_drawmodes wgui_dm;								// data structure for resolution data
wgui_drawmodes* pwgui_dm = &wgui_dm;
wgui_drawmode *pwgui_dm_match;
BOOLE wgui_emulation_state = FALSE;
HBITMAP power_led_on_bitmap = 0;
HBITMAP power_led_off_bitmap = 0;
HBITMAP diskdrive_led_disabled_bitmap = 0;
HBITMAP diskdrive_led_off_bitmap = 0;


#define MAX_JOYKEY_PORT 2
#define MAX_DISKDRIVES 4

kbd_event gameport_keys_events[MAX_JOYKEY_PORT][MAX_JOYKEY_VALUE] = {
  {
    EVENT_JOY0_UP_ACTIVE,
      EVENT_JOY0_DOWN_ACTIVE,
      EVENT_JOY0_LEFT_ACTIVE,
      EVENT_JOY0_RIGHT_ACTIVE,
      EVENT_JOY0_FIRE0_ACTIVE,
      EVENT_JOY0_FIRE1_ACTIVE,
      EVENT_JOY0_AUTOFIRE0_ACTIVE,
      EVENT_JOY0_AUTOFIRE1_ACTIVE
  },
  {
    EVENT_JOY1_UP_ACTIVE,
      EVENT_JOY1_DOWN_ACTIVE,
      EVENT_JOY1_LEFT_ACTIVE,
      EVENT_JOY1_RIGHT_ACTIVE,
      EVENT_JOY1_FIRE0_ACTIVE,
      EVENT_JOY1_FIRE1_ACTIVE,
      EVENT_JOY1_AUTOFIRE0_ACTIVE,
      EVENT_JOY1_AUTOFIRE1_ACTIVE
    }
};

int gameport_keys_labels[MAX_JOYKEY_PORT][MAX_JOYKEY_VALUE] = {
  { IDC_GAMEPORT0_UP, IDC_GAMEPORT0_DOWN, IDC_GAMEPORT0_LEFT, IDC_GAMEPORT0_RIGHT, IDC_GAMEPORT0_FIRE0, IDC_GAMEPORT0_FIRE1, IDC_GAMEPORT0_AUTOFIRE0, IDC_GAMEPORT0_AUTOFIRE1 },
  { IDC_GAMEPORT1_UP, IDC_GAMEPORT1_DOWN, IDC_GAMEPORT1_LEFT, IDC_GAMEPORT1_RIGHT, IDC_GAMEPORT1_FIRE0, IDC_GAMEPORT1_FIRE1, IDC_GAMEPORT1_AUTOFIRE0, IDC_GAMEPORT1_AUTOFIRE1 }
};


#define DISKDRIVE_PROPERTIES 3
#define DISKDRIVE_PROPERTIES_MAIN 4

int diskimage_data[MAX_DISKDRIVES][DISKDRIVE_PROPERTIES] = {
  {IDC_EDIT_DF0_IMAGENAME, IDC_CHECK_DF0_ENABLED, IDC_CHECK_DF0_READONLY},
  {IDC_EDIT_DF1_IMAGENAME, IDC_CHECK_DF1_ENABLED, IDC_CHECK_DF1_READONLY},
  {IDC_EDIT_DF2_IMAGENAME, IDC_CHECK_DF2_ENABLED, IDC_CHECK_DF2_READONLY},
  {IDC_EDIT_DF3_IMAGENAME, IDC_CHECK_DF3_ENABLED, IDC_CHECK_DF3_READONLY}
};

enum {DID_IMAGENAME, DID_ENABLED, DID_READONLY};

int diskimage_data_main[MAX_DISKDRIVES][DISKDRIVE_PROPERTIES_MAIN] = {
  {IDC_EDIT_DF0_IMAGENAME_MAIN, IDC_BUTTON_DF0_EJECT_MAIN, IDC_BUTTON_DF0_FILEDIALOG_MAIN, IDC_IMAGE_DF0_LED_MAIN},
  {IDC_EDIT_DF1_IMAGENAME_MAIN, IDC_BUTTON_DF1_EJECT_MAIN, IDC_BUTTON_DF1_FILEDIALOG_MAIN, IDC_IMAGE_DF1_LED_MAIN},
  {IDC_EDIT_DF2_IMAGENAME_MAIN, IDC_BUTTON_DF2_EJECT_MAIN, IDC_BUTTON_DF2_FILEDIALOG_MAIN, IDC_IMAGE_DF2_LED_MAIN},
  {IDC_EDIT_DF3_IMAGENAME_MAIN, IDC_BUTTON_DF3_EJECT_MAIN, IDC_BUTTON_DF3_FILEDIALOG_MAIN, IDC_IMAGE_DF3_LED_MAIN}
};

enum {DID_IMAGENAME_MAIN, DID_EJECT_MAIN, DID_FILEDIALOG_MAIN, DID_LED_MAIN};

#define NUMBER_OF_CHIPRAM_SIZES 8

STR *wgui_chipram_strings[NUMBER_OF_CHIPRAM_SIZES] = {
  "256 KB", 
  "512 KB", 
  "768 KB", 
  "1024 KB", 
  "1280 KB", 
  "1536 KB", 
  "1792 KB", 
  "2048 KB"
};

#define NUMBER_OF_FASTRAM_SIZES 5

STR *wgui_fastram_strings[NUMBER_OF_FASTRAM_SIZES] = {
  "0 MB", 
  "1 MB", 
  "2 MB", 
  "4 MB", 
  "8 MB"
};

#define NUMBER_OF_BOGORAM_SIZES 8

STR *wgui_bogoram_strings[NUMBER_OF_BOGORAM_SIZES] = {
  "0 KB", 
  "256 KB", 
  "512 KB", 
  "768 KB", 
  "1024 KB", 
  "1280 KB", 
  "1536 KB", 
  "1792 KB"
};

//from sound.h: typedef enum {SOUND_15650, SOUND_22050, SOUND_31300, SOUND_44100} sound_rates;
#define NUMBER_OF_SOUND_RATES 4

int wgui_sound_rates_cci[NUMBER_OF_SOUND_RATES] = {
  IDC_RADIO_SOUND_15650, 
  IDC_RADIO_SOUND_22050, 
  IDC_RADIO_SOUND_31300, 
  IDC_RADIO_SOUND_44100
};

//from sound.h: typedef enum {SOUND_FILTER_ORIGINAL, SOUND_FILTER_ALWAYS, SOUND_FILTER_NEVER} sound_filters;
#define NUMBER_OF_SOUND_FILTERS 3
int wgui_sound_filters_cci[NUMBER_OF_SOUND_FILTERS] = {
  IDC_RADIO_SOUND_FILTER_ORIGINAL, 
  IDC_RADIO_SOUND_FILTER_ALWAYS, 
  IDC_RADIO_SOUND_FILTER_NEVER
};


//from CpuIntegration.h: typedef enum {M68000  = 0, M68010  = 1, M68020  = 2, M68030  = 3, M68EC30 = 4, M68040  = 5, M68EC40 = 6, M68060  = 7, M68EC60 = 8, M68EC20 = 9} cpu_types;
#define NUMBER_OF_CPUS 10
int wgui_cpus_cci[NUMBER_OF_CPUS] = {
  IDC_RADIO_68000,
  IDC_RADIO_68010,
  IDC_RADIO_68020,
  IDC_RADIO_68030,
  IDC_RADIO_68EC30,
  IDC_RADIO_68040,
  IDC_RADIO_68EC40, 
  IDC_RADIO_68060,
  IDC_RADIO_68EC60, 
  IDC_RADIO_68EC20
};

//from gameport.h: typedef enum {GP_NONE, GP_JOYKEY0, GP_JOYKEY1, GP_ANALOG0, GP_ANALOG1, GP_MOUSE0, GP_MOUSE1} gameport_inputs;
#define NUMBER_OF_GAMEPORT_STRINGS 6
STR *wgui_gameport_strings[NUMBER_OF_GAMEPORT_STRINGS] = {
  "none",
  "keyboard layout 1",
  "keyboard layout 2",
  "joystick 1",
  "joystick 2",
  "mouse"
};

// preset handling
STR wgui_preset_path[CFG_FILENAME_LENGTH] = "";
ULO wgui_num_presets = 0;
wgui_preset *wgui_presets = NULL;

/*============================================================================*/
/* Flags for various global events                                            */
/*============================================================================*/

typedef enum {
  WGUI_NO_ACTION,
  WGUI_START_EMULATION,
  WGUI_QUIT_EMULATOR,
  WGUI_CONFIGURATION,
  WGUI_OPEN_CONFIGURATION,
  WGUI_SAVE_CONFIGURATION,
  WGUI_SAVE_CONFIGURATION_AS,
  WGUI_LOAD_HISTORY0,
  WGUI_LOAD_HISTORY1,
  WGUI_LOAD_HISTORY2,
  WGUI_LOAD_HISTORY3,
  WGUI_DEBUGGER_START,
  WGUI_ABOUT,
  WGUI_LOAD_STATE,
  WGUI_SAVE_STATE
} wguiActions;

wguiActions wgui_action;

/*============================================================================*/
/* Forward declarations for each property sheet dialog procedure              */
/*============================================================================*/

INT_PTR CALLBACK wguiPresetDialogProc        (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiCPUDialogProc           (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiFloppyDialogProc	     (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiFloppyCreateDialogProc  (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiMemoryDialogProc	     (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiDisplayDialogProc	     (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiSoundDialogProc         (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiFilesystemAddDialogProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiFilesystemDialogProc    (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiHardfileCreateDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiHardfileAddDialogProc   (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiHardfileDialogProc      (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiGameportDialogProc      (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wguiVariousDialogProc	     (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*============================================================================*/
/* The following tables defines the data needed to create the property        */
/* sheets for the configuration dialog box.                                   */
/* -Number of property sheets                                                 */
/* -Resource ID for each property sheet                                       */
/* -Dialog procedure for each property sheet                                  */
/*============================================================================*/

#define PROP_SHEETS 10

typedef enum {
  PROPSHEETPRESETS    = 0,
  PROPSHEETCPU        = 1,
  PROPSHEETFLOPPY     = 2,
  PROPSHEETMEMORY     = 3,
  PROPSHEETDISPLAY    = 4,
  PROPSHEETSOUND      = 5,
  PROPSHEETFILESYSTEM = 6,
  PROPSHEETHARDFILE   = 7,
  PROPSHEETGAMEPORT   = 8,
  PROPSHEETVARIOUS    = 9
};

UINT wgui_propsheetRID[PROP_SHEETS] = {
  IDD_PRESETS,
  IDD_CPU,
  IDD_FLOPPY,
  IDD_MEMORY,
  IDD_DISPLAY,
  IDD_SOUND,
  IDD_FILESYSTEM,
  IDD_HARDFILE,
  IDD_GAMEPORT,
  IDD_VARIOUS
};

UINT wgui_propsheetICON[PROP_SHEETS] = {
  0,
  IDI_ICON_CPU,
  IDI_ICON_FLOPPY,
  //IDI_ICON_MEMORY,
  0,
  IDI_ICON_DISPLAY,
  IDI_ICON_SOUND,
  IDI_ICON_FILESYSTEM,
  IDI_ICON_HARDFILE,
  //IDI_ICON_GAMEPORT,
  //IDI_ICON_VARIOUS
  0,0
};

// in this struct, we remember the configuration dialog property sheet handles,
// so that a refresh can be triggered by the presets propery sheet
HWND wgui_propsheetHWND[PROP_SHEETS] = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

typedef INT_PTR (CALLBACK *wguiDlgProc)(HWND, UINT, WPARAM, LPARAM);

wguiDlgProc wgui_propsheetDialogProc[PROP_SHEETS] = {
  wguiPresetDialogProc,
  wguiCPUDialogProc,
  wguiFloppyDialogProc,
  wguiMemoryDialogProc,
  wguiDisplayDialogProc,
  wguiSoundDialogProc,
  wguiFilesystemDialogProc,
  wguiHardfileDialogProc,
  wguiGameportDialogProc,
  wguiVariousDialogProc
};

void wguiLoadBitmaps(void)
{
  if (power_led_on_bitmap == 0)
  {
    power_led_on_bitmap = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_POWER_LED_ON));
  }
  if (power_led_off_bitmap == 0)
  {
    power_led_off_bitmap = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_POWER_LED_OFF));
  }
  if (diskdrive_led_off_bitmap == 0)
  {
    diskdrive_led_off_bitmap = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_DISKDRIVE_LED_OFF));
  }
  if (diskdrive_led_disabled_bitmap == 0)
  {
    diskdrive_led_disabled_bitmap = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_DISKDRIVE_LED_DISABLED));
  }
}

void wguiReleaseBitmaps(void)
{
  if (power_led_on_bitmap != 0) DeleteObject(power_led_on_bitmap);
  if (power_led_off_bitmap != 0) DeleteObject(power_led_off_bitmap);
  if (diskdrive_led_disabled_bitmap != 0) DeleteObject(diskdrive_led_disabled_bitmap);
  if (diskdrive_led_off_bitmap != 0) DeleteObject(diskdrive_led_off_bitmap);
}

void wguiCheckMemorySettingsForChipset(void)
{
  if (!cfgGetECS(wgui_cfg) && cfgGetChipSize(wgui_cfg) > 0x80000)
  {
    MessageBox(wgui_hDialog, "The configuration uses more than 512k chip memory with OCS. The size has been reduced to 512k", "Configuration Error", 0);
    cfgSetChipSize(wgui_cfg, 0x80000);
    cfgSetConfigChangedSinceLastSave(wgui_cfg, TRUE);
  }
}

void wguiGetResolutionStrWithIndex(LONG index, char char_buffer[]) {

  felist *listnode;
  wgui_drawmode *pwguicfgwdm;
  LONG i;

  pwguicfgwdm = NULL;
  if (pwgui_dm_match->windowed) {
    // windowed
    listnode = pwgui_dm->reswindowed;
  } else {
    // fullscreen
    switch (pwgui_dm_match->colorbits) {
			case 16:
			  listnode = pwgui_dm->res16bit;
			  break;
			case 24:
			  listnode = pwgui_dm->res24bit;
			  break;
			case 32:
			  listnode = pwgui_dm->res32bit;
			  break;
    }
  }
  for (i=0; i < index; i++) {
    listnode = listNext(listnode);
  }
  pwguicfgwdm = (wgui_drawmode *)listNode(listnode);
  if (pwguicfgwdm != NULL) {
    sprintf(char_buffer, "%u by %u pixels", pwguicfgwdm->width, pwguicfgwdm->height);
  } else {
    sprintf(char_buffer, "unknown screen area");
  }
}

void wguiGetFrameSkippingStrWithIndex(LONG index, char char_buffer[]) {

  if (index == 0) {
    sprintf(char_buffer, "no skipping");
  } else {
    sprintf(char_buffer, "skip %d of %d frames", index, (index+1));	
  }
}

void wguiSetSliderTextAccordingToPosition(HWND windowHandle, int sliderIdentifier, int sliderTextIdentifier, void (*getSliderStrWithIndex)(LONG, char[])) {
  char buffer[255];
  ULO pos;

  pos = ccwSliderGetPosition(windowHandle, sliderIdentifier);
  getSliderStrWithIndex(pos, buffer);
  ccwStaticSetText(windowHandle, sliderTextIdentifier, buffer);
}

ULO wguiGetColorBitsFromComboboxIndex(LONG index) {
  if (pwgui_dm->comboxbox16bitindex == index) { return 16; }
  if (pwgui_dm->comboxbox24bitindex == index) { return 24; }
  if (pwgui_dm->comboxbox32bitindex == index) { return 32; }
  return 8;
}

LONG wguiGetComboboxIndexFromColorBits(ULO colorbits) {

  switch (colorbits) {
    case 16: return pwgui_dm->comboxbox16bitindex;
    case 24: return pwgui_dm->comboxbox24bitindex;
    case 32: return pwgui_dm->comboxbox32bitindex;
  }
  return 0;
}

wgui_drawmode *wguiConvertDrawModeNodeToGuiDrawNode(draw_mode *dm, wgui_drawmode *wdm) {

  wdm->height = dm->height;
  wdm->refresh = dm->refresh;
  wdm->width = dm->width;
  wdm->colorbits = dm->bits;
  wdm->windowed = dm->windowed;
  return wdm;
}

int wguiCompareScreenArea(wgui_drawmode *lineup_dm, wgui_drawmode *insert_dm) {

  if (insert_dm->width < lineup_dm->width) {
    return 0;
  } else {
    if (insert_dm->width == lineup_dm->width) {
      if (insert_dm->height < lineup_dm->height) {
	return 0;
      } else {
	return -1;
      }
    } else {
      //dm1->width > dm2->width
      return -1;
    }
  }
}

void wguiConvertDrawModeListToGuiDrawModes(wgui_drawmodes *wdms) {
  ULO i;
  felist* reslist;
  draw_mode * dm;
  wgui_drawmode* pwdm;
  ULO idw				= 0;
  ULO id16bit		= 0;
  ULO id24bit		= 0;
  ULO id32bit		= 0;
  wdms->reswindowed = NULL;
  wdms->res16bit = NULL;
  wdms->res24bit = NULL;
  wdms->res32bit = NULL;
  wdms->comboxbox16bitindex = -1;
  wdms->comboxbox24bitindex = -1;
  wdms->comboxbox32bitindex = -1;

  HDC desktopwindow_DC = GetWindowDC(GetDesktopWindow());
  int desktopwindow_bitspixel = GetDeviceCaps(desktopwindow_DC, BITSPIXEL);
  ReleaseDC(GetDesktopWindow(), desktopwindow_DC);
  bool allowWindowedModes = desktopwindow_bitspixel != 8;

  if (!allowWindowedModes)
  {
    wguiRequester("Your desktop is currently running an 8-bit color resolution.", "This is not supported.", "Only fullscreen modes will be available");          
  }

  for (reslist = drawGetModes(); reslist != NULL; reslist = listNext(reslist)) {
    dm = (draw_mode*)listNode(reslist);
    pwdm = (wgui_drawmode *) malloc(sizeof(wgui_drawmode));
    wguiConvertDrawModeNodeToGuiDrawNode(dm, pwdm);
    if (dm->windowed) {
      // windowed
      //pwdm->id = idw;
      if (allowWindowedModes)
      {
	wdms->reswindowed = listAddSorted(wdms->reswindowed, listNew(pwdm), (int (*)(void*,void*)) &wguiCompareScreenArea);
	idw++;
      }
    } else {
      // fullscreen
      switch(dm->bits) {
				case 16:
				  //pwdm->id = id16bit;
				  wdms->res16bit = listAddSorted(wdms->res16bit, listNew(pwdm), (int (*)(void*,void*)) &wguiCompareScreenArea);
				  id16bit++;
				  break;

				case 24:
				  //pwdm->id = id24bit;
				  wdms->res24bit = listAddSorted(wdms->res24bit, listNew(pwdm), (int (*)(void*,void*)) &wguiCompareScreenArea);
				  id24bit++;
				  break;

				case 32:
				  //pwdm->id = id32bit;
				  wdms->res32bit = listAddSorted(wdms->res32bit, listNew(pwdm), (int (*)(void*,void*)) &wguiCompareScreenArea);
				  id32bit++;
				  break;
      }
    }
  }
  wdms->numberofwindowed	= idw;
  wdms->numberof16bit			= id16bit;
  wdms->numberof24bit			= id24bit;
  wdms->numberof32bit			= id32bit;
  i=0;
  for (reslist=wdms->reswindowed; reslist!=NULL; reslist=listNext(reslist)) {
    ((wgui_drawmode *) listNode(reslist))->id = i;
    i++;
  }
  i=0;
  for (reslist=wdms->res16bit; reslist!=NULL; reslist=listNext(reslist)) {
    ((wgui_drawmode *) listNode(reslist))->id = i;
    i++;
  }
  i=0;
  for (reslist=wdms->res24bit; reslist!=NULL; reslist=listNext(reslist)) {
    ((wgui_drawmode *) listNode(reslist))->id = i;
    i++;
  }
  i=0;
  for (reslist=wdms->res32bit; reslist!=NULL; reslist=listNext(reslist)) {
    ((wgui_drawmode *) listNode(reslist))->id = i;
    i++;
  }
}

void wguiFreeGuiDrawModesList(wgui_drawmodes *wdms) {

  listFreeAll(wdms->reswindowed, TRUE);
  listFreeAll(wdms->res16bit, TRUE);
  listFreeAll(wdms->res24bit, TRUE);
  listFreeAll(wdms->res32bit, TRUE);
}

felist *wguiGetMatchingList(BOOLE windowed, ULO colorbits) {

  if (windowed) {
    // windowed
    return pwgui_dm->reswindowed;
  } else {
    // fullscreen
    switch(colorbits) {
			case 16:
			  return pwgui_dm->res16bit;
			case 24:
			  return pwgui_dm->res24bit;
			case 32:
			  return pwgui_dm->res32bit;
    }
  }
  return pwgui_dm->res16bit;
}

wgui_drawmode *wguiMatchResolution() {

  BOOLE windowed		= cfgGetScreenWindowed(wgui_cfg); 
  ULO width					= cfgGetScreenWidth(wgui_cfg);
  ULO height				= cfgGetScreenHeight(wgui_cfg);
  ULO colorbits			= cfgGetScreenColorBits(wgui_cfg);
  felist *reslist		= NULL;
  felist *i					= NULL;
  wgui_drawmode *dm = NULL;

  reslist = wguiGetMatchingList(windowed, colorbits);	
  for (i = reslist; i!=NULL; i = listNext(i)) {
    dm = (wgui_drawmode *) listNode(i);
    if ((dm->height == height) && (dm->width == width)) {
      return dm;
    }
  }
  // if no matching is found return pointer to first resolution of 32 or 16 colorbits
  if(listNode(pwgui_dm->res32bit) != NULL)
    return (wgui_drawmode *) listNode(pwgui_dm->res32bit);
  else
    return (wgui_drawmode *) listNode(pwgui_dm->res16bit);
}

/*============================================================================*/
/* Extract the filename from a full path name                                 */
/*============================================================================*/

STR *wguiExtractFilename(STR *fullpathname) {

  char *strpointer;

  strpointer = strrchr(fullpathname, '\\');
  strncpy(extractedfilename, fullpathname + strlen(fullpathname) - strlen(strpointer) + 1, strlen(fullpathname) - strlen(strpointer) - 1);
  return extractedfilename;
}

/*============================================================================*/
/* Extract the path from a full path name (includes filename)                 */
/*============================================================================*/

STR *wguiExtractPath(STR *fullpathname) {

  char *strpointer;

  strpointer = strrchr(fullpathname, '\\');
  strncpy(extractedpathname, fullpathname, strlen(fullpathname) - strlen(strpointer));
  extractedpathname[strlen(fullpathname) - strlen(strpointer)] = '\0';
  return extractedpathname;
}

/*============================================================================*/
/* Convert bool to string                                                     */
/*============================================================================*/

static STR *wguiGetBOOLEToString(BOOLE value) {
  return (value) ? "yes" : "no";
}

/*============================================================================*/
/* Runs a session in the file requester                                       */
/*============================================================================*/

static STR FileType[7][CFG_FILENAME_LENGTH] = {
  "ROM Images (.rom)\0*.rom\0ADF Diskfiles\0*.adf;*.adz;*.adf.gz;*.dms\0\0\0",
#ifndef FELLOW_SUPPORT_CAPS
  "ADF Diskfiles (.adf;.adz;.adf.gz;.dms)\0*.adf;*.adz;*.adf.gz;*.dms\0\0\0",
#else
  "ADF Diskfiles (.adf;.adz;.adf.gz;.dms)\0*.adf;*.adz;*.adf.gz;*.dms\0CAPS IPF Images (.ipf)\0*.ipf\0\0\0",
#endif
  "Key Files (.key)\0*.key\0\0\0", 
  "Hard Files (.hdf)\0*.hdf\0\0\0",
  "Configuration Files (.wfc)\0*.wfc\0\0\0",
  "Amiga Modules (.amod)\0*.amod\0\0\0",
  "State Files (.fst)\0\0\0"
};

  BOOLE wguiSelectFile(HWND hwndDlg, STR *filename, ULO filenamesize, 
    STR *title, SelectFileFlags SelectFileType) {
      OPENFILENAME ofn;
      STR filters[CFG_FILENAME_LENGTH];
      STR *pfilters;

      memcpy(filters, &FileType[SelectFileType], CFG_FILENAME_LENGTH);
      pfilters = &filters[0];

      ofn.lStructSize = sizeof(ofn);       /* Set all members to familiarize with */
      ofn.hwndOwner = hwndDlg;                            /* the possibilities... */
      ofn.hInstance = win_drv_hInstance;
      ofn.lpstrFilter = pfilters;
      ofn.lpstrCustomFilter = NULL;
      ofn.nMaxCustFilter = 0;
      ofn.nFilterIndex = 1;
      filename[0] = '\0';
      ofn.lpstrFile = filename;
      ofn.nMaxFile = filenamesize;
      ofn.lpstrFileTitle = NULL;
      ofn.nMaxFileTitle = 0;

      switch (SelectFileType) {
		case FSEL_ROM:
		  ofn.lpstrInitialDir = iniGetLastUsedKickImageDir(wgui_ini);
		  break;
		case FSEL_ADF:
		  ofn.lpstrInitialDir = cfgGetLastUsedDiskDir(wgui_cfg);
		  if (strncmp(ofn.lpstrInitialDir, "", CFG_FILENAME_LENGTH) == 0) {
		    ofn.lpstrInitialDir = iniGetLastUsedGlobalDiskDir(wgui_ini);
		  }
		  break;
		case FSEL_KEY:
		  ofn.lpstrInitialDir = iniGetLastUsedKeyDir(wgui_ini);
		  break;
		case FSEL_HDF:
		  ofn.lpstrInitialDir = iniGetLastUsedHdfDir(wgui_ini);
		  if (strncmp(ofn.lpstrInitialDir, "", CFG_FILENAME_LENGTH) == 0) {
		    cfgGetLastUsedDiskDir(wgui_cfg);
		  } else if (strncmp(ofn.lpstrInitialDir, "", CFG_FILENAME_LENGTH) == 0) {
		    ofn.lpstrInitialDir = iniGetLastUsedGlobalDiskDir(wgui_ini);
		  }
		  break;
		case FSEL_WFC:
		  ofn.lpstrInitialDir = iniGetLastUsedCfgDir(wgui_ini);
		  break;
		case FSEL_FST:
		  ofn.lpstrInitialDir = iniGetLastUsedStateFileDir(wgui_ini);
		  break;
		default:
		  ofn.lpstrInitialDir = NULL;
      }

      ofn.lpstrTitle = title;
      ofn.Flags = OFN_EXPLORER |
	OFN_FILEMUSTEXIST |
	OFN_NOCHANGEDIR;
      ofn.nFileOffset = 0;
      ofn.nFileExtension = 0;
      ofn.lpstrDefExt = NULL;
      ofn.lCustData = 0;
      ofn.lpfnHook = NULL;
      ofn.lpTemplateName = NULL;
      return GetOpenFileName(&ofn);
  }

  BOOLE wguiSaveFile(HWND hwndDlg, STR *filename, ULO filenamesize, 
    STR *title, SelectFileFlags SelectFileType) {
      OPENFILENAME ofn;
      STR filters[CFG_FILENAME_LENGTH];
      STR *pfilters;

      memcpy(filters, &FileType[SelectFileType], CFG_FILENAME_LENGTH);
      pfilters = &filters[0];

      ofn.lStructSize = sizeof(ofn);       /* Set all members to familiarize with */
      ofn.hwndOwner = hwndDlg;                            /* the possibilities... */
      ofn.hInstance = win_drv_hInstance;
      ofn.lpstrFilter = pfilters;
      ofn.lpstrCustomFilter = NULL;
      ofn.nMaxCustFilter = 0;
      ofn.nFilterIndex = 1;
      // filename[0] = '\0';
      ofn.lpstrFile = filename;
      ofn.nMaxFile = filenamesize;
      ofn.lpstrFileTitle = NULL;
      ofn.nMaxFileTitle = 0;

      switch (SelectFileType) {
		case FSEL_ROM:
		  ofn.lpstrInitialDir = iniGetLastUsedKickImageDir(wgui_ini);
		  break;
		case FSEL_ADF:
		  ofn.lpstrInitialDir = cfgGetLastUsedDiskDir(wgui_cfg);
		  if (strncmp(ofn.lpstrInitialDir, "", CFG_FILENAME_LENGTH) == 0) {
		    ofn.lpstrInitialDir = iniGetLastUsedGlobalDiskDir(wgui_ini);
		  }
		  break;
		case FSEL_KEY:
		  ofn.lpstrInitialDir = iniGetLastUsedKeyDir(wgui_ini);
		  break;
		case FSEL_HDF:
		  ofn.lpstrInitialDir = iniGetLastUsedHdfDir(wgui_ini);
		  if (strncmp(ofn.lpstrInitialDir, "", CFG_FILENAME_LENGTH) == 0) {
		    cfgGetLastUsedDiskDir(wgui_cfg);
		  } else if (strncmp(ofn.lpstrInitialDir, "", CFG_FILENAME_LENGTH) == 0) {
		    ofn.lpstrInitialDir = iniGetLastUsedGlobalDiskDir(wgui_ini);
		  }
		  break;
		case FSEL_WFC:
		  ofn.lpstrInitialDir = iniGetLastUsedCfgDir(wgui_ini);
		  break;
		case FSEL_MOD:
		  ofn.lpstrInitialDir = iniGetLastUsedModDir(wgui_ini);
		  break;
		case FSEL_FST:
		  ofn.lpstrInitialDir = iniGetLastUsedStateFileDir(wgui_ini);
		  break;
		default:
		  ofn.lpstrInitialDir = NULL;
      }

      ofn.lpstrTitle = title;
      ofn.Flags = OFN_EXPLORER |
	OFN_HIDEREADONLY |
	OFN_OVERWRITEPROMPT |
	OFN_NOCHANGEDIR;
      ofn.nFileOffset = 0;
      ofn.nFileExtension = 0;
      ofn.lpstrDefExt = (LPCSTR) &".wfc";
      ofn.lCustData = (LPARAM) 0;
      ofn.lpfnHook = NULL;
      ofn.lpTemplateName = NULL;
      return GetSaveFileName(&ofn);
  }

  BOOLE wguiSelectDirectory(HWND hwndDlg,
    STR *szPath, 
    STR *szDescription,
    ULO filenamesize,
    STR *szTitle) {

      BROWSEINFO bi = {
	hwndDlg,								// hwndOwner
	NULL,									// pidlRoot
	szPath,									// pszDisplayName
	szTitle,								// lpszTitle
	BIF_RETURNONLYFSDIRS,					// ulFlags
	NULL,									// lpfn
	0,										// lParam
	0										// iImage
      };

      LPITEMIDLIST pidlTarget;

      if(pidlTarget = SHBrowseForFolder(&bi)) {
        if(szDescription != NULL) 
	  strcpy(szDescription, bi.pszDisplayName);
	SHGetPathFromIDList(pidlTarget, szPath);   // Make sure it is a path
	CoTaskMemFree(pidlTarget);
	return TRUE;
      }
      return FALSE;
  }


  /*============================================================================*/
  /* Install history of configuration files into window menu                    */
  /*============================================================================*/

  void wguiRemoveAllHistory(void) {
    HMENU menu = GetMenu(wgui_hDialog);
    if (menu != 0)
    {
      HMENU submenu = GetSubMenu(menu, 0);
      if (submenu != 0)
      {
	RemoveMenu(submenu, ID_FILE_HISTORYCONFIGURATION0, MF_BYCOMMAND);
	RemoveMenu(submenu, ID_FILE_HISTORYCONFIGURATION1, MF_BYCOMMAND);
	RemoveMenu(submenu, ID_FILE_HISTORYCONFIGURATION2, MF_BYCOMMAND);
	RemoveMenu(submenu, ID_FILE_HISTORYCONFIGURATION3, MF_BYCOMMAND);
      }
    }
  }

  void wguiInstallHistoryIntoMenu(void) {
    STR cfgfilename[CFG_FILENAME_LENGTH+2];

    wguiRemoveAllHistory();
    cfgfilename[0] = '&'; cfgfilename[1] = '1'; cfgfilename[2] = ' '; cfgfilename[3] = '\0';
    if (strcmp(iniGetConfigurationHistoryFilename(wgui_ini, 0),"") != 0) {
      strcat(cfgfilename, iniGetConfigurationHistoryFilename(wgui_ini, 0));
      AppendMenu(GetSubMenu(GetMenu(wgui_hDialog),0), MF_STRING, ID_FILE_HISTORYCONFIGURATION0, cfgfilename);
    }

    cfgfilename[1] = '2'; cfgfilename[3] = '\0';
    if (strcmp(iniGetConfigurationHistoryFilename(wgui_ini, 1),"") != 0) {
      strcat(cfgfilename, iniGetConfigurationHistoryFilename(wgui_ini, 1));
      AppendMenu(GetSubMenu(GetMenu(wgui_hDialog),0), MF_STRING, ID_FILE_HISTORYCONFIGURATION1, cfgfilename);
    }

    cfgfilename[1] = '3'; cfgfilename[3] = '\0';
    if (strcmp(iniGetConfigurationHistoryFilename(wgui_ini, 2),"") != 0) {
      strcat(cfgfilename, iniGetConfigurationHistoryFilename(wgui_ini, 2));
      AppendMenu(GetSubMenu(GetMenu(wgui_hDialog),0), MF_STRING, ID_FILE_HISTORYCONFIGURATION2, cfgfilename);
    }

    cfgfilename[1] = '4'; cfgfilename[3] = '\0';
    if (strcmp(iniGetConfigurationHistoryFilename(wgui_ini, 3),"") != 0) {
      strcat(cfgfilename, iniGetConfigurationHistoryFilename(wgui_ini, 3));
      AppendMenu(GetSubMenu(GetMenu(wgui_hDialog),0), MF_STRING, ID_FILE_HISTORYCONFIGURATION3, cfgfilename);
    }
  }

  void wguiPutCfgInHistoryOnTop(ULO cfgtotop) {
    ULO i;
    STR cfgfilename[CFG_FILENAME_LENGTH];

    strncpy(cfgfilename, iniGetConfigurationHistoryFilename(wgui_ini, cfgtotop), CFG_FILENAME_LENGTH);
    for (i=cfgtotop; i>0; i--) {
      iniSetConfigurationHistoryFilename(wgui_ini, i, iniGetConfigurationHistoryFilename(wgui_ini, i-1));
    }
    iniSetConfigurationHistoryFilename(wgui_ini, 0, cfgfilename);
    wguiInstallHistoryIntoMenu();
  }

  void wguiInsertCfgIntoHistory(STR *cfgfilenametoinsert) {
    STR cfgfilename[CFG_FILENAME_LENGTH];
    ULO i;
    BOOL exists;

    // first we need to check if the file is already in the history
    exists = FALSE;
    i = 0;
    while ((i<4) && (exists == FALSE)) {
      i++;
      if (strncmp(cfgfilenametoinsert, iniGetConfigurationHistoryFilename(wgui_ini, i-1), CFG_FILENAME_LENGTH) == 0) {
	exists = TRUE;
      }
    }

    if (exists == TRUE) {
      wguiPutCfgInHistoryOnTop(i-1);	
    } else {
      for (i=3; i>0; i--) {
	cfgfilename[0]='\0';
	strncat(cfgfilename, iniGetConfigurationHistoryFilename(wgui_ini, i-1), CFG_FILENAME_LENGTH);
	iniSetConfigurationHistoryFilename(wgui_ini, i, cfgfilename);
      }
      iniSetConfigurationHistoryFilename(wgui_ini, 0, cfgfilenametoinsert);
      wguiInstallHistoryIntoMenu();
    }
  }

  void wguiDeleteCfgFromHistory(ULO itemtodelete) {
    ULO i;

    for (i=itemtodelete; i<3; i++) {
      iniSetConfigurationHistoryFilename(wgui_ini, i, iniGetConfigurationHistoryFilename(wgui_ini, i+1));
    }
    iniSetConfigurationHistoryFilename(wgui_ini, 3, "");
    wguiInstallHistoryIntoMenu();
  }

  void wguiSwapCfgsInHistory(ULO itemA, ULO itemB) {
    STR cfgfilename[CFG_FILENAME_LENGTH];

    strncpy(cfgfilename, iniGetConfigurationHistoryFilename(wgui_ini, itemA), CFG_FILENAME_LENGTH);
    iniSetConfigurationHistoryFilename(wgui_ini, itemA, iniGetConfigurationHistoryFilename(wgui_ini, itemB));
    iniSetConfigurationHistoryFilename(wgui_ini, itemB, cfgfilename);
    wguiInstallHistoryIntoMenu();
  }


  /*============================================================================*/
  /* Saves and loads configuration files (*.wfc)                                */
  /*============================================================================*/

  void wguiSaveConfigurationFileAs(cfg *conf, HWND hwndDlg) {
    STR filename[CFG_FILENAME_LENGTH];

    strcpy(filename, "");

    if (wguiSaveFile(hwndDlg, filename, CFG_FILENAME_LENGTH, "Save Configuration As:", FSEL_WFC)) {
      cfgSaveToFilename(wgui_cfg, filename);
      iniSetCurrentConfigurationFilename(wgui_ini, filename);
      iniSetLastUsedCfgDir(wgui_ini, wguiExtractPath(filename));
    }
  }

  void wguiOpenConfigurationFile(cfg *conf, HWND hwndDlg) {
    STR filename[CFG_FILENAME_LENGTH];

    if (wguiSelectFile(hwndDlg, filename, CFG_FILENAME_LENGTH, "Open", FSEL_WFC)) {
      cfgLoadFromFilename(wgui_cfg, filename, false);
      iniSetCurrentConfigurationFilename(wgui_ini, filename);
      iniSetLastUsedCfgDir(wgui_ini, wguiExtractPath(filename));
      wguiCheckMemorySettingsForChipset();
      wguiInsertCfgIntoHistory(filename);
    }
  }

  /*============================================================================*/
  /* Saves and loads state files (*.fst)                                        */
  /*============================================================================*/

  void wguiSaveStateFileAs(cfg *conf, HWND hwndDlg)
  {
    STR filename[CFG_FILENAME_LENGTH];

    strcpy(filename, "");

    if (wguiSaveFile(hwndDlg, filename, CFG_FILENAME_LENGTH, "Save State File As:", FSEL_FST))
    {
      fellowSaveState(filename);
      iniSetLastUsedStateFileDir(wgui_ini, wguiExtractPath(filename));
    }
  }

  void wguiOpenStateFile(cfg *conf, HWND hwndDlg)
  {
    STR filename[CFG_FILENAME_LENGTH];

    if (wguiSelectFile(hwndDlg, filename, CFG_FILENAME_LENGTH, "Open State File", FSEL_FST))
    {
      fellowLoadState(filename);
      iniSetLastUsedStateFileDir(wgui_ini, wguiExtractPath(filename));
    }
  }

  /*============================================================================*/
  /* CPU config                                                                 */
  /*============================================================================*/

  /* install CPU config */

  void wguiInstallCPUConfig(HWND hwndDlg, cfg *conf) {
    int slidervalue, i;

    /* set CPU type */
    for(i=0; i<NUMBER_OF_CPUS; i++)
      ccwButtonUncheck(hwndDlg, wgui_cpus_cci[i]);
    ccwButtonSetCheck(hwndDlg, wgui_cpus_cci[cfgGetCPUType(conf)]);

    /* Set CPU speed */
    ccwSliderSetRange(hwndDlg, IDC_SLIDER_CPU_SPEED, 0, 4);
    switch (cfgGetCPUSpeed(conf)) {
    case 8:
      slidervalue = 0;
      break;
    default:
    case 4:
      slidervalue = 1;
      break;
    case 2:
      slidervalue = 2;
      break;
    case 1:
      slidervalue = 3;
      break;
    case 0:
      slidervalue = 4;
      break;
    }
    ccwSliderSetPosition(hwndDlg, IDC_SLIDER_CPU_SPEED, (LPARAM) slidervalue);
  }

  /* Extract CPU config */

  void wguiExtractCPUConfig(HWND hwndDlg, cfg *conf) {
    ULO i;

    /* get CPU type */
    for (i=0; i<NUMBER_OF_CPUS; i++) {
      if (ccwButtonGetCheck(hwndDlg, wgui_cpus_cci[i])) { cfgSetCPUType(conf, (cpu_integration_models) i); }
    }	

    /* get CPU speed */
    switch (ccwSliderGetPosition(hwndDlg, IDC_SLIDER_CPU_SPEED)) {
    case 0:
      cfgSetCPUSpeed(conf, 8);
      break;
    case 1:
      cfgSetCPUSpeed(conf, 4);
      break;
    case 2:
      cfgSetCPUSpeed(conf, 2);
      break;
    case 3:
      cfgSetCPUSpeed(conf, 1);
      break;
    case 4:
      cfgSetCPUSpeed(conf, 0);
      break;
    }
  }


  /*============================================================================*/
  /* Floppy config                                                              */
  /*============================================================================*/

  /* install floppy config */

  void wguiInstallFloppyConfig(HWND hwndDlg, cfg *conf) {
    ULO i;

    /* set floppy image names */

    for (i=0; i<MAX_DISKDRIVES; i++) {
      ccwEditSetText(hwndDlg, diskimage_data[i][DID_IMAGENAME], cfgGetDiskImage(conf, i));
      ccwButtonCheckConditional(hwndDlg, diskimage_data[i][DID_ENABLED], cfgGetDiskEnabled(conf, i));
      ccwButtonCheckConditional(hwndDlg, diskimage_data[i][DID_READONLY], cfgGetDiskReadOnly(conf, i));
    }

    /* set fast DMA check box */

    ccwButtonCheckConditional(hwndDlg, IDC_CHECK_FAST_DMA, cfgGetDiskFast(conf));
  }

  /* set floppy images in main window */

  void wguiInstallFloppyMain(HWND hwndDlg, cfg *conf) {
    ULO i;

    wguiLoadBitmaps();
    for (i=0; i<MAX_DISKDRIVES; i++) {
      ccwEditSetText(hwndDlg, diskimage_data_main[i][DID_IMAGENAME_MAIN], cfgGetDiskImage(conf, i));
      ccwEditEnableConditional(hwndDlg, diskimage_data_main[i][DID_IMAGENAME_MAIN], cfgGetDiskEnabled(conf, i));
      ccwButtonEnableConditional(hwndDlg, diskimage_data_main[i][DID_EJECT_MAIN], cfgGetDiskEnabled(conf,i));
      ccwButtonEnableConditional(hwndDlg, diskimage_data_main[i][DID_FILEDIALOG_MAIN], cfgGetDiskEnabled(conf,i));
      ccwSetImageConditional(hwndDlg, diskimage_data_main[i][DID_LED_MAIN], diskdrive_led_off_bitmap, diskdrive_led_disabled_bitmap, cfgGetDiskEnabled(conf,i));
    }
  }

  /* Extract floppy config */

  void wguiExtractFloppyConfig(HWND hwndDlg, cfg *conf) {
    char tmp[CFG_FILENAME_LENGTH];
    ULO i;

    /* Get floppy disk image names */

    for (i=0; i<MAX_DISKDRIVES; i++) {
      ccwEditGetText(hwndDlg, diskimage_data[i][DID_IMAGENAME], tmp, CFG_FILENAME_LENGTH);
      cfgSetDiskImage(conf, i, tmp);
      cfgSetDiskEnabled(conf, i, ccwButtonGetCheck(hwndDlg, diskimage_data[i][DID_ENABLED]));
      cfgSetDiskReadOnly(conf, i, ccwButtonGetCheck(hwndDlg,diskimage_data[i][DID_READONLY]));
    }

    /* Get fast DMA */

    cfgSetDiskFast(conf, ccwButtonGetCheck(hwndDlg,IDC_CHECK_FAST_DMA));
  }

  /* Extract floppy config from main window */

  void wguiExtractFloppyMain(HWND hwndDlg, cfg *conf) {
    char tmp[CFG_FILENAME_LENGTH];
    ULO i;

    /* Get floppy disk image names */

    for (i=0; i<MAX_DISKDRIVES; i++) {
      if (cfgGetDiskEnabled(conf, i) == FALSE) {
	ccwEditGetText(hwndDlg, diskimage_data_main[i][DID_IMAGENAME_MAIN], tmp, CFG_FILENAME_LENGTH);
	cfgSetDiskImage(conf, i, tmp);
      }
    }
  }



  /*============================================================================*/
  /* Memory config                                                              */
  /*============================================================================*/

  /* Install memory config */

  void wguiInstallMemoryConfig(HWND hwndDlg, cfg *conf) {
    ULO fastindex;
    ULO i;

    /* Add choice choices */

    for (i=0; i<NUMBER_OF_CHIPRAM_SIZES; i++) {
      ccwComboBoxAddString(hwndDlg, IDC_COMBO_CHIP, wgui_chipram_strings[i]);
    }  

    for (i=0; i<NUMBER_OF_FASTRAM_SIZES; i++) {
      ccwComboBoxAddString(hwndDlg, IDC_COMBO_FAST, wgui_fastram_strings[i]);
    }

    for (i=0; i<NUMBER_OF_BOGORAM_SIZES; i++) {
      ccwComboBoxAddString(hwndDlg, IDC_COMBO_BOGO, wgui_bogoram_strings[i]);
    }

    /* Set current memory size selection */

    switch (cfgGetFastSize(conf)) {
    case 0:
      fastindex = 0;
      break;
    case 0x100000:
      fastindex = 1;
      break;
    case 0x200000:
      fastindex = 2;
      break;
    case 0x400000:
      fastindex = 3;
      break;
    case 0x800000:
      fastindex = 4;
      break;
    }
    ccwComboBoxSetCurrentSelection(hwndDlg, IDC_COMBO_CHIP, (cfgGetChipSize(conf) / 0x40000) - 1);
    ccwComboBoxSetCurrentSelection(hwndDlg, IDC_COMBO_FAST, fastindex);
    ccwComboBoxSetCurrentSelection(hwndDlg, IDC_COMBO_BOGO, cfgGetBogoSize(conf) / 0x40000);

    /* Set current ROM and key file names */

    ccwEditSetText(hwndDlg, IDC_EDIT_KICKSTART, cfgGetKickImage(conf));
    ccwEditSetText(hwndDlg, IDC_EDIT_KEYFILE, cfgGetKey(conf));
  }

  /* Extract memory config */

  void wguiExtractMemoryConfig(HWND hwndDlg, cfg *conf) {
    char tmp[CFG_FILENAME_LENGTH];
    ULO cursel;
    ULO sizes1[9] = {0,
      0x40000,
      0x80000,
      0xc0000,
      0x100000,
      0x140000,
      0x180000,
      0x1c0000,
      0x200000};
    ULO sizes2[5] = {0,
      0x100000,
      0x200000,
      0x400000,
      0x800000};

    /* Get current memory sizes */

    cursel = ccwComboBoxGetCurrentSelection(hwndDlg, IDC_COMBO_CHIP);
    if (cursel > 7) cursel = 7;
    cfgSetChipSize(conf, sizes1[cursel + 1]);

    cursel = ccwComboBoxGetCurrentSelection(hwndDlg, IDC_COMBO_BOGO);
    if (cursel > 7) cursel = 7;
    cfgSetBogoSize(conf, sizes1[cursel]);

    cursel = ccwComboBoxGetCurrentSelection(hwndDlg, IDC_COMBO_FAST);
    if (cursel > 4) cursel = 4;
    cfgSetFastSize(conf, sizes2[cursel]);

    /* Get current kickstart image and keyfile */

    ccwEditGetText(hwndDlg, IDC_EDIT_KICKSTART, tmp, CFG_FILENAME_LENGTH);
    cfgSetKickImage(conf, tmp);
    ccwEditGetText(hwndDlg, IDC_EDIT_KEYFILE, tmp, CFG_FILENAME_LENGTH);
    cfgSetKey(conf, tmp);
  }

  /*============================================================================*/
  /* Blitter config                                                             */
  /*============================================================================*/

  /* Install Blitter config */

  void wguiInstallBlitterConfig(HWND hwndDlg, cfg *conf) {

    /* Set blitter operation type */

    if (cfgGetBlitterFast(conf))
      ccwButtonSetCheck(hwndDlg, IDC_RADIO_BLITTER_IMMEDIATE);
    else
      ccwButtonSetCheck(hwndDlg, IDC_RADIO_BLITTER_NORMAL);

    /* Set chipset type */

    if (cfgGetECS(conf))
      ccwButtonSetCheck(hwndDlg, IDC_RADIO_BLITTER_ECS);
    else
      ccwButtonSetCheck(hwndDlg, IDC_RADIO_BLITTER_OCS);
  }

  /* Extract Blitter config */

  void wguiExtractBlitterConfig(HWND hwndDlg, cfg *conf) {
    /* get current blitter operation type */
    cfgSetBlitterFast(conf, ccwButtonGetCheck(hwndDlg, IDC_RADIO_BLITTER_IMMEDIATE));

    /* get current chipset type */
    cfgSetECS(conf, ccwButtonGetCheckBool(hwndDlg, IDC_RADIO_BLITTER_ECS));
  }


  /*============================================================================*/
  /* Sound config                                                               */
  /*============================================================================*/

  /* Install sound config */

  void wguiInstallSoundConfig(HWND hwndDlg, cfg *conf) {

	/* set sound volume slider */
    ccwSliderSetRange(hwndDlg, IDC_SLIDER_SOUND_VOLUME, 0, 100);
    ccwSliderSetPosition(hwndDlg, IDC_SLIDER_SOUND_VOLUME, cfgGetSoundVolume(conf));

    /* Set sound rate */
    ccwButtonSetCheck(hwndDlg, wgui_sound_rates_cci[cfgGetSoundRate(conf)]);

    /* set sound hardware notification */
    ccwButtonCheckConditional(hwndDlg, IDC_CHECK_SOUND_NOTIFICATION, cfgGetSoundNotification(conf) == SOUND_DSOUND_NOTIFICATION); 

    /* Set sound channels */
    if (cfgGetSoundStereo(conf))
      ccwButtonSetCheck(hwndDlg, IDC_RADIO_SOUND_STEREO);
    else
      ccwButtonSetCheck(hwndDlg, IDC_RADIO_SOUND_MONO);

    /* Set sound bits */
    if (cfgGetSound16Bits(conf))
      ccwButtonSetCheck(hwndDlg, IDC_RADIO_SOUND_16BITS);
    else
      ccwButtonSetCheck(hwndDlg, IDC_RADIO_SOUND_8BITS);

    /* Set sound filter */
    ccwButtonSetCheck(hwndDlg, wgui_sound_filters_cci[cfgGetSoundFilter(conf)]);

    /* Set sound WAV dump */
    ccwButtonCheckConditional(hwndDlg, IDC_CHECK_SOUND_WAV, cfgGetSoundWAVDump(conf));

    /* set slider of buffer length */
    ccwSliderSetRange(hwndDlg, IDC_SLIDER_SOUND_BUFFER_LENGTH, 10, 80);
    ccwSliderSetPosition(hwndDlg, IDC_SLIDER_SOUND_BUFFER_LENGTH, cfgGetSoundBufferLength(conf));
  }

  /* Extract sound config */

  void wguiExtractSoundConfig(HWND hwndDlg, cfg *conf) {
	ULO i;

	/* get current sound volume */
	cfgSetSoundVolume(conf, ccwSliderGetPosition(hwndDlg, IDC_SLIDER_SOUND_VOLUME));

    /* get current sound rate */
    for (i=0; i<NUMBER_OF_SOUND_RATES; i++) {
      if (ccwButtonGetCheck(hwndDlg, wgui_sound_rates_cci[i])) { cfgSetSoundRate(conf, (sound_rates)i); }
    }

    /* get current sound channels */
    cfgSetSoundStereo(conf, ccwButtonGetCheckBool(hwndDlg, IDC_RADIO_SOUND_STEREO));

    /* get current sound bits */
    cfgSetSound16Bits(conf, ccwButtonGetCheckBool(hwndDlg, IDC_RADIO_SOUND_16BITS));

    /* get current sound filter */
    for (i=0; i<NUMBER_OF_SOUND_FILTERS; i++) {
      if (ccwButtonGetCheck(hwndDlg, wgui_sound_filters_cci[i])) { cfgSetSoundFilter(conf, (sound_filters)i); }
    }

    /* get current sound WAV dump */
    cfgSetSoundWAVDump(conf, ccwButtonGetCheck(hwndDlg, IDC_CHECK_SOUND_WAV));

    /* get notify option */
    if (ccwButtonGetCheck(hwndDlg, IDC_CHECK_SOUND_NOTIFICATION))
      cfgSetSoundNotification(conf, SOUND_DSOUND_NOTIFICATION);
    else if (!ccwButtonGetCheck(hwndDlg, IDC_CHECK_SOUND_NOTIFICATION))
      cfgSetSoundNotification(conf, SOUND_MMTIMER_NOTIFICATION);

    /* get slider of buffer length */
    cfgSetSoundBufferLength(conf, ccwSliderGetPosition(hwndDlg, IDC_SLIDER_SOUND_BUFFER_LENGTH));
  }


  /*============================================================================*/
  /* Gameport config                                                            */
  /*============================================================================*/

  /* Install gameport config */

  void wguiInstallGameportConfig(HWND hwndDlg, cfg *conf) {
    ULO i, j;

    /* fill comboboxes with choices */
    for (i=0; i<NUMBER_OF_GAMEPORT_STRINGS; i++) {
      ccwComboBoxAddString(hwndDlg, IDC_COMBO_GAMEPORT1, wgui_gameport_strings[i]);
      ccwComboBoxAddString(hwndDlg, IDC_COMBO_GAMEPORT2, wgui_gameport_strings[i]);
    }

    /* set current selection */
    ccwComboBoxSetCurrentSelection(hwndDlg, IDC_COMBO_GAMEPORT1, cfgGetGameport(conf, 0));
    ccwComboBoxSetCurrentSelection(hwndDlg, IDC_COMBO_GAMEPORT2, cfgGetGameport(conf, 1));

    /* set current used keys for keyboard layout replacements */
    for( i=0; i<MAX_JOYKEY_PORT; i++ ) {
      for ( j=0; j<MAX_JOYKEY_VALUE; j++) {
	ccwStaticSetText(hwndDlg, gameport_keys_labels[i][j], kbdDrvKeyPrettyString(kbdDrvJoystickReplacementGet(gameport_keys_events[i][j])));
      }
    }
  }

  /* Extract gameport config */

  void wguiExtractGameportConfig(HWND hwndDlg, cfg *conf) {

    /* get current gameport inputs */
    cfgSetGameport(conf, 0, (gameport_inputs)ccwComboBoxGetCurrentSelection(hwndDlg, IDC_COMBO_GAMEPORT1));
    cfgSetGameport(conf, 1, (gameport_inputs)ccwComboBoxGetCurrentSelection(hwndDlg, IDC_COMBO_GAMEPORT2));
  }


  /*============================================================================*/
  /* various config                                                             */
  /*============================================================================*/

  /* install various config */

  void wguiInstallVariousConfig(HWND hwndDlg, cfg *conf) {
    /* set measure speed */
    ccwButtonCheckConditional(hwndDlg, IDC_CHECK_VARIOUS_SPEED, cfgGetMeasureSpeed(conf));

    /* set draw LED */
    ccwButtonCheckConditional(hwndDlg, IDC_CHECK_VARIOUS_LED, cfgGetScreenDrawLEDs(conf));

    /* set autoconfig disable */
    ccwButtonCheckConditional(hwndDlg, IDC_CHECK_AUTOCONFIG_DISABLE, !cfgGetUseAutoconfig(conf));

    /* set real-time clock */
    ccwButtonCheckConditional(hwndDlg, IDC_CHECK_VARIOUS_RTC, cfgGetRtc(conf));

    /* set silent sound emulation */
    ccwButtonCheckConditional(hwndDlg, IDC_CHECK_SOUND_EMULATE, cfgGetSoundEmulation(conf) == SOUND_EMULATE ? TRUE : FALSE);

    /* set automatic interlace compensation */
    ccwButtonCheckConditional(hwndDlg, IDC_CHECK_GRAPHICS_DEINTERLACE, cfgGetDeinterlace(conf));
  }

  /* extract various config */

  void wguiExtractVariousConfig(HWND hwndDlg, cfg *conf) {

    /* get measure speed */
    cfgSetMeasureSpeed(conf, ccwButtonGetCheckBool(hwndDlg, IDC_CHECK_VARIOUS_SPEED));

    /* get draw LED */
    cfgSetScreenDrawLEDs(conf, ccwButtonGetCheckBool(hwndDlg, IDC_CHECK_VARIOUS_LED));

    /* get autoconfig disable */
    cfgSetUseAutoconfig(conf, !ccwButtonGetCheck(hwndDlg, IDC_CHECK_AUTOCONFIG_DISABLE));

    /* get real-time clock */
    cfgSetRtc(conf, ccwButtonGetCheckBool(hwndDlg, IDC_CHECK_VARIOUS_RTC));

    /* get silent sound emulation */
    cfgSetSoundEmulation(conf, ccwButtonGetCheck(hwndDlg, IDC_CHECK_SOUND_EMULATE) ? SOUND_EMULATE : SOUND_PLAY);

    /* get automatic interlace compensation */
    cfgSetDeinterlace(conf, ccwButtonGetCheckBool(hwndDlg, IDC_CHECK_GRAPHICS_DEINTERLACE));
  }


  /*============================================================================*/
  /* Hardfile config                                                            */
  /*============================================================================*/

  /* Update hardfile description in the list view box */

  void wguiHardfileUpdate(HWND lvHWND, cfg_hardfile *hf, ULO i, BOOL add) {
    LV_ITEM lvi;
    STR stmp[48];

    memset(&lvi, 0, sizeof(lvi));
    lvi.mask = LVIF_TEXT;
    sprintf(stmp, "FELLOW%u", i);
    lvi.iItem = i;
    lvi.pszText = stmp;
    lvi.cchTextMax = (int) strlen(stmp);
    lvi.iSubItem = 0;
    if (!add) ListView_SetItem(lvHWND, &lvi);
    else ListView_InsertItem(lvHWND, &lvi);
    lvi.pszText = hf->filename;
    lvi.cchTextMax = (int) strlen(hf->filename);
    lvi.iSubItem = 1;
    ListView_SetItem(lvHWND, &lvi);
    sprintf(stmp, "%s", (hf->readonly) ? "R" : "RW");
    lvi.pszText = stmp;
    lvi.cchTextMax = (int) strlen(stmp);
    lvi.iSubItem = 2;
    ListView_SetItem(lvHWND, &lvi);
    sprintf(stmp, "%u", hf->sectorspertrack);
    lvi.pszText = stmp;
    lvi.cchTextMax = (int) strlen(stmp);
    lvi.iSubItem = 3;
    ListView_SetItem(lvHWND, &lvi);
    sprintf(stmp, "%u", hf->surfaces);
    lvi.pszText = stmp;
    lvi.cchTextMax = (int) strlen(stmp);
    lvi.iSubItem = 4;
    ListView_SetItem(lvHWND, &lvi);
    sprintf(stmp, "%u", hf->reservedblocks);
    lvi.pszText = stmp;
    lvi.cchTextMax = (int) strlen(stmp);
    lvi.iSubItem = 5;
    ListView_SetItem(lvHWND, &lvi);
    sprintf(stmp, "%u", hf->bytespersector);
    lvi.pszText = stmp;
    lvi.cchTextMax = (int) strlen(stmp);
    lvi.iSubItem = 6;
    ListView_SetItem(lvHWND, &lvi);
  }

  /* Install hardfile config */

#define HARDFILE_COLS 7
  void wguiInstallHardfileConfig(HWND hwndDlg, cfg *conf) {
    LV_COLUMN lvc;
    HWND lvHWND = GetDlgItem(hwndDlg, IDC_LIST_HARDFILES);
    ULO i, hfcount;
    STR *colheads[HARDFILE_COLS] = {"Unit",
      "File",
      "RW",
      "Sectors per Track",
      "Surfaces",
      "Reserved Blocks",
      "Bytes Per Sector"};

    /* Create list view control columns */

    memset(&lvc, 0, sizeof(lvc));
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    for (i = 0; i < HARDFILE_COLS; i++) {
      ULO colwidth = ListView_GetStringWidth(lvHWND, colheads[i]);
      if (i == 0) colwidth += 48;
      else if (i == 1) colwidth += 216;
      else colwidth += 16;
      lvc.pszText = colheads[i];
      lvc.cchTextMax = (int) strlen(colheads[i]);
      lvc.cx = colwidth;
      ListView_InsertColumn(lvHWND, i, &lvc);
    }

    /* Add current hardfiles to the list */

    hfcount = cfgGetHardfileCount(conf);
    ListView_SetItemCount(lvHWND, hfcount);
    for (i = 0; i < hfcount; i++) {
      cfg_hardfile hf = cfgGetHardfile(conf, i);
      wguiHardfileUpdate(lvHWND, &hf, i, TRUE);
    }
    ListView_SetExtendedListViewStyle(lvHWND, LVS_EX_FULLROWSELECT);
  }

  /* Extract hardfile config */

  void wguiExtractHardfileConfig(HWND hwndDlg, cfg *conf) {
  }


  /* Execute hardfile add or edit data */

  cfg_hardfile *wgui_current_hardfile_edit = NULL;
  ULO wgui_current_hardfile_edit_index = 0;

  /* Run a hardfile edit or add dialog */

  BOOLE wguiHardfileAdd(HWND hwndDlg, 
    cfg *conf, 
    BOOLE add, 
    ULO index,
    cfg_hardfile *target) {
      wgui_current_hardfile_edit = target;
      wgui_current_hardfile_edit_index = index;
      if (add) cfgSetHardfileUnitDefaults(target);
      return DialogBox(win_drv_hInstance,
	MAKEINTRESOURCE(IDD_HARDFILE_ADD),
	hwndDlg,
	wguiHardfileAddDialogProc) == IDOK;
  }

  BOOLE wguiHardfileCreate(HWND hwndDlg,
    cfg *conf,
    ULO index,
    cfg_hardfile *target) {
      wgui_current_hardfile_edit = target;
      wgui_current_hardfile_edit_index = index;
      cfgSetHardfileUnitDefaults(target);
      return DialogBox(win_drv_hInstance,
	MAKEINTRESOURCE(IDD_HARDFILE_CREATE),
	hwndDlg,
	wguiHardfileCreateDialogProc) == IDOK;
  }

  /*============================================================================*/
  /* Filesystem config                                                          */
  /*============================================================================*/

  /* Update filesystem description in the list view box */

  void wguiFilesystemUpdate(HWND lvHWND, cfg_filesys *fs, ULO i, BOOL add) {
    LV_ITEM lvi;
    STR stmp[48];

    memset(&lvi, 0, sizeof(lvi));
    lvi.mask = LVIF_TEXT;
    sprintf(stmp, "DH%u", i);
    lvi.iItem = i;
    lvi.pszText = stmp;
    lvi.cchTextMax = (int) strlen(stmp);
    lvi.iSubItem = 0;
    if (!add) ListView_SetItem(lvHWND, &lvi);
    else ListView_InsertItem(lvHWND, &lvi);
    lvi.pszText = fs->volumename;
    lvi.cchTextMax = (int) strlen(fs->volumename);
    lvi.iSubItem = 1;
    ListView_SetItem(lvHWND, &lvi);
    lvi.pszText = fs->rootpath;
    lvi.cchTextMax = (int) strlen(fs->rootpath);
    lvi.iSubItem = 2;
    ListView_SetItem(lvHWND, &lvi);
    sprintf(stmp, "%s", (fs->readonly) ? "R" : "RW");
    lvi.pszText = stmp;
    lvi.cchTextMax = (int) strlen(stmp);
    lvi.iSubItem = 3;
    ListView_SetItem(lvHWND, &lvi);
  }

  /* Install filesystem config */

#define FILESYSTEM_COLS 4
  void wguiInstallFilesystemConfig(HWND hwndDlg, cfg *conf) {
    LV_COLUMN lvc;
    HWND lvHWND = GetDlgItem(hwndDlg, IDC_LIST_FILESYSTEMS);
    ULO i, fscount;
    STR *colheads[FILESYSTEM_COLS] = {"Unit",
      "Volume",
      "Root Path",
      "RW"};

    /* Create list view control columns */

    memset(&lvc, 0, sizeof(lvc));
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    for (i = 0; i < FILESYSTEM_COLS; i++) {
      ULO colwidth = ListView_GetStringWidth(lvHWND, colheads[i]);
      if (i == 0) colwidth += 32;
      else if (i == 2) colwidth += 164;
      else colwidth += 16;
      lvc.pszText = colheads[i];
      lvc.cchTextMax = (int) strlen(colheads[i]);
      lvc.cx = colwidth;
      ListView_InsertColumn(lvHWND, i, &lvc);
    }

    /* Add current hardfiles to the list */

    fscount = cfgGetFilesystemCount(conf);
    ListView_SetItemCount(lvHWND, fscount);
    for (i = 0; i < fscount; i++) {
      cfg_filesys fs = cfgGetFilesystem(conf, i);
      wguiFilesystemUpdate(lvHWND, &fs, i, TRUE);
    }
    ListView_SetExtendedListViewStyle(lvHWND, LVS_EX_FULLROWSELECT);
    ccwButtonCheckConditional(hwndDlg, IDC_CHECK_AUTOMOUNT_FILESYSTEMS, cfgGetFilesystemAutomountDrives(conf));
  }

  /* Extract filesystem config */

  void wguiExtractFilesystemConfig(HWND hwndDlg, cfg *conf) {
    cfgSetFilesystemAutomountDrives(conf, ccwButtonGetCheck(hwndDlg, IDC_CHECK_AUTOMOUNT_FILESYSTEMS));
  }


  /* Execute filesystem add or edit data */

  cfg_filesys *wgui_current_filesystem_edit = NULL;
  ULO wgui_current_filesystem_edit_index = 0;

  /* Run a filesystem edit or add dialog */

  BOOLE wguiFilesystemAdd(HWND hwndDlg, 
    cfg *conf, 
    BOOLE add, 
    ULO index,
    cfg_filesys *target) {
      wgui_current_filesystem_edit = target;
      if (add) cfgSetFilesystemUnitDefaults(target);
      wgui_current_filesystem_edit_index = index;
      return DialogBox(win_drv_hInstance,
	MAKEINTRESOURCE(IDD_FILESYSTEM_ADD),
	hwndDlg,
	wguiFilesystemAddDialogProc) == IDOK;
  }

  /*============================================================================*/
  /* display config                                                              */
  /*============================================================================*/

  /* install display config */

  void wguiInstallDisplayConfig(HWND hwndDlg, cfg *conf)
  {
    HWND colorBitsComboboxHWND = GetDlgItem(hwndDlg, IDC_COMBO_COLOR_BITS);

    ULO comboboxid;

    // match available resolutions with configuration
    pwgui_dm_match = wguiMatchResolution();

    HDC desktopwindow_DC = GetWindowDC(GetDesktopWindow());
    int desktopwindow_bitspixel = GetDeviceCaps(desktopwindow_DC, BITSPIXEL);
    ReleaseDC(GetDesktopWindow(), desktopwindow_DC);

    // fill combobox for colorbit depth
    comboboxid = 0;
    if (pwgui_dm->numberof16bit > 0)
    { 
      ComboBox_AddString(colorBitsComboboxHWND, "high color (16 bit)"); 
      pwgui_dm->comboxbox16bitindex = comboboxid; comboboxid++; 
    }
    if (pwgui_dm->numberof24bit > 0)
    {
      ComboBox_AddString(colorBitsComboboxHWND, "true color (24 bit)"); 
      pwgui_dm->comboxbox24bitindex = comboboxid; comboboxid++; 
    }
    if (pwgui_dm->numberof32bit > 0)
    { 
      ComboBox_AddString(colorBitsComboboxHWND, "true color (32 bit)"); 
      pwgui_dm->comboxbox32bitindex = comboboxid; comboboxid++; 
    }

    ComboBox_Enable(colorBitsComboboxHWND, TRUE);
    ComboBox_SetCurSel(colorBitsComboboxHWND, wguiGetComboboxIndexFromColorBits(pwgui_dm_match->colorbits));

    // add multiple buffer option
    ccwButtonCheckConditional(hwndDlg, IDC_CHECK_MULTIPLE_BUFFERS, cfgGetUseMultipleGraphicalBuffers(conf));

    // set fullscreen button check
    if (pwgui_dm_match->windowed)
    {
      // windowed 
      // colorbits can't be selected through WinFellow, desktop setting will be used

      ComboBox_SetCurSel(colorBitsComboboxHWND, wguiGetComboboxIndexFromColorBits(desktopwindow_bitspixel));
      ComboBox_Enable(colorBitsComboboxHWND, FALSE);
      ccwButtonUncheck(hwndDlg, IDC_CHECK_FULLSCREEN);
      // disable multiplebuffers
      ccwButtonDisable(hwndDlg, IDC_CHECK_MULTIPLE_BUFFERS);
    }
    else
    {
      // fullscreen
      ComboBox_Enable(colorBitsComboboxHWND, TRUE);
      ccwButtonSetCheck(hwndDlg, IDC_CHECK_FULLSCREEN);

      if (desktopwindow_bitspixel == 8)
      {
	ccwButtonDisable(hwndDlg, IDC_CHECK_FULLSCREEN);
      }

      // enable the checkbox for multiplebuffers
      ccwButtonEnable(hwndDlg, IDC_CHECK_MULTIPLE_BUFFERS);
    }

    // add display scale option and scale strategy
    BOOLE isDisplaySize1x = cfgGetDisplayScale(conf) == DISPLAYSCALE_1X;
    ccwButtonCheckConditional(hwndDlg, IDC_RADIO_DISPLAYSIZE_1X, isDisplaySize1x);
    if (isDisplaySize1x == FALSE)
    {
      ccwButtonCheckConditional(hwndDlg, IDC_RADIO_DISPLAYSIZE_2X, TRUE);
    }

    BOOLE isDisplayStrategySolid = cfgGetDisplayScaleStrategy(conf) == DISPLAYSCALE_STRATEGY_SOLID;
    ccwButtonCheckConditional(hwndDlg, IDC_RADIO_LINE_FILL_SOLID, isDisplayStrategySolid);
    if (isDisplayStrategySolid == FALSE)
    {
      ccwButtonCheckConditional(hwndDlg, IDC_RADIO_LINE_FILL_SCANLINES, TRUE);
    }

    // add screen area 
    if (pwgui_dm_match->windowed)
    {
      // windowed
      ccwSliderSetRange(hwndDlg, IDC_SLIDER_SCREEN_AREA, 0, (pwgui_dm->numberofwindowed - 1));
    }
    else 
    {
      switch (pwgui_dm_match->colorbits)
      {
	case 16:
	  ccwSliderSetRange(hwndDlg, IDC_SLIDER_SCREEN_AREA, 0, (pwgui_dm->numberof16bit - 1));
	  break;
	case 24:
	  ccwSliderSetRange(hwndDlg, IDC_SLIDER_SCREEN_AREA, 0, (pwgui_dm->numberof24bit - 1));
	  break;
	case 32:
	  ccwSliderSetRange(hwndDlg, IDC_SLIDER_SCREEN_AREA, 0, (pwgui_dm->numberof32bit - 1));
	  break;
      }
    }
    ccwSliderSetPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA, pwgui_dm_match->id);
    wguiSetSliderTextAccordingToPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA, IDC_STATIC_SCREEN_AREA, &wguiGetResolutionStrWithIndex);

    // add frame skipping rate choices 
    ccwSliderSetRange(hwndDlg, IDC_SLIDER_FRAME_SKIPPING, 0, 24);
    ccwSliderSetPosition(hwndDlg, IDC_SLIDER_FRAME_SKIPPING, cfgGetFrameskipRatio(conf));
    wguiSetSliderTextAccordingToPosition(hwndDlg, IDC_SLIDER_FRAME_SKIPPING, IDC_STATIC_FRAME_SKIPPING, &wguiGetFrameSkippingStrWithIndex);

    // add blitter selection radio buttons
    wguiInstallBlitterConfig(hwndDlg, conf);
  }

  /* extract display config */

  void wguiExtractDisplayConfig(HWND hwndDlg, cfg *conf)
  {
    HWND colorBitsComboboxHWND = GetDlgItem(hwndDlg, IDC_COMBO_COLOR_BITS);

    // get current colorbits
    cfgSetScreenColorBits(conf, wguiGetColorBitsFromComboboxIndex(ccwComboBoxGetCurrentSelection(hwndDlg, IDC_COMBO_COLOR_BITS)));

    // get multiplebuffer check
    cfgSetUseMultipleGraphicalBuffers(conf, ccwButtonGetCheck(hwndDlg, IDC_CHECK_MULTIPLE_BUFFERS));

    // get fullscreen check
    cfgSetScreenWindowed(conf, !ccwButtonGetCheck(hwndDlg, IDC_CHECK_FULLSCREEN));

    // get scaling
    cfgSetDisplayScale(conf, (ccwButtonGetCheck(hwndDlg, IDC_RADIO_DISPLAYSIZE_1X)) ? DISPLAYSCALE_1X : DISPLAYSCALE_2X);
    cfgSetDisplayScaleStrategy(conf, (ccwButtonGetCheck(hwndDlg, IDC_RADIO_LINE_FILL_SOLID)) ? DISPLAYSCALE_STRATEGY_SOLID : DISPLAYSCALE_STRATEGY_SCANLINES);

    // get height and width
    if (cfgGetScreenWindowed(conf))
    {
      cfgSetScreenWidth(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->reswindowed, ccwSliderGetPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA))))->width);
      cfgSetScreenHeight(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->reswindowed, ccwSliderGetPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA))))->height);
    }
    else
    {
      switch(cfgGetScreenColorBits(conf))
      {
	case 16:
	  cfgSetScreenWidth(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res16bit, ccwSliderGetPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA))))->width);
	  cfgSetScreenHeight(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res16bit, ccwSliderGetPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA))))->height);
	  break;
	case 24:
	  cfgSetScreenWidth(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res24bit, ccwSliderGetPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA))))->width);
	  cfgSetScreenHeight(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res24bit, ccwSliderGetPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA))))->height);
	  break;
	case 32:
	  cfgSetScreenWidth(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res32bit, ccwSliderGetPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA))))->width);
	  cfgSetScreenHeight(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res32bit, ccwSliderGetPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA))))->height);
	  break;
      }
    }

    // get frame skipping rate choice
    cfgSetFrameskipRatio(conf, ccwSliderGetPosition(hwndDlg, IDC_SLIDER_FRAME_SKIPPING));

    // get blitter selection radio buttons
    wguiExtractBlitterConfig(hwndDlg, conf);
  }

  /*============================================================================*/
  /* List view selection investigate                                            */
  /*============================================================================*/

  LON wguiListViewNext(HWND ListHWND, ULO initialindex) {
    ULO itemcount = ListView_GetItemCount(ListHWND);
    ULO index = initialindex;

    while (index < itemcount)
      if (ListView_GetItemState(ListHWND, index, LVIS_SELECTED))
	return index;
      else index++;
      return -1;
  }

   
  /*===========================================================================*/
  /* Dialog Procedure for the Presets property sheet                            */
  /*============================================================================*/

  INT_PTR CALLBACK wguiPresetDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    switch (uMsg) 
    {
      case WM_INITDIALOG:
      {
        STR *strLastPresetROMDir, strAmigaForeverROMDir[CFG_FILENAME_LENGTH] = "";

        wgui_propsheetHWND[PROPSHEETPRESETS] = hwndDlg;
        strLastPresetROMDir = iniGetLastUsedPresetROMDir(wgui_ini);

        if(strncmp(strLastPresetROMDir, "", CFG_FILENAME_LENGTH) == 0)
        {
          // last preset directory not set
          if(fileopsResolveVariables("%AMIGAFOREVERDATA%Shared\\rom", strAmigaForeverROMDir))
          {
            strLastPresetROMDir = strAmigaForeverROMDir;
            iniSetLastUsedPresetROMDir(wgui_ini, strAmigaForeverROMDir);
          }
        }

        ccwEditSetText(hwndDlg, IDC_EDIT_PRESETS_ROMSEARCHPATH, strLastPresetROMDir);

        if(strLastPresetROMDir != NULL)
          if(strLastPresetROMDir[0] != '\0') {
            ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_MODEL);
            ccwButtonEnable(hwndDlg, IDC_COMBO_PRESETS_MODEL);
          }

        if(wgui_presets != NULL) {
          ULO i;          

          for(i=0; i<wgui_num_presets; i++)
            ccwComboBoxAddString(hwndDlg, IDC_COMBO_PRESETS_MODEL, wgui_presets[i].strPresetDescription);
        }

        return TRUE;
      }
      case WM_COMMAND:
        if(HIWORD(wParam) == BN_CLICKED) {
	  switch(LOWORD(wParam)) {
            case IDC_BUTTON_PRESETS_ROMSEARCHPATH:
              {
                STR strROMSearchPath[CFG_FILENAME_LENGTH] = "";

                if (wguiSelectDirectory(hwndDlg, strROMSearchPath, NULL, CFG_FILENAME_LENGTH, 
	          "Select ROM Directory:")) {
	          ccwEditSetText(hwndDlg, IDC_EDIT_PRESETS_ROMSEARCHPATH, strROMSearchPath);
                  iniSetLastUsedPresetROMDir(wgui_ini, strROMSearchPath);
                }
              }
              break;
            case IDC_BUTTON_PRESETS_APPLY:
              {
                ULO lIndex = 0;
                STR strFilename[CFG_FILENAME_LENGTH] = "";
                STR strKickstart[CFG_FILENAME_LENGTH] = "";
                STR strROMSearchPath[CFG_FILENAME_LENGTH] = "";

                lIndex = ccwComboBoxGetCurrentSelection(hwndDlg, IDC_COMBO_PRESETS_MODEL);

                strncpy(strFilename, wgui_presets[lIndex].strPresetFilename, CFG_FILENAME_LENGTH);

                fellowAddLog("Applying preset %s...\n", strFilename);

                if(cfgLoadFromFilename(wgui_cfg, strFilename, true)) {
                  ULO lCRC32 = 0;

                  if(lCRC32 = cfgGetKickCRC32(wgui_cfg)) {
                    ccwEditGetText(hwndDlg, IDC_EDIT_PRESETS_ROMSEARCHPATH, strROMSearchPath, CFG_FILENAME_LENGTH);
                    if(fileopsGetKickstartByCRC32(strROMSearchPath, lCRC32, strKickstart, CFG_FILENAME_LENGTH))
                    {
                      cfgSetKickImage(wgui_cfg, strKickstart);
                    }
                    else
                      fellowAddLog(" WARNING: could not locate ROM with checksum %X in %s.\n",
                        lCRC32, strROMSearchPath);
                  }

                  wguiInstallCPUConfig        (wgui_propsheetHWND[PROPSHEETCPU],        wgui_cfg);
                  wguiInstallFloppyConfig     (wgui_propsheetHWND[PROPSHEETFLOPPY],     wgui_cfg);
                  wguiInstallMemoryConfig     (wgui_propsheetHWND[PROPSHEETMEMORY],     wgui_cfg);
                  wguiInstallDisplayConfig    (wgui_propsheetHWND[PROPSHEETDISPLAY],    wgui_cfg);
                  wguiInstallSoundConfig      (wgui_propsheetHWND[PROPSHEETSOUND],      wgui_cfg);
                  // wguiInstallFilesystemConfig (wgui_propsheetHWND[PROPSHEETFILESYSTEM], wgui_cfg);
                  // wguiInstallHardfileConfig   (wgui_propsheetHWND[PROPSHEETHARDFILE],   wgui_cfg);
                  wguiInstallGameportConfig   (wgui_propsheetHWND[PROPSHEETGAMEPORT],   wgui_cfg);
                  wguiInstallVariousConfig    (wgui_propsheetHWND[PROPSHEETVARIOUS],    wgui_cfg);

                  fellowAddLog(" Preset applied successfully.\n");
                }
                else
                  fellowAddLog(" ERROR applying preset.\n");
              }
              break;
            default:
              break;
          }
        }
        else if(HIWORD(wParam) == CBN_SELENDOK) {
           switch (LOWORD(wParam)) {
            case IDC_COMBO_PRESETS_MODEL:
              {
                ULO index = 0;
                cfg *cfgTemp = NULL;
                STR strTemp[CFG_FILENAME_LENGTH] = "";

                index = ccwComboBoxGetCurrentSelection(hwndDlg, IDC_COMBO_PRESETS_MODEL);
#ifdef _DEBUG
                fellowAddLog("preset selected: %s\n", wgui_presets[index].strPresetFilename);
#endif

                ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_ROM);         ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_ROM_LABEL);
                ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_ROMLOCATION); ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_ROM_LOCATION_LABEL);
                ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_CHIPSET);     ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_CHIPSET_LABEL);
                ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_CPU);         ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_CPU_LABEL);
                ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_CHIPRAM);     ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_CHIPRAM_LABEL);
                ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_FASTRAM);     ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_FASTRAM_LABEL);
                ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_BOGORAM);     ccwButtonEnable(hwndDlg, IDC_LABEL_PRESETS_BOGORAM_LABEL);

                if(cfgTemp = cfgManagerGetNewConfig(&cfg_manager)) {
                  if(cfgLoadFromFilename(cfgTemp, wgui_presets[index].strPresetFilename, true)) {
                    STR strKickstart[CFG_FILENAME_LENGTH] = "";
                    STR strROMSearchPath[CFG_FILENAME_LENGTH] = "";
                    ULO lCRC32 = 0;

                    ccwEditSetText(hwndDlg, IDC_LABEL_PRESETS_CHIPSET, cfgGetECS(cfgTemp) ? "ECS": "OCS");

                    switch(cfgGetCPUType(cfgTemp))
                    {
                      case 0:
                        sprintf(strTemp, "68000"); break;
                      case 1:
                        sprintf(strTemp, "68010"); break;
                      case 2:
                        sprintf(strTemp, "68020"); break;
                      case 3:
                        sprintf(strTemp, "68030"); break;
                      case 4:
                        sprintf(strTemp, "68EC30"); break;
                      case 9:
                        sprintf(strTemp, "68EC20"); break;
                      default:
                        sprintf(strTemp, "unknown model");
                    }

                    ccwEditSetText(hwndDlg, IDC_LABEL_PRESETS_CPU, strTemp);
                    sprintf(strTemp, "%d bytes", cfgGetChipSize(cfgTemp));
                    ccwEditSetText(hwndDlg, IDC_LABEL_PRESETS_CHIPRAM, strTemp);
                    sprintf(strTemp, "%d bytes", cfgGetFastSize(cfgTemp));
                    ccwEditSetText(hwndDlg, IDC_LABEL_PRESETS_FASTRAM, strTemp);
                    sprintf(strTemp, "%d bytes", cfgGetBogoSize(cfgTemp));
                    ccwEditSetText(hwndDlg, IDC_LABEL_PRESETS_BOGORAM, strTemp);

                    if(lCRC32 = cfgGetKickCRC32(cfgTemp)) {
                      ccwEditGetText(hwndDlg, IDC_EDIT_PRESETS_ROMSEARCHPATH, strROMSearchPath, CFG_FILENAME_LENGTH);
                      if(fileopsGetKickstartByCRC32(strROMSearchPath, lCRC32, strKickstart, CFG_FILENAME_LENGTH))
                      {
                        cfgSetKickImage(cfgTemp, strKickstart);
                      }
                      else
                        fellowAddLog(" WARNING: could not locate ROM with checksum %X in %s.\n",
                          lCRC32, strROMSearchPath);
                    }

                    ccwEditSetText(hwndDlg, IDC_LABEL_PRESETS_ROM, cfgGetKickDescription(cfgTemp));
                    ccwEditSetText(hwndDlg, IDC_LABEL_PRESETS_ROMLOCATION, cfgGetKickImage(cfgTemp));

                    ccwButtonEnable(hwndDlg, IDC_BUTTON_PRESETS_APPLY);
                  }
                  cfgManagerFreeConfig(&cfg_manager, cfgTemp);
                }
              }
              break;
            default:
              break;
           }
        }
    case WM_DESTROY:
      break;
      }
      return FALSE;
  }


  /*============================================================================*/
  /* Dialog Procedure for the CPU property sheet                                */
  /*============================================================================*/

  INT_PTR CALLBACK wguiCPUDialogProc(HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) {
      switch (uMsg) {
    case WM_INITDIALOG:
      wgui_propsheetHWND[PROPSHEETCPU] = hwndDlg;
      wguiInstallCPUConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      break;
    case WM_DESTROY:
      wguiExtractCPUConfig(hwndDlg, wgui_cfg);
      break;
      }
      return FALSE;
  }


  /*============================================================================*/
  /* Dialog Procedure for the floppy property sheet                             */
  /*============================================================================*/

  void wguiSelectDiskImage(cfg *conf, HWND hwndDlg, int editIdentifier, ULO index) {
    STR filename[CFG_FILENAME_LENGTH];

    if (wguiSelectFile(hwndDlg, filename, CFG_FILENAME_LENGTH, "Select Diskimage", FSEL_ADF)) {
      cfgSetDiskImage(conf, index, filename);

      cfgSetLastUsedDiskDir(conf, wguiExtractPath(filename));
      iniSetLastUsedGlobalDiskDir(wgui_ini, wguiExtractPath(filename));

      ccwEditSetText(hwndDlg, editIdentifier, cfgGetDiskImage(conf, index));
    }
  }

  bool wguiCreateFloppyDiskImage(cfg *conf, HWND hwndDlg, ULO index) 
  {
    char *filename = NULL;
    
    filename = (char *) DialogBox(win_drv_hInstance,
	MAKEINTRESOURCE(IDD_FLOPPY_ADF_CREATE),
	hwndDlg,
	wguiFloppyCreateDialogProc);

    if(filename)
    {
      cfgSetDiskImage(conf, index, filename);
      cfgSetLastUsedDiskDir(conf, wguiExtractPath(filename));
      free(filename);
      return true;
    }
    return false;
  }

  INT_PTR CALLBACK wguiFloppyDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
  {
    switch (uMsg) {
    case WM_INITDIALOG:
      wgui_propsheetHWND[PROPSHEETFLOPPY] = hwndDlg;
      wguiInstallFloppyConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
	switch (LOWORD(wParam)) {
    case IDC_BUTTON_DF0_FILEDIALOG:
      wguiSelectDiskImage(wgui_cfg, hwndDlg, IDC_EDIT_DF0_IMAGENAME, 0);
      break;
    case IDC_BUTTON_DF1_FILEDIALOG:
      wguiSelectDiskImage(wgui_cfg, hwndDlg, IDC_EDIT_DF1_IMAGENAME, 1);
      break;
    case IDC_BUTTON_DF2_FILEDIALOG:
      wguiSelectDiskImage(wgui_cfg, hwndDlg, IDC_EDIT_DF2_IMAGENAME, 2);
      break;
    case IDC_BUTTON_DF3_FILEDIALOG:
      wguiSelectDiskImage(wgui_cfg, hwndDlg, IDC_EDIT_DF3_IMAGENAME, 3);
      break;
    case IDC_BUTTON_DF0_CREATE:
      wguiCreateFloppyDiskImage(wgui_cfg, hwndDlg, 0);
      ccwEditSetText(hwndDlg, IDC_EDIT_DF0_IMAGENAME, cfgGetDiskImage(wgui_cfg, 0));
      break;
    case IDC_BUTTON_DF1_CREATE:
      wguiCreateFloppyDiskImage(wgui_cfg, hwndDlg, 1);
      ccwEditSetText(hwndDlg, IDC_EDIT_DF1_IMAGENAME, cfgGetDiskImage(wgui_cfg, 1));
      break;
    case IDC_BUTTON_DF2_CREATE:
      wguiCreateFloppyDiskImage(wgui_cfg, hwndDlg, 2);
      ccwEditSetText(hwndDlg, IDC_EDIT_DF2_IMAGENAME, cfgGetDiskImage(wgui_cfg, 2));
      break;
    case IDC_BUTTON_DF3_CREATE:
      wguiCreateFloppyDiskImage(wgui_cfg, hwndDlg, 3);
      ccwEditSetText(hwndDlg, IDC_EDIT_DF3_IMAGENAME, cfgGetDiskImage(wgui_cfg, 3));
      break;
    case IDC_BUTTON_DF0_EJECT:
      cfgSetDiskImage(wgui_cfg, 0, "");
      floppySetDiskImage(0, "");
      ccwEditSetText(hwndDlg, IDC_EDIT_DF0_IMAGENAME, cfgGetDiskImage(wgui_cfg, 0));
      break;
    case IDC_BUTTON_DF1_EJECT:
      cfgSetDiskImage(wgui_cfg, 1, "");
      floppySetDiskImage(1, "");
      ccwEditSetText(hwndDlg, IDC_EDIT_DF1_IMAGENAME, cfgGetDiskImage(wgui_cfg, 1));
      break;
    case IDC_BUTTON_DF2_EJECT:
      cfgSetDiskImage(wgui_cfg, 2, "");
      floppySetDiskImage(2, "");
      ccwEditSetText(hwndDlg, IDC_EDIT_DF2_IMAGENAME, cfgGetDiskImage(wgui_cfg, 2));
      break;
    case IDC_BUTTON_DF3_EJECT:
      cfgSetDiskImage(wgui_cfg, 3, "");
      floppySetDiskImage(3, "");
      ccwEditSetText(hwndDlg, IDC_EDIT_DF3_IMAGENAME, cfgGetDiskImage(wgui_cfg, 3));
      break;
    default:
      break;
      }      
      break;
    case WM_DESTROY:
      wguiExtractFloppyConfig(hwndDlg, wgui_cfg);
      wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
      break;
    }
    return FALSE;
  }

  /**
   * dialog procedure for creation of floppy ADF images
   *
   * @return EndDialog is passed a pointer to the newly created floppy image, that must be freed by the caller
   */
  INT_PTR CALLBACK wguiFloppyCreateDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
  {
    switch (uMsg) {
      case WM_INITDIALOG:
        ccwEditSetText(hwndDlg, IDC_EDIT_FLOPPY_ADF_CREATE_VOLUME, "Empty");
        return TRUE;
      case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
	  switch (LOWORD(wParam)) {
            case IDOK:
              {
                char *strFilename;
                STR strVolume[CFG_FILENAME_LENGTH] = "";
                bool bResult = false;
                bool bFormat, bBootable = false, bFFS = false;

                strFilename = (char *) malloc(CFG_FILENAME_LENGTH + 1);

                ccwEditGetText(hwndDlg, IDC_EDIT_FLOPPY_ADF_CREATE_FILENAME, strFilename, CFG_FILENAME_LENGTH);
  
	        if(strFilename[0] == '\0') 
                {
	          MessageBox(hwndDlg,
	            "You must specify a floppy image filename.",
	            "Create floppy image",
	            MB_ICONERROR);
	          break;
	        }

                bFormat   = ccwButtonGetCheckBool(hwndDlg, IDC_CHECK_FLOPPY_ADF_CREATE_FORMAT);
                if(bFormat) 
                {
                  bBootable = ccwButtonGetCheckBool(hwndDlg, IDC_CHECK_FLOPPY_ADF_CREATE_BOOTABLE);
                  bFFS      = ccwButtonGetCheckBool(hwndDlg, IDC_CHECK_FLOPPY_ADF_CREATE_FFS);
                  ccwEditGetText(hwndDlg, IDC_EDIT_FLOPPY_ADF_CREATE_VOLUME, strVolume, CFG_FILENAME_LENGTH);

                  if(!floppyValidateAmigaDOSVolumeName(strVolume))
                  {
	            MessageBox(hwndDlg,
	              "The specified volume name is invalid.",
	              "Create floppy image",
	              MB_ICONERROR);
	            break;
	          }
                }

                bResult = floppyImageADFCreate(strFilename, strVolume, bFormat, bBootable, bFFS);

                if(bResult)
                  EndDialog(hwndDlg, (INT_PTR) strFilename);
                else {
                  if(strFilename) free(strFilename);
                  EndDialog(hwndDlg, NULL);
                }

                return FALSE;
              }
            case IDCANCEL:
               EndDialog(hwndDlg, NULL);
               return FALSE;
            case IDC_CHECK_FLOPPY_ADF_CREATE_FORMAT:
              {
                // (un)hide the elements that are needed for formatting
                bool bChecked = ccwButtonGetCheckBool(hwndDlg, IDC_CHECK_FLOPPY_ADF_CREATE_FORMAT);
                
                ccwButtonEnableConditional(hwndDlg, IDC_CHECK_FLOPPY_ADF_CREATE_BOOTABLE, bChecked);
                ccwButtonEnableConditional(hwndDlg, IDC_CHECK_FLOPPY_ADF_CREATE_FFS,      bChecked);
                ccwButtonEnableConditional(hwndDlg, IDC_LABEL_FLOPPY_ADF_CREATE_VOLUME,   bChecked);
                ccwEditEnableConditional  (hwndDlg, IDC_EDIT_FLOPPY_ADF_CREATE_VOLUME,    bChecked);
              }
              break;
            case IDC_FLOPPY_ADF_CREATE_SELECT:
              {
                STR strFilename[CFG_FILENAME_LENGTH] = "";

                if(wguiSaveFile(hwndDlg, strFilename, CFG_FILENAME_LENGTH, "Select disk image filename", FSEL_ADF)) {
                  ccwEditSetText(hwndDlg, IDC_EDIT_FLOPPY_ADF_CREATE_FILENAME, strFilename);
                }
              }
              break;
          }      
          break;
      case WM_DESTROY:
        break;
    }
    return FALSE;
  }

  /*============================================================================*/
  /* Dialog Procedure for the memory property sheet                             */
  /*============================================================================*/

  INT_PTR CALLBACK wguiMemoryDialogProc(HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) {
      STR filename[CFG_FILENAME_LENGTH];

      switch (uMsg) {
    case WM_INITDIALOG:
      wgui_propsheetHWND[PROPSHEETMEMORY] = hwndDlg;
      wguiInstallMemoryConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
	switch (LOWORD(wParam)) {
    case IDC_BUTTON_KICKSTART_FILEDIALOG:
      if (wguiSelectFile(hwndDlg, filename, CFG_FILENAME_LENGTH, "Select ROM File", FSEL_ROM)) {
	cfgSetKickImage(wgui_cfg, filename);
	iniSetLastUsedKickImageDir(wgui_ini, wguiExtractPath(filename));
	ccwEditSetText(hwndDlg, IDC_EDIT_KICKSTART, cfgGetKickImage(wgui_cfg));
      }
      break;
    case IDC_BUTTON_KEYFILE_FILEDIALOG:
      if (wguiSelectFile(hwndDlg, filename, CFG_FILENAME_LENGTH, "Select Keyfile", FSEL_KEY)) {
	cfgSetKey(wgui_cfg, filename);
	iniSetLastUsedKeyDir(wgui_ini, wguiExtractPath(filename));

	ccwEditSetText(hwndDlg, IDC_EDIT_KEYFILE, cfgGetKey(wgui_cfg));
      }
      break;
    default:
      break;
      }
      break;
    case WM_DESTROY:
      wguiExtractMemoryConfig(hwndDlg, wgui_cfg);
      break;
      }
      return FALSE;
  }

  /*============================================================================*/
  /* dialog procedure for the display property sheet                            */
  /*============================================================================*/

  ULO wguiGetNumberOfScreenAreas(ULO colorbits)
  {

    switch (colorbits)
    {
      case 16:
	return pwgui_dm->numberof16bit;
      case 24:
	return pwgui_dm->numberof24bit;
      case 32:
	return pwgui_dm->numberof32bit;
    }
    return pwgui_dm->numberof16bit;
  }

  INT_PTR CALLBACK wguiDisplayDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    ULO comboboxIndexColorBits;
    ULO selectedColorBits;

    switch (uMsg)
    {
      case WM_INITDIALOG:
        wgui_propsheetHWND[PROPSHEETDISPLAY] = hwndDlg;
        wguiInstallDisplayConfig(hwndDlg, wgui_cfg);
        return TRUE;
      case WM_COMMAND:
        switch ((int) LOWORD(wParam))
        {
          case IDC_CHECK_FULLSCREEN:
            switch (HIWORD(wParam))
            {
              case BN_CLICKED:
                if (!ccwButtonGetCheck(hwndDlg, IDC_CHECK_FULLSCREEN))
                {
	          // the checkbox was unchecked - going to windowed
	          ccwSliderSetRange(hwndDlg, IDC_SLIDER_SCREEN_AREA, 0, (pwgui_dm->numberofwindowed - 1));					
	          ccwSliderSetPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA, 0);
	          pwgui_dm_match = (wgui_drawmode *) listNode(pwgui_dm->reswindowed);
	          wguiSetSliderTextAccordingToPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA, IDC_STATIC_SCREEN_AREA, &wguiGetResolutionStrWithIndex);
	          ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_COMBO_COLOR_BITS), wguiGetComboboxIndexFromColorBits(GetDeviceCaps(GetWindowDC(GetDesktopWindow()), BITSPIXEL)));
	          ComboBox_Enable(GetDlgItem(hwndDlg, IDC_COMBO_COLOR_BITS), FALSE);
	          Button_Enable(GetDlgItem(hwndDlg, IDC_CHECK_MULTIPLE_BUFFERS), FALSE);
                } 
                else 
                {
	          // the checkbox was checked 
	          comboboxIndexColorBits = ccwComboBoxGetCurrentSelection(hwndDlg, IDC_COMBO_COLOR_BITS);
	          selectedColorBits = wguiGetColorBitsFromComboboxIndex(comboboxIndexColorBits);
	          ccwSliderSetPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA, 0);
	          ccwSliderSetRange(hwndDlg, IDC_SLIDER_SCREEN_AREA, 0, (wguiGetNumberOfScreenAreas(selectedColorBits) - 1));					
	          pwgui_dm_match = (wgui_drawmode *) listNode(wguiGetMatchingList(FALSE, wguiGetColorBitsFromComboboxIndex(comboboxIndexColorBits)));
	          wguiSetSliderTextAccordingToPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA, IDC_STATIC_SCREEN_AREA, &wguiGetResolutionStrWithIndex);
	          ComboBox_Enable(GetDlgItem(hwndDlg, IDC_COMBO_COLOR_BITS), TRUE);
	          Button_Enable(GetDlgItem(hwndDlg, IDC_CHECK_MULTIPLE_BUFFERS), TRUE);
                }
                break;
            }
            break;
          case IDC_COMBO_COLOR_BITS:
            switch (HIWORD(wParam))
            {
              case CBN_SELCHANGE:
                comboboxIndexColorBits = ccwComboBoxGetCurrentSelection(hwndDlg, IDC_COMBO_COLOR_BITS);
                selectedColorBits = wguiGetColorBitsFromComboboxIndex(comboboxIndexColorBits);
                ccwSliderSetPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA, 0);
                ccwSliderSetRange(hwndDlg, IDC_SLIDER_SCREEN_AREA, 0, (wguiGetNumberOfScreenAreas(selectedColorBits) - 1));					
                pwgui_dm_match = (wgui_drawmode *) listNode(wguiGetMatchingList(!ccwButtonGetCheck(hwndDlg, IDC_CHECK_FULLSCREEN), wguiGetColorBitsFromComboboxIndex(comboboxIndexColorBits)));
                wguiSetSliderTextAccordingToPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA, IDC_STATIC_SCREEN_AREA, &wguiGetResolutionStrWithIndex);
                break;
            }
            break;
          case IDC_RADIO_DISPLAYSIZE_1X:
            switch (HIWORD(wParam))
            {
              case BN_CLICKED:
                if (ccwButtonGetCheck(hwndDlg, IDC_RADIO_DISPLAYSIZE_1X))
                {
	          // 1x was checked
	          ccwButtonUncheck(hwndDlg, IDC_RADIO_DISPLAYSIZE_2X);
                } 
                break;
            }
            break;
          case IDC_RADIO_DISPLAYSIZE_2X:
            switch (HIWORD(wParam))
            {
              case BN_CLICKED:
                if (ccwButtonGetCheck(hwndDlg, IDC_RADIO_DISPLAYSIZE_2X))
                {
	          // 2x was checked
	          ccwButtonUncheck(hwndDlg, IDC_RADIO_DISPLAYSIZE_1X);
                } 
                break;
            }
            break;
          case IDC_RADIO_LINE_FILL_SOLID:
            switch (HIWORD(wParam))
            {
              case BN_CLICKED:
                if (ccwButtonGetCheck(hwndDlg, IDC_RADIO_LINE_FILL_SOLID))
                {
	          // solid was checked
	          ccwButtonUncheck(hwndDlg, IDC_RADIO_LINE_FILL_SCANLINES);
                } 
                break;
            }
            break;
          case IDC_RADIO_LINE_FILL_SCANLINES:
            switch (HIWORD(wParam))
            {
              case BN_CLICKED:
                if (ccwButtonGetCheck(hwndDlg, IDC_RADIO_LINE_FILL_SCANLINES))
                {
	          // scanlines was checked
	          ccwButtonUncheck(hwndDlg, IDC_RADIO_LINE_FILL_SOLID);
                } 
                break;
            }
            break;
        }
        break;
      case WM_NOTIFY:
        switch ((int) wParam)
        {
          case IDC_SLIDER_SCREEN_AREA:
            wguiSetSliderTextAccordingToPosition(hwndDlg, IDC_SLIDER_SCREEN_AREA, IDC_STATIC_SCREEN_AREA, &wguiGetResolutionStrWithIndex);
            break;
          case IDC_SLIDER_FRAME_SKIPPING:
            wguiSetSliderTextAccordingToPosition(hwndDlg, IDC_SLIDER_FRAME_SKIPPING, IDC_STATIC_FRAME_SKIPPING, &wguiGetFrameSkippingStrWithIndex);
            break;
        }
        break;
      case WM_DESTROY:
        wguiExtractDisplayConfig(hwndDlg, wgui_cfg);
        break;
    }
    return FALSE;
  }

  /*============================================================================*/
  /* Dialog Procedure for the blitter property sheet                            */
  /*============================================================================*/

  BOOL CALLBACK wguiBlitterDialogProc(HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) {
      switch (uMsg) {
    case WM_INITDIALOG:
      wguiInstallBlitterConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      break;
    case WM_DESTROY:
      wguiExtractBlitterConfig(hwndDlg, wgui_cfg);
      break;
      }
      return FALSE;
  }


  /*============================================================================*/
  /* Dialog Procedure for the sound property sheet                              */
  /*============================================================================*/

  INT_PTR CALLBACK wguiSoundDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    long buffer_length;
	long sound_volume;
    STR buffer[16];
	STR volume[16];

    switch (uMsg) {
    case WM_INITDIALOG:
      wguiInstallSoundConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      break;
    case WM_DESTROY:
      wguiExtractSoundConfig(hwndDlg, wgui_cfg);
      break;
    case WM_NOTIFY:
      buffer_length = (long) SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_SOUND_BUFFER_LENGTH), TBM_GETPOS, 0, 0);
      sprintf(buffer, "%d", buffer_length);
      ccwStaticSetText(hwndDlg, IDC_STATIC_BUFFER_LENGTH, buffer);

	  sound_volume = (long) SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_SOUND_VOLUME), TBM_GETPOS, 0, 0);
	  sprintf(volume, "%d %%", sound_volume);
	  ccwStaticSetText(hwndDlg, IDC_STATIC_SOUND_VOLUME, volume);
      break;
    }
    return FALSE;
  }


  /*============================================================================*/
  /* Dialog Procedure for the filesystem property sheet                         */
  /*============================================================================*/

  INT_PTR CALLBACK wguiFilesystemAddDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {
    case WM_INITDIALOG:
      {
	ccwEditSetText(hwndDlg, IDC_EDIT_FILESYSTEM_ADD_VOLUMENAME, wgui_current_filesystem_edit->volumename);
	ccwEditSetText(hwndDlg, IDC_EDIT_FILESYSTEM_ADD_ROOTPATH, wgui_current_filesystem_edit->rootpath);
	ccwButtonCheckConditional(hwndDlg, IDC_CHECK_FILESYSTEM_ADD_READONLY,	wgui_current_filesystem_edit->readonly);
      }
      return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
	switch (LOWORD(wParam)) {
    case IDC_BUTTON_FILESYSTEM_ADD_DIRDIALOG:
      if (wguiSelectDirectory(hwndDlg, wgui_current_filesystem_edit->rootpath,
	wgui_current_filesystem_edit->volumename, CFG_FILENAME_LENGTH, 
	"Select Filesystem Root Directory:")) {
	  ccwEditSetText(hwndDlg, IDC_EDIT_FILESYSTEM_ADD_ROOTPATH, wgui_current_filesystem_edit->rootpath);
	  ccwEditSetText(hwndDlg, IDC_EDIT_FILESYSTEM_ADD_VOLUMENAME, wgui_current_filesystem_edit->volumename);
      }
      break;
    case IDOK:
      {
	ccwEditGetText(hwndDlg, IDC_EDIT_FILESYSTEM_ADD_VOLUMENAME, wgui_current_filesystem_edit->volumename, 64);
	if (wgui_current_filesystem_edit->volumename[0] == '\0') {
	  MessageBox(hwndDlg,
	    "You must specify a volume name",
	    "Edit Filesystem",
	    0);
	  break;
	}
	ccwEditGetText(hwndDlg, IDC_EDIT_FILESYSTEM_ADD_ROOTPATH, wgui_current_filesystem_edit->rootpath, CFG_FILENAME_LENGTH);
	if (wgui_current_filesystem_edit->rootpath[0] == '\0') {
	  MessageBox(hwndDlg,
	    "You must specify a root path",
	    "Edit Filesystem",
	    0);
	  break;
	}
	wgui_current_filesystem_edit->readonly = ccwButtonGetCheck(hwndDlg, IDC_CHECK_FILESYSTEM_ADD_READONLY);
      }
    case IDCANCEL:
      EndDialog(hwndDlg, LOWORD(wParam));
      return TRUE;
      }
      break;
    }
    return FALSE;
  }


  INT_PTR CALLBACK wguiFilesystemDialogProc(HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) {
      switch (uMsg) {
    case WM_INITDIALOG:
      wgui_propsheetHWND[PROPSHEETFILESYSTEM] = hwndDlg;
      wguiInstallFilesystemConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
	switch (LOWORD(wParam)) {
    case IDC_BUTTON_FILESYSTEM_ADD:
      {  
	cfg_filesys fs;
	if (wguiFilesystemAdd(hwndDlg,
	  wgui_cfg,
	  TRUE,
	  cfgGetFilesystemCount(wgui_cfg),
	  &fs) == IDOK) {
	    wguiFilesystemUpdate(GetDlgItem(hwndDlg, IDC_LIST_FILESYSTEMS), 
	      &fs,
	      cfgGetFilesystemCount(wgui_cfg), 
	      TRUE);
	    cfgFilesystemAdd(wgui_cfg, &fs);
	}
      }
      break;
    case IDC_BUTTON_FILESYSTEM_EDIT:
      {
	ULO sel = wguiListViewNext(GetDlgItem(hwndDlg,
	  IDC_LIST_FILESYSTEMS),
	  0);
	if (sel != -1) {
	  cfg_filesys fs = cfgGetFilesystem(wgui_cfg, sel);
	  if (wguiFilesystemAdd(hwndDlg, 
	    wgui_cfg, 
	    FALSE, 
	    sel, 
	    &fs) == IDOK) {
	      cfgFilesystemChange(wgui_cfg, &fs, sel);
	      wguiFilesystemUpdate(GetDlgItem(hwndDlg,
		IDC_LIST_FILESYSTEMS), 
		&fs, 
		sel, 
		FALSE);
	  }
	}
      }
      break;
    case IDC_BUTTON_FILESYSTEM_REMOVE:
      { 
	LON sel = 0;
	while ((sel = wguiListViewNext(GetDlgItem(hwndDlg,
	  IDC_LIST_FILESYSTEMS),
	  sel)) != -1) {
	    ULO i;
	    cfgFilesystemRemove(wgui_cfg, sel);
	    ListView_DeleteItem(GetDlgItem(hwndDlg, IDC_LIST_FILESYSTEMS), 
	      sel);
	    for (i = sel; i < cfgGetFilesystemCount(wgui_cfg); i++) {
	      cfg_filesys fs = cfgGetFilesystem(wgui_cfg, i);
	      wguiFilesystemUpdate(GetDlgItem(hwndDlg,
		IDC_LIST_FILESYSTEMS), 
		&fs, 
		i, 
		FALSE);
	    }
	}
      }
      break;
    default:
      break;
      }
      break;
    case WM_DESTROY:
      wguiExtractFilesystemConfig(hwndDlg, wgui_cfg);
      break;
      }
      return FALSE;
  }


  /*============================================================================*/
  /* Dialog Procedure for the hardfile property sheet                           */
  /*============================================================================*/

  INT_PTR CALLBACK wguiHardfileCreateDialogProc(HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) {
      switch (uMsg) {
    case WM_INITDIALOG:
      ccwEditSetText(hwndDlg, IDC_CREATE_HARDFILE_NAME, "");
      ccwEditSetText(hwndDlg, IDC_CREATE_HARDFILE_SIZE, "0");
      return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
	switch (LOWORD(wParam)) {
    case IDOK:
      {
	STR stmp[32];
	fhfile_dev hfile;
	STR fname[CFG_FILENAME_LENGTH];

	ccwEditGetText(hwndDlg, IDC_CREATE_HARDFILE_NAME, hfile.filename, 256);

	strncpy(fname, hfile.filename, CFG_FILENAME_LENGTH);
	_strupr(fname);
	if (strrchr(fname, '.HDF') == NULL) {
	  if (strlen(hfile.filename) > 252) {
	    MessageBox(hwndDlg, "Hardfile name too long, maximum is 252 characters", "Create Hardfile", 0);
	    break;
	  }
	  strncat(hfile.filename, ".hdf",4);
	}


	if (hfile.filename[0] == '\0') {
	  MessageBox(hwndDlg, "You must specify a hardfile name", "Create Hardfile", 0);
	  break;
	}
	ccwEditGetText(hwndDlg, IDC_CREATE_HARDFILE_SIZE,	stmp, 32);
	hfile.size = atoi(stmp);

	if (ccwButtonGetCheckBool(hwndDlg, IDC_CHECK_CREATE_HARDFILE_MEGABYTES)) {
	  hfile.size = hfile.size * 1024 * 1024;
	}
	if ((hfile.size < 1) && (hfile.size > 4294967295)) {
	  MessageBox(hwndDlg, "Size must be between 1 byte and 4294967295 bytes", "Create Hardfile", 0);
	  break;
	}
	// creates the HDF file 
	fhfileCreate(hfile);
	strncpy(wgui_current_hardfile_edit->filename, hfile.filename, CFG_FILENAME_LENGTH);
      }
    case IDCANCEL:
      EndDialog(hwndDlg, LOWORD(wParam));
      return TRUE;

    case IDC_BUTTON_HARDFILE_CREATE_FILEDIALOG:
      if (wguiSaveFile(hwndDlg, wgui_current_hardfile_edit->filename, 
	CFG_FILENAME_LENGTH, "Select Hardfile Name", FSEL_HDF)) {
	  ccwEditSetText(hwndDlg, IDC_CREATE_HARDFILE_NAME, wgui_current_hardfile_edit->filename);
	  iniSetLastUsedHdfDir(wgui_ini, wguiExtractPath(wgui_current_hardfile_edit->filename));
      }
      break;

      }
      break;
      }
      return FALSE;
  }

  INT_PTR CALLBACK wguiHardfileAddDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    STR stmp[16];

    switch (uMsg) {
	case WM_INITDIALOG:
	  sprintf(stmp, "%u", wgui_current_hardfile_edit_index);
	  ccwStaticSetText(hwndDlg, IDC_STATIC_HARDFILE_ADD_UNIT, stmp);
	  ccwEditSetText(hwndDlg, IDC_EDIT_HARDFILE_ADD_FILENAME, wgui_current_hardfile_edit->filename);
	  sprintf(stmp, "%u", wgui_current_hardfile_edit->sectorspertrack);
	  ccwEditSetText(hwndDlg, IDC_EDIT_HARDFILE_ADD_SECTORS, stmp);
	  sprintf(stmp, "%u", wgui_current_hardfile_edit->surfaces);
	  ccwEditSetText(hwndDlg, IDC_EDIT_HARDFILE_ADD_SURFACES, stmp);
	  sprintf(stmp, "%u", wgui_current_hardfile_edit->reservedblocks);
	  ccwEditSetText(hwndDlg, IDC_EDIT_HARDFILE_ADD_RESERVED, stmp);
	  sprintf(stmp, "%u", wgui_current_hardfile_edit->bytespersector);
	  ccwEditSetText(hwndDlg, IDC_EDIT_HARDFILE_ADD_BYTES_PER_SECTOR, stmp);
	  return TRUE;
	case WM_COMMAND:
	  if (HIWORD(wParam) == BN_CLICKED)
	    switch (LOWORD(wParam)) {
	case IDC_BUTTON_HARDFILE_ADD_FILEDIALOG:
	  if (wguiSelectFile(hwndDlg, wgui_current_hardfile_edit->filename, 
	    CFG_FILENAME_LENGTH, "Select Hardfile", FSEL_HDF)) {
	      ccwEditSetText(hwndDlg, IDC_EDIT_HARDFILE_ADD_FILENAME, wgui_current_hardfile_edit->filename);
	      iniSetLastUsedHdfDir(wgui_ini, wguiExtractPath(wgui_current_hardfile_edit->filename));
	  }
	  break;
	case IDOK:
	  {
	    STR stmp[32];
	    ccwEditGetText(hwndDlg, IDC_EDIT_HARDFILE_ADD_FILENAME, wgui_current_hardfile_edit->filename, 256);
	    if (wgui_current_hardfile_edit->filename[0] == '\0') {
	      MessageBox(hwndDlg,
		"You must specify a hardfile name",
		"Edit Hardfile",
		0);
	      break;
	    }
	    ccwEditGetText(hwndDlg, IDC_EDIT_HARDFILE_ADD_SECTORS, stmp, 32);
	    if (atoi(stmp) < 1) {
	      MessageBox(hwndDlg,
		"Sectors Per Track must be 1 or higher",
		"Edit Hardfile",
		0);
	      break;
	    }
	    wgui_current_hardfile_edit->sectorspertrack = atoi(stmp);
	    ccwEditGetText(hwndDlg, IDC_EDIT_HARDFILE_ADD_SURFACES, stmp, 32);
	    if (atoi(stmp) < 1) {
	      MessageBox(hwndDlg,
		"The number of surfaces must be 1 or higher",
		"Edit Hardfile",
		0);
	      break;
	    }
	    wgui_current_hardfile_edit->surfaces = atoi(stmp);
	    ccwEditGetText(hwndDlg, IDC_EDIT_HARDFILE_ADD_RESERVED, stmp, 32);
	    if (atoi(stmp) < 1) {
	      MessageBox(hwndDlg,
		"The number of reserved blocks must be 1 or higher",
		"Edit Hardfile",
		0);
	      break;
	    }
	    wgui_current_hardfile_edit->reservedblocks = atoi((char *) stmp);
	    ccwEditGetText(hwndDlg, IDC_EDIT_HARDFILE_ADD_BYTES_PER_SECTOR, stmp, 32);
	    wgui_current_hardfile_edit->bytespersector = atoi((char *) stmp);
	  }
	case IDCANCEL:
	  EndDialog(hwndDlg, LOWORD(wParam));
	  return TRUE;
	  }
	  break;
    }
    return FALSE;
  }


  INT_PTR CALLBACK wguiHardfileDialogProc(HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) {
      switch (uMsg) {
    case WM_INITDIALOG:
      wgui_propsheetHWND[PROPSHEETHARDFILE] = hwndDlg;
      wguiInstallHardfileConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
	switch (LOWORD(wParam)) {
    case  IDC_BUTTON_HARDFILE_CREATE:
      {  
	cfg_hardfile fhd;
	if (wguiHardfileCreate(hwndDlg, wgui_cfg, cfgGetHardfileCount(wgui_cfg), &fhd) == IDOK) {
	  wguiHardfileUpdate(GetDlgItem(hwndDlg, IDC_LIST_HARDFILES), 
	    &fhd,
	    cfgGetHardfileCount(wgui_cfg), 
	    TRUE);
	  cfgHardfileAdd(wgui_cfg, &fhd);
	}
      }
      break;
    case IDC_BUTTON_HARDFILE_ADD:
      {  
	cfg_hardfile fhd;
	if (wguiHardfileAdd(hwndDlg,
	  wgui_cfg,
	  TRUE,
	  cfgGetHardfileCount(wgui_cfg),
	  &fhd) == IDOK) {
	    wguiHardfileUpdate(GetDlgItem(hwndDlg, IDC_LIST_HARDFILES), 
	      &fhd,
	      cfgGetHardfileCount(wgui_cfg), 
	      TRUE);
	    cfgHardfileAdd(wgui_cfg, &fhd);
	}
      }
      break;
    case IDC_BUTTON_HARDFILE_EDIT:
      {
	ULO sel = wguiListViewNext(GetDlgItem(hwndDlg,
	  IDC_LIST_HARDFILES),
	  0);
	if (sel != -1) {
	  cfg_hardfile fhd = cfgGetHardfile(wgui_cfg, sel);
	  if (wguiHardfileAdd(hwndDlg, 
	    wgui_cfg, 
	    FALSE, 
	    sel, 
	    &fhd) == IDOK) {
	      cfgHardfileChange(wgui_cfg, &fhd, sel);
	      wguiHardfileUpdate(GetDlgItem(hwndDlg, IDC_LIST_HARDFILES), 
		&fhd, 
		sel, 
		FALSE);
	  }
	}
      }
      break;
    case IDC_BUTTON_HARDFILE_REMOVE:
      { 
	LON sel = 0;
	while ((sel = wguiListViewNext(GetDlgItem(hwndDlg,
	  IDC_LIST_HARDFILES),
	  sel)) != -1) {
	    ULO i;
	    cfgHardfileRemove(wgui_cfg, sel);
	    ListView_DeleteItem(GetDlgItem(hwndDlg, IDC_LIST_HARDFILES), 
	      sel);
	    for (i = sel; i < cfgGetHardfileCount(wgui_cfg); i++) {
	      cfg_hardfile fhd = cfgGetHardfile(wgui_cfg, i);
	      wguiHardfileUpdate(GetDlgItem(hwndDlg, IDC_LIST_HARDFILES), 
		&fhd, 
		i, 
		FALSE);
	    }
	}
      }
      break;
    default:
      break;
      }
      break;
    case WM_DESTROY:
      wguiExtractHardfileConfig(hwndDlg, wgui_cfg);
      break;
      }
      return FALSE;
  }


  /*============================================================================*/
  /* Dialog Procedure for the gameport property sheet                           */
  /*============================================================================*/

  INT_PTR CALLBACK wguiGameportDialogProc(HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) {
      HWND gpChoice[2];

      gpChoice[0] = GetDlgItem(hwndDlg, IDC_COMBO_GAMEPORT1);
      gpChoice[1] = GetDlgItem(hwndDlg, IDC_COMBO_GAMEPORT2);

      switch (uMsg) {
    case WM_INITDIALOG:
      wgui_propsheetHWND[PROPSHEETGAMEPORT] = hwndDlg;
      wguiInstallGameportConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      if (wgui_action == WGUI_NO_ACTION)
      {
	HWND gpChoice[2];

	gpChoice[0] = GetDlgItem(hwndDlg, IDC_COMBO_GAMEPORT1);
	gpChoice[1] = GetDlgItem(hwndDlg, IDC_COMBO_GAMEPORT2);

	switch (LOWORD(wParam)) {
    case IDC_COMBO_GAMEPORT1:
      if (HIWORD(wParam) == CBN_SELCHANGE) {
	if (ComboBox_GetCurSel(gpChoice[0]) == ComboBox_GetCurSel(gpChoice[1])) {
	  ComboBox_SetCurSel(gpChoice[1], 0);
	}
      }
      break;
    case IDC_COMBO_GAMEPORT2:
      if (HIWORD(wParam) == CBN_SELCHANGE) {
	if (ComboBox_GetCurSel(gpChoice[0]) == ComboBox_GetCurSel(gpChoice[1])) {
	  ComboBox_SetCurSel(gpChoice[0], 0);
	}
      }
      break;
	}
      }
      break;
    case WM_DESTROY:
      wguiExtractGameportConfig(hwndDlg, wgui_cfg);
      break;
      }
      return FALSE;
  }


  /*============================================================================*/
  /* Dialog Procedure for the various property sheet                            */
  /*============================================================================*/

  INT_PTR CALLBACK wguiVariousDialogProc(HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
  {
    switch (uMsg) {
    case WM_INITDIALOG:
      wgui_propsheetHWND[PROPSHEETVARIOUS] = hwndDlg;
      wguiInstallVariousConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      break;
    case WM_DESTROY:
      wguiExtractVariousConfig(hwndDlg, wgui_cfg);
      break;
    }
    return FALSE;
  }

  /*============================================================================*/
  /* Creates the configuration dialog                                           */
  /* Does this by first initializing the array of PROPSHEETPAGE structs to      */
  /* describe each property sheet in the dialog.                                */
  /* This process also creates and runs the dialog.                             */
  /*============================================================================*/

  INT_PTR wguiConfigurationDialog()
  {
    int i;
    PROPSHEETPAGE propertysheets[PROP_SHEETS];
    PROPSHEETHEADER propertysheetheader;

    for (i = 0; i < PROP_SHEETS; i++)  {
      propertysheets[i].dwSize = sizeof(PROPSHEETPAGE);
      if (wgui_propsheetICON[i] != 0) {
	propertysheets[i].dwFlags = PSP_USEHICON;
	propertysheets[i].hIcon = LoadIcon(win_drv_hInstance, MAKEINTRESOURCE(wgui_propsheetICON[i]));
      } else {
	propertysheets[i].dwFlags = PSP_DEFAULT;
	propertysheets[i].hIcon = 0;
      }
      propertysheets[i].hInstance = win_drv_hInstance;
      propertysheets[i].pszTemplate = MAKEINTRESOURCE(wgui_propsheetRID[i]);
      propertysheets[i].pszTitle = NULL;
      propertysheets[i].pfnDlgProc = wgui_propsheetDialogProc[i];
      propertysheets[i].lParam = 0;
      propertysheets[i].pfnCallback = NULL;
      propertysheets[i].pcRefParent = NULL;
    }
    propertysheetheader.dwSize = sizeof(PROPSHEETHEADER);
    propertysheetheader.dwFlags = PSH_PROPSHEETPAGE;
    propertysheetheader.hwndParent = wgui_hDialog;
    propertysheetheader.hInstance = win_drv_hInstance;
    propertysheetheader.hIcon = LoadIcon(win_drv_hInstance, MAKEINTRESOURCE(IDI_ICON_WINFELLOW));
    propertysheetheader.pszCaption = "WinFellow Configuration";
    propertysheetheader.nPages = PROP_SHEETS;
    // propertysheetheader.nStartPage = iniGetLastUsedCfgTab(wgui_ini);
    propertysheetheader.nStartPage = 4;
    propertysheetheader.ppsp = propertysheets;
    propertysheetheader.pfnCallback = NULL;
    return PropertySheet(&propertysheetheader);
  }

  void wguiSetCheckOfUseMultipleGraphicalBuffers(BOOLE useMultipleGraphicalBuffers) {

    if (useMultipleGraphicalBuffers) {
      CheckMenuItem(GetMenu(wgui_hDialog), ID_OPTIONS_MULTIPLE_GRAPHICAL_BUFFERS, MF_CHECKED);
    } else {
      CheckMenuItem(GetMenu(wgui_hDialog), ID_OPTIONS_MULTIPLE_GRAPHICAL_BUFFERS, MF_UNCHECKED);
    }	
  }

  /*============================================================================*/
  /* DialogProc for the About box                                               */
  /*============================================================================*/

  INT_PTR CALLBACK wguiAboutDialogProc(HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) {
      switch (uMsg) {
    case WM_INITDIALOG:
      {
	char *versionstring = fellowGetVersionString();
	if(versionstring)
	{
	  ccwStaticSetText(hwndDlg, IDC_STATIC_ABOUT_VERSION, versionstring);
	  free(versionstring);
	}
      }
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
    case IDCANCEL:
    case IDOK:
      EndDialog(hwndDlg, LOWORD(wParam));
      return TRUE;
      break;
    case IDC_STATIC_LINK:
      SetTextColor((HDC) LOWORD(lParam), RGB(0, 0, 255));
      ShellExecute(NULL, "open", "http://fellow.sourceforge.net", NULL,NULL, SW_SHOWNORMAL);
      break;  
    default:
      break;
      }
      }
      return FALSE;
  }


  /*============================================================================*/
  /* About box                                                                  */
  /*============================================================================*/

  void wguiAbout(HWND hwndDlg) {
    DialogBox(win_drv_hInstance,
      MAKEINTRESOURCE(IDD_ABOUT),
      hwndDlg,
      wguiAboutDialogProc);
  }


  /*============================================================================*/
  /* DialogProc for our main Dialog                                             */
  /*============================================================================*/

  INT_PTR CALLBACK wguiDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    STR l_diskimage[CFG_FILENAME_LENGTH];
    char *versionstring;

    switch (uMsg) {
	case WM_INITDIALOG:
	  SendMessage(hwndDlg, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) LoadIcon(win_drv_hInstance, MAKEINTRESOURCE(IDI_ICON_WINFELLOW)));

	  if(versionstring = fellowGetVersionString()) 
	  {
	    SetWindowText(hwndDlg, versionstring);
	    free(versionstring);
	  }
	  return TRUE;

	case WM_COMMAND:
	  if (wgui_action == WGUI_NO_ACTION)
	    switch (LOWORD(wParam)) {
	case IDC_START_EMULATION:
	  wgui_emulation_state = TRUE;
	  wgui_action = WGUI_START_EMULATION;
	  break;
	case IDCANCEL:
	case ID_FILE_QUIT:
	case IDC_QUIT_EMULATOR:
	  wgui_action = WGUI_QUIT_EMULATOR;
	  break;
	case ID_FILE_OPENCONFIGURATION:
	  wgui_action = WGUI_OPEN_CONFIGURATION;
	  break;
	case ID_FILE_SAVECONFIGURATION:
	  wgui_action = WGUI_SAVE_CONFIGURATION;
	  break;
	case ID_FILE_SAVECONFIGURATIONAS:
	  wgui_action = WGUI_SAVE_CONFIGURATION_AS;
	  break;
	case ID_FILE_LOAD_STATE:
	  wgui_action = WGUI_LOAD_STATE;
	  break;
	case ID_FILE_SAVE_STATE:
	  wgui_action = WGUI_SAVE_STATE;
	  break;
	case ID_FILE_HISTORYCONFIGURATION0:   
	  wgui_action = WGUI_LOAD_HISTORY0;
	  break;
	case ID_FILE_HISTORYCONFIGURATION1:
	  wgui_action = WGUI_LOAD_HISTORY1;
	  break;
	case ID_FILE_HISTORYCONFIGURATION2:   
	  wgui_action = WGUI_LOAD_HISTORY2;
	  break;
	case ID_FILE_HISTORYCONFIGURATION3:   
	  wgui_action = WGUI_LOAD_HISTORY3;
	  break;
	case IDC_CONFIGURATION:
          {
            cfg *configbackup;
            INT_PTR result;

            configbackup = cfgManagerGetCopyOfCurrentConfig(&cfg_manager);
 
            result = wguiConfigurationDialog();
            if(result >= 1)
            {
              cfgManagerFreeConfig(&cfg_manager, configbackup);
              cfgSetConfigChangedSinceLastSave(wgui_cfg, TRUE);
            }
            else 
            { // discard changes, as user clicked cancel or error occurred
              cfgManagerFreeConfig(&cfg_manager, wgui_cfg);
              cfgManagerSetCurrentConfig(&cfg_manager, configbackup);
              wgui_cfg = configbackup;
            }

            break;
          }
	case IDC_HARD_RESET:
	  fellowPreStartReset(TRUE);
	  wguiLoadBitmaps();
	  SendMessage(GetDlgItem(wgui_hDialog, IDC_IMAGE_POWER_LED_MAIN), 
	    STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) power_led_off_bitmap);
	  wgui_emulation_state = FALSE;
	  break;
	case ID_DEBUGGER_START:
	  wgui_action = WGUI_DEBUGGER_START;
	  break;
	case ID_HELP_ABOUT:
	  wgui_action = WGUI_ABOUT;
	  wguiAbout(hwndDlg);
	  wgui_action = WGUI_NO_ACTION;
	  break;
	default:
	  break;
	  }
	  if (HIWORD(wParam) == BN_CLICKED)
	    switch (LOWORD(wParam)) {
	case IDC_BUTTON_DF0_FILEDIALOG_MAIN:
	  wguiSelectDiskImage(wgui_cfg,	hwndDlg, IDC_EDIT_DF0_IMAGENAME_MAIN, 0);
	  break;
	case IDC_BUTTON_DF1_FILEDIALOG_MAIN:
	  wguiSelectDiskImage(wgui_cfg,	hwndDlg, IDC_EDIT_DF1_IMAGENAME_MAIN,	1);
	  break;
	case IDC_BUTTON_DF2_FILEDIALOG_MAIN:
	  wguiSelectDiskImage(wgui_cfg,	hwndDlg, IDC_EDIT_DF2_IMAGENAME_MAIN,	2);
	  break;
	case IDC_BUTTON_DF3_FILEDIALOG_MAIN:
	  wguiSelectDiskImage(wgui_cfg,	hwndDlg, IDC_EDIT_DF3_IMAGENAME_MAIN, 3);
	  break;
	case IDC_BUTTON_DF0_EJECT_MAIN:
	  cfgSetDiskImage(wgui_cfg, 0, "");
	  ccwEditSetText(hwndDlg, IDC_EDIT_DF0_IMAGENAME_MAIN, cfgGetDiskImage(wgui_cfg, 0));
	  break;
	case IDC_BUTTON_DF1_EJECT_MAIN:
	  cfgSetDiskImage(wgui_cfg, 1, "");
	  ccwEditSetText(hwndDlg, IDC_EDIT_DF1_IMAGENAME_MAIN, cfgGetDiskImage(wgui_cfg, 1));
	  break;
	case IDC_BUTTON_DF2_EJECT_MAIN:
	  cfgSetDiskImage(wgui_cfg, 2, "");
	  ccwEditSetText(hwndDlg, IDC_EDIT_DF2_IMAGENAME_MAIN, cfgGetDiskImage(wgui_cfg, 2));
	  break;
	case IDC_BUTTON_DF3_EJECT_MAIN:
	  cfgSetDiskImage(wgui_cfg, 3, "");
	  ccwEditSetText(hwndDlg, IDC_EDIT_DF3_IMAGENAME_MAIN, cfgGetDiskImage(wgui_cfg, 3));
	  break;
	case IDC_BUTTON_DF1_SWAP:
	  strcpy(l_diskimage, cfgGetDiskImage(wgui_cfg, 0));
	  cfgSetDiskImage(wgui_cfg, 0, cfgGetDiskImage(wgui_cfg, 1));
	  cfgSetDiskImage(wgui_cfg, 1, l_diskimage);
	  ccwEditSetText(hwndDlg, IDC_EDIT_DF0_IMAGENAME_MAIN, cfgGetDiskImage(wgui_cfg, 0));
	  ccwEditSetText(hwndDlg, IDC_EDIT_DF1_IMAGENAME_MAIN, cfgGetDiskImage(wgui_cfg, 1));
	  break;	
	case IDC_BUTTON_DF2_SWAP:
	  strcpy(l_diskimage, cfgGetDiskImage(wgui_cfg, 0));
	  cfgSetDiskImage(wgui_cfg, 0, cfgGetDiskImage(wgui_cfg, 2));
	  cfgSetDiskImage(wgui_cfg, 2, l_diskimage);
	  ccwEditSetText(hwndDlg, IDC_EDIT_DF0_IMAGENAME_MAIN, cfgGetDiskImage(wgui_cfg, 0));
	  ccwEditSetText(hwndDlg, IDC_EDIT_DF2_IMAGENAME_MAIN, cfgGetDiskImage(wgui_cfg, 2));
	  break;	
	case IDC_BUTTON_DF3_SWAP:
	  strcpy(l_diskimage, cfgGetDiskImage(wgui_cfg, 0));
	  cfgSetDiskImage(wgui_cfg, 0, cfgGetDiskImage(wgui_cfg, 3));
	  cfgSetDiskImage(wgui_cfg, 3, l_diskimage);
	  ccwEditSetText(hwndDlg, IDC_EDIT_DF0_IMAGENAME_MAIN, cfgGetDiskImage(wgui_cfg, 0));
	  ccwEditSetText(hwndDlg, IDC_EDIT_DF3_IMAGENAME_MAIN, cfgGetDiskImage(wgui_cfg, 3));
	  break;
	case IDC_BUTTON_TURBO_LOAD:
	  if (cfgGetSoundEmulation(wgui_cfg) != 1)
	  {
	    cfgSetSoundEmulation(wgui_cfg, SOUND_PLAY);
	  }
	  else
	  {
	    cfgSetSoundEmulation(wgui_cfg, SOUND_EMULATE);
	  }
	  break;
	default:
	  break;
	  }
    }
    return FALSE;
  }


  /*============================================================================*/
  /* The functions below this line are for "public" use by other modules        */
  /*============================================================================*/


  /*============================================================================*/
  /* Shows a message box                                                        */
  /*============================================================================*/

  void wguiRequester(STR *line1, STR *line2, STR *line3) {
    STR text[1024];

    sprintf(text, "%s\n%s\n%s", line1, line2, line3);
    MessageBox(NULL, text, "WinFellow Amiga Emulator", 0);
  }


  /*============================================================================*/
  /* Runs the GUI                                                               */
  /*============================================================================*/

  BOOLE wguiCheckEmulationNecessities(void) {
    if(strcmp(cfgGetKickImage(wgui_cfg), "") != 0) {
      FILE *F = fopen(cfgGetKickImage(wgui_cfg), "rb");
      if (F != NULL)
      {
	fclose(F);
	return TRUE;
      }
      return FALSE;
    }
    else return FALSE;
  }	

  BOOLE wguiEnter(void)
  {
    BOOLE quit_emulator = FALSE;
    BOOLE debugger_start = FALSE;
    RECT dialogRect;

    do
    {
      MSG myMsg;
      BOOLE end_loop = FALSE;

      wgui_action = WGUI_NO_ACTION;

      wgui_hDialog = CreateDialog(win_drv_hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, wguiDialogProc); 
      SetWindowPos(wgui_hDialog, HWND_TOP, iniGetMainWindowXPos(wgui_ini), iniGetMainWindowYPos(wgui_ini), 0, 0, SWP_NOSIZE | SWP_ASYNCWINDOWPOS);
      wguiStartupPost();
      wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);


      // install history into menu
      wguiInstallHistoryIntoMenu();
      ShowWindow(wgui_hDialog, win_drv_nCmdShow);

      while (!end_loop)
      {
	if (GetMessage(&myMsg, wgui_hDialog, 0, 0))
	{
	  if (!IsDialogMessage(wgui_hDialog, &myMsg))
	  {
	    DispatchMessage(&myMsg);
	  }
	}
	switch (wgui_action)
	{
	  case WGUI_START_EMULATION:
            wguiCheckMemorySettingsForChipset();
	    if (wguiCheckEmulationNecessities() == TRUE)
	    {
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
            {
              BOOLE bQuit = TRUE;

              if(cfgGetConfigChangedSinceLastSave(wgui_cfg)) {
                int result;

                result = MessageBox(wgui_hDialog, 
                  "There are unsaved configuration changes, quit anyway?", 
                  "WinFellow",
                MB_YESNO | MB_ICONEXCLAMATION);
                if(result == IDNO)
                  bQuit = FALSE;
              }
              if(bQuit)
	            {
                end_loop = TRUE;
	              quit_emulator = TRUE;
              }
              else
                wgui_action = WGUI_NO_ACTION;
            }
	    break;
	  case WGUI_OPEN_CONFIGURATION:
	    wguiOpenConfigurationFile(wgui_cfg, wgui_hDialog);
            cfgSetConfigChangedSinceLastSave(wgui_cfg, FALSE);
            wguiCheckMemorySettingsForChipset();
            wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
	    wgui_action = WGUI_NO_ACTION;
	    break;
	  case WGUI_SAVE_CONFIGURATION:
	    cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
            cfgSetConfigChangedSinceLastSave(wgui_cfg, FALSE);
	    wgui_action = WGUI_NO_ACTION;
	    break;
	  case WGUI_SAVE_CONFIGURATION_AS:
	    wguiSaveConfigurationFileAs(wgui_cfg, wgui_hDialog);
            cfgSetConfigChangedSinceLastSave(wgui_cfg, FALSE);
	    wguiInsertCfgIntoHistory(iniGetCurrentConfigurationFilename(wgui_ini));
	    wgui_action = WGUI_NO_ACTION;
	    break;
	  case WGUI_LOAD_HISTORY0:
	    //cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
	    if(cfgLoadFromFilename(wgui_cfg, iniGetConfigurationHistoryFilename(wgui_ini, 0), false) == FALSE) 
	    {
	      wguiDeleteCfgFromHistory(0);
	    } 
	    else 
	    {
	      iniSetCurrentConfigurationFilename(wgui_ini, iniGetConfigurationHistoryFilename(wgui_ini, 0));
              cfgSetConfigChangedSinceLastSave(wgui_cfg, FALSE);
              wguiCheckMemorySettingsForChipset();
	    }
	    wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
	    wgui_action = WGUI_NO_ACTION;
	    break;
	  case WGUI_LOAD_HISTORY1:
	    //cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
	    if(cfgLoadFromFilename(wgui_cfg, iniGetConfigurationHistoryFilename(wgui_ini, 1), false) == FALSE) 
	    {
	      wguiDeleteCfgFromHistory(1);
	    } 
	    else 
	    {
	      iniSetCurrentConfigurationFilename(wgui_ini, iniGetConfigurationHistoryFilename(wgui_ini, 1));
              cfgSetConfigChangedSinceLastSave(wgui_cfg, FALSE);
              wguiCheckMemorySettingsForChipset();
	      wguiPutCfgInHistoryOnTop(1);
	    } 
	    wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
	    wgui_action = WGUI_NO_ACTION;
	    break;
	  case WGUI_LOAD_HISTORY2:
	    //cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
	    if(cfgLoadFromFilename(wgui_cfg, iniGetConfigurationHistoryFilename(wgui_ini, 2), false) == FALSE) 
	    {
	      wguiDeleteCfgFromHistory(2);
	    } 
	    else 
	    {
	      iniSetCurrentConfigurationFilename(wgui_ini, iniGetConfigurationHistoryFilename(wgui_ini, 2));
              cfgSetConfigChangedSinceLastSave(wgui_cfg, FALSE);
              wguiCheckMemorySettingsForChipset();
	      wguiPutCfgInHistoryOnTop(2);
	    } 
	    wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
	    wgui_action = WGUI_NO_ACTION;
	    break;
	  case WGUI_LOAD_HISTORY3:
	    //cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
	    if(cfgLoadFromFilename(wgui_cfg, iniGetConfigurationHistoryFilename(wgui_ini, 3), false) == FALSE) 
	    {
	      wguiDeleteCfgFromHistory(3);
	    } 
	    else 
	    {
	      iniSetCurrentConfigurationFilename(wgui_ini, iniGetConfigurationHistoryFilename(wgui_ini, 3));
              cfgSetConfigChangedSinceLastSave(wgui_cfg, FALSE);
              wguiCheckMemorySettingsForChipset();
	      wguiPutCfgInHistoryOnTop(3);
	    } 
	    wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
	    wgui_action = WGUI_NO_ACTION;
	    break;
	  //case WGUI_LOAD_STATE:
	  //  wguiOpenStateFile(wgui_cfg, wgui_hDialog);
	  //  wgui_action = WGUI_NO_ACTION;
	  //  break;
	  //case WGUI_SAVE_STATE:
	  //  wguiSaveStateFileAs(wgui_cfg, wgui_hDialog);
	  //  wgui_action = WGUI_NO_ACTION;
	  //  break;
	  case WGUI_DEBUGGER_START:
	    end_loop = TRUE;
	    cfgManagerSetCurrentConfig(&cfg_manager, wgui_cfg);
	    fellowPreStartReset(cfgManagerConfigurationActivate(&cfg_manager) || fellowGetPreStartReset());
	    debugger_start = TRUE;
	  default:
	    break;
	}
      }

      // save main window position
      GetWindowRect(wgui_hDialog, &dialogRect);
      iniSetMainWindowPosition(wgui_ini, dialogRect.left, dialogRect.top);

      DestroyWindow(wgui_hDialog);
      if (!quit_emulator && debugger_start)
      {
	debugger_start = FALSE;
	//wdbgDebugSessionRun(NULL);
	wdebDebug();
      }
      else if (!quit_emulator)
      {
	do
	{
	  winDrvEmulationStart();
	} while (gfx_drv_displaychange);
      }
    } while (!quit_emulator);
    return quit_emulator;
  }

  static bool wguiInitializePresets(wgui_preset **wgui_presets, ULO *wgui_num_presets) {
    STR strSearchPattern[CFG_FILENAME_LENGTH] = "";
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    ULO i = 0;
    cfg *cfgTemp = NULL;
    BOOLE bResult = false;
    STR strFilename[CFG_FILENAME_LENGTH] = "";

    strncpy(strSearchPattern, wgui_preset_path, CFG_FILENAME_LENGTH);
    strncat(strSearchPattern, "\\*", 3);

    // first we count the number of files in the preset directory
    hFind = FindFirstFile(strSearchPattern, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
      fellowAddLog("wguiInitializePresets(): FindFirstFile failed.\n");
      return false;
    }

    *wgui_num_presets = 0;
    do {
      if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
      else
         *wgui_num_presets+=1;
    } while(FindNextFile(hFind, &ffd) != 0);
    FindClose(hFind);

    fellowAddLog("wguiInitializePresets(): %u preset(s) found.\n", *wgui_num_presets);

    // then we allocate the memory to store preset information, and read the information
    if(*wgui_num_presets > 0) {
      *wgui_presets = (wgui_preset *) malloc(*wgui_num_presets * sizeof(wgui_preset));

      hFind = FindFirstFile(strSearchPattern, &ffd);
      do {
        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        else {
          strncpy((*wgui_presets)[i].strPresetFilename, wgui_preset_path, CFG_FILENAME_LENGTH);
          strncat((*wgui_presets)[i].strPresetFilename, "\\", 2);
          strncat((*wgui_presets)[i].strPresetFilename, ffd.cFileName, CFG_FILENAME_LENGTH);

          cfgTemp = cfgManagerGetNewConfig(&cfg_manager);
          if(cfgTemp) {
            bResult = cfgLoadFromFilename(cfgTemp, (*wgui_presets)[i].strPresetFilename, true);

            if(bResult) {
              strncpy((*wgui_presets)[i].strPresetDescription, cfgGetDescription(cfgTemp), CFG_FILENAME_LENGTH);
#ifdef _DEBUG
              fellowAddLog(" preset %u: %s - %s\n", i, ffd.cFileName, (*wgui_presets)[i].strPresetDescription);
#endif
              i++;
            }
            else
              strncpy((*wgui_presets)[i].strPresetDescription, "", CFG_FILENAME_LENGTH);

            cfgManagerFreeConfig(&cfg_manager, cfgTemp);
          }
        }
      } while((FindNextFile(hFind, &ffd) != 0) && (i < *wgui_num_presets));
      FindClose(hFind);
    }

    return TRUE;
  } 


  /*============================================================================*/
  /* Called at the start of Fellow execution                                    */
  /*============================================================================*/

  void wguiStartup(void) {
    wgui_cfg = cfgManagerGetCurrentConfig(&cfg_manager);
    wgui_ini = iniManagerGetCurrentInitdata(&ini_manager);
    wguiConvertDrawModeListToGuiDrawModes(pwgui_dm);

    if(fileopsGetWinFellowPresetPath(wgui_preset_path, CFG_FILENAME_LENGTH)) {
      fellowAddLog("wguiStartup(): preset path = %s\n", wgui_preset_path);
      wguiInitializePresets(&wgui_presets, &wgui_num_presets);
    }
  }


  /*============================================================================*/
  /* Called at the end of Fellow initialization                                 */
  /*============================================================================*/

  void wguiStartupPost(void) {
    wguiLoadBitmaps();

    if (wgui_emulation_state) {
      SendMessage(GetDlgItem(wgui_hDialog, IDC_IMAGE_POWER_LED_MAIN), 
	STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) power_led_on_bitmap);
    } else {
      SendMessage(GetDlgItem(wgui_hDialog, IDC_IMAGE_POWER_LED_MAIN), 
	STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) power_led_off_bitmap);
    }
  }


  /*============================================================================*/
  /* Called at the end of Fellow execution                                      */
  /*============================================================================*/

  void wguiShutdown(void) {
    wguiReleaseBitmaps();
    wguiFreeGuiDrawModesList(pwgui_dm);

    if(wgui_presets != NULL)
      free(wgui_presets);
  }


#endif /* WGUI */
