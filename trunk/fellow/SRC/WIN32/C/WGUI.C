/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Windows GUI code                                                           */
/* Author: Petter Schau (peschau@online.no)                                   */
/* Author: Worfje (worfje@gmx.net)                                    */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "defs.h"

#ifdef WGUI

#include <windows.h>
#include <windowsx.h>
#include <windef.h>
#include <stdlib.h>
#include "gui_general.h"
#include "gui_debugger.h"
#include <process.h>
#include <commctrl.h>
#include <prsht.h>

#include "wgui.h"
#include "windrv.h"
#include "sound.h"
#include "cpu.h"
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


HWND wgui_hDialog;                           /* Handle of the main dialog box */
cfg *wgui_cfg;                               /* GUI copy of configuration */
ini *wgui_ini;								 /* GUI copy of initdata */
STR extractedfilename[CFG_FILENAME_LENGTH];

wgui_drawmodes wgui_dm;								// data structure for resolution data
wgui_drawmodes* pwgui_dm = &wgui_dm;
wgui_drawmode *pwgui_dm_match;
BOOLE wgui_emulation_state = FALSE;

#define MAX_JOYKEY_VALUE 6
#define MAX_JOYKEY_PORT 2

	kbd_event gameport_keys_events[MAX_JOYKEY_PORT][MAX_JOYKEY_VALUE] = {
    {
	    EVENT_JOY0_UP_ACTIVE,
      EVENT_JOY0_DOWN_ACTIVE,
      EVENT_JOY0_LEFT_ACTIVE,
      EVENT_JOY0_RIGHT_ACTIVE,
      EVENT_JOY0_FIRE0_ACTIVE,
      EVENT_JOY0_FIRE1_ACTIVE
      /*
      EVENT_JOY0_AUTOFIRE0_ACTIVE,
      EVENT_JOY0_AUTOFIRE1_ACTIVE
      */
    },
    {
	    EVENT_JOY1_UP_ACTIVE,
      EVENT_JOY1_DOWN_ACTIVE,
      EVENT_JOY1_LEFT_ACTIVE,
      EVENT_JOY1_RIGHT_ACTIVE,
      EVENT_JOY1_FIRE0_ACTIVE,
      EVENT_JOY1_FIRE1_ACTIVE
      /*
      EVENT_JOY1_AUTOFIRE0_ACTIVE,
      EVENT_JOY1_AUTOFIRE1_ACTIVE
      */
    }
  };

  int gameport_keys_labels[MAX_JOYKEY_PORT][MAX_JOYKEY_VALUE] = {
    { IDC_GAMEPORT0_UP, IDC_GAMEPORT0_DOWN, IDC_GAMEPORT0_LEFT, IDC_GAMEPORT0_RIGHT,
      IDC_GAMEPORT0_FIRE0, IDC_GAMEPORT0_FIRE1 },
    { IDC_GAMEPORT1_UP, IDC_GAMEPORT1_DOWN, IDC_GAMEPORT1_LEFT, IDC_GAMEPORT1_RIGHT,
      IDC_GAMEPORT1_FIRE0, IDC_GAMEPORT1_FIRE1 }
  };

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
  WGUI_ABOUT
} wguiActions;

wguiActions wgui_action;

/*============================================================================*/
/* Forward declarations for each property sheet dialog procedure              */
/*============================================================================*/

BOOL CALLBACK wguiCPUDialogProc(HWND hwndDlg,
				UINT uMsg,
				WPARAM wParam,
				LPARAM lParam);
BOOL CALLBACK wguiFloppyDialogProc(HWND hwndDlg,
				   UINT uMsg,
				   WPARAM wParam,
				   LPARAM lParam);
BOOL CALLBACK wguiMemoryDialogProc(HWND hwndDlg,
				   UINT uMsg,
				   WPARAM wParam,
				   LPARAM lParam);
BOOL CALLBACK wguiBlitterDialogProc(HWND hwndDlg,
				    UINT uMsg,
				    WPARAM wParam,
				    LPARAM lParam);
BOOL CALLBACK wguiDisplayDialogProc(HWND hwndDlg,
				    UINT uMsg,
				    WPARAM wParam,
				    LPARAM lParam);
BOOL CALLBACK wguiSoundDialogProc(HWND hwndDlg,
				  UINT uMsg,
				  WPARAM wParam,
				  LPARAM lParam);
BOOL CALLBACK wguiFilesystemAddDialogProc(HWND hwndDlg,
					  UINT uMsg,
					  WPARAM wParam,
					  LPARAM lParam);
BOOL CALLBACK wguiFilesystemDialogProc(HWND hwndDlg,
				       UINT uMsg,
				       WPARAM wParam,
				       LPARAM lParam);
BOOL CALLBACK wguiHardfileCreateDialogProc(HWND hwndDlg,
					UINT uMsg,
					WPARAM wParam,
					LPARAM lParam);
BOOL CALLBACK wguiHardfileAddDialogProc(HWND hwndDlg,
					UINT uMsg,
					WPARAM wParam,
					LPARAM lParam);
BOOL CALLBACK wguiHardfileDialogProc(HWND hwndDlg,
				     UINT uMsg,
				     WPARAM wParam,
				     LPARAM lParam);
BOOL CALLBACK wguiGameportDialogProc(HWND hwndDlg,
				     UINT uMsg,
				     WPARAM wParam,
				     LPARAM lParam);
BOOL CALLBACK wguiVariousDialogProc(HWND hwndDlg,
				    UINT uMsg,
				    WPARAM wParam,
				    LPARAM lParam);
BOOL CALLBACK wguiScreenDialogProc(HWND hwndDlg,
				   UINT uMsg,
				   WPARAM wParam,
				   LPARAM lParam);


/*============================================================================*/
/* The following tables defines the data needed to create the property        */
/* sheets for the configuration dialog box.                                   */
/* -Number of property sheets                                                 */
/* -Resource ID for each property sheet                                       */
/* -Dialog procedure for each property sheet                                  */
/*============================================================================*/

#define PROP_SHEETS 9

UINT wgui_propsheetRID[PROP_SHEETS] = {
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


typedef BOOL (CALLBACK *wguiDlgProc)(HWND, UINT, WPARAM, LPARAM);

wguiDlgProc wgui_propsheetDialogProc[PROP_SHEETS] = {wguiCPUDialogProc,
						     wguiFloppyDialogProc,
						     wguiMemoryDialogProc,
						     wguiDisplayDialogProc,
						     wguiSoundDialogProc,
						     wguiFilesystemDialogProc,
						     wguiHardfileDialogProc,
						     wguiGameportDialogProc,
						     wguiVariousDialogProc};


void wguiSetSliderRange(HWND sliderHWND, ULO minPos, ULO maxPos) {

	SendMessage(sliderHWND, TBM_SETRANGE, TRUE, (LPARAM) MAKELONG(minPos, maxPos));
}

ULO wguiGetComboboxCurrentSelection(HWND comboboxHWND) {
	
	return ComboBox_GetCurSel(comboboxHWND);
}

void wguiSetComboboxCurrentSelection(HWND comboboxHWND, ULO index) {
	
	ComboBox_SetCurSel(comboboxHWND, index);
}

ULO wguiGetSliderPosition(HWND sliderHWND) {

	return SendMessage(sliderHWND, TBM_GETPOS, 0, 0);
}

void wguiSetSliderPosition(HWND sliderHWND, LONG position) {

	SendMessage(sliderHWND, TBM_SETPOS, TRUE, (LPARAM) position);
}

void wguiGetResolutionStrWithIndex(LONG index, char char_buffer[]) {

	felist *listnode;
	wgui_drawmode *pwguicfgwdm;
	ULO i;
	
	pwguicfgwdm = NULL;
	if (pwgui_dm_match->windowed) {
		// windowed
		listnode = pwgui_dm->reswindowed;
	} else {
		// fullscreen
		switch (pwgui_dm_match->colorbits) {
			case 8:
				listnode = pwgui_dm->res8bit;
				break;
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
	pwguicfgwdm = listNode(listnode);
	if (pwguicfgwdm != NULL) {
		sprintf(char_buffer, "%d by %d pixels", pwguicfgwdm->width, pwguicfgwdm->height);
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

void wguiSetSliderTextAccordingToPosition(HWND sliderHWND, HWND sliderTextHWND, void (*getSliderStrWithIndex)(LONG, char[])) {
	char buffer[255];
	ULO pos;

	pos = wguiGetSliderPosition(sliderHWND);
	getSliderStrWithIndex(pos, buffer);
	Static_SetText(sliderTextHWND, buffer);
}

ULO wguiGetColorBitsFromComboboxIndex(LONG index) {

	if (pwgui_dm->comboxbox8bitindex == index) { return 8; }
	if (pwgui_dm->comboxbox16bitindex == index) { return 16; }
	if (pwgui_dm->comboxbox24bitindex == index) { return 24; }
	if (pwgui_dm->comboxbox32bitindex == index) { return 32; }
	return 8;
}

LONG wguiGetComboboxIndexFromColorBits(ULO colorbits) {

  switch (colorbits) {
    case 8:  return pwgui_dm->comboxbox8bitindex;
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

void wguiConvertDrawModeListToGuiDrawModes(wgui_drawmodes *wdms) {

  felist* reslist;
  draw_mode * dm;
  wgui_drawmode* pwdm;
	ULO idw				= 0;
	ULO id8bit		= 0;
	ULO id16bit		= 0;
	ULO id24bit		= 0;
	ULO id32bit		= 0;
	wdms->reswindowed = NULL;
	wdms->res8bit = NULL;
	wdms->res16bit = NULL;
	wdms->res24bit = NULL;
	wdms->res32bit = NULL;
	wdms->comboxbox8bitindex = -1;
	wdms->comboxbox16bitindex = -1;
	wdms->comboxbox24bitindex = -1;
	wdms->comboxbox32bitindex = -1;

  for (reslist = drawGetModes(); reslist != NULL; reslist = listNext(reslist)) {
    dm = listNode(reslist);
	  pwdm = (wgui_drawmode *) malloc(sizeof(wgui_drawmode));
		wguiConvertDrawModeNodeToGuiDrawNode(dm, pwdm);
		if (dm->windowed) {
			// windowed
			pwdm->id = idw;
			wdms->reswindowed = listAddLast(wdms->reswindowed, listNew(pwdm));
			idw++;
		} else {
			// fullscreen
			switch(dm->bits) {
				case 8:
					pwdm->id = id8bit;
					wdms->res8bit = listAddLast(wdms->res8bit, listNew(pwdm));
					id8bit++;
					break;

				case 16:
					pwdm->id = id16bit;
					wdms->res16bit = listAddLast(wdms->res16bit, listNew(pwdm));
					id16bit++;
					break;

				case 24:
					pwdm->id = id24bit;
					wdms->res24bit = listAddLast(wdms->res24bit, listNew(pwdm));
					id24bit++;
					break;

				case 32:
					pwdm->id = id32bit;
					wdms->res32bit = listAddLast(wdms->res32bit, listNew(pwdm));
					id32bit++;
					break;
			}
		}
	}
	wdms->numberofwindowed	= idw;
	wdms->numberof8bit			= id8bit;
	wdms->numberof16bit			= id16bit;
	wdms->numberof24bit			= id24bit;
	wdms->numberof32bit			= id32bit;
}

void wguiFreeGuiDrawModesList(wgui_drawmodes *wdms) {
	
	listFreeAll(wdms->reswindowed, TRUE);
	listFreeAll(wdms->res8bit, TRUE);
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
			case 8:
				return pwgui_dm->res8bit;
			case 16:
				return pwgui_dm->res16bit;
			case 24:
				return pwgui_dm->res24bit;
			case 32:
				return pwgui_dm->res32bit;
		}
	}
	return pwgui_dm->res8bit;
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
		dm = listNode(i);
		if ((dm->height == height) && (dm->width == width)) {
			return dm;
		}
	}
	// if no matching is found return pointer to first resolution of 8 colorbits
	return listNode(pwgui_dm->res8bit);
}

/*============================================================================*/
/* Extract the filename from a full path name                                 */
/*============================================================================*/

STR *wguiExtractFilename(STR *fullpathname) {
  BYT *strpointer;

  strpointer = strrchr(fullpathname, '\\');
  strncpy(extractedfilename, fullpathname + strlen(fullpathname) - strlen(strpointer) + 1, strlen(fullpathname) - strlen(strpointer) - 1);
  return extractedfilename;
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

typedef enum {
  FSEL_ROM = 0,
  FSEL_ADF = 1,
  FSEL_KEY = 2,
  FSEL_HDF = 3,
  FSEL_WFC = 4
} SelectFileFlags;

static STR FileType[5][CFG_FILENAME_LENGTH] = {
	"ROM Images (.rom)\0*.rom\0ADF Diskfiles\0*.adf;*.adz;*.adf.gz;*.dms\0\0\0",
    "ADF Diskfiles (.adf;.adz;.adf.gz;.dms)\0*.adf;*.adz;*.adf.gz;*.dms\0\0\0",
    "Key Files (.key)\0*.key\0\0\0", 
    "Hard Files (.hdf)\0*.hdf\0\0\0",
    "Configuration Files (.wfc)\0*.wfc\0\0\0"};

BOOLE wguiSelectFile(HWND DlgHWND,
		     STR *filename, 
		     ULO filenamesize,
		     STR *title,
		     SelectFileFlags SelectFileType) {
  OPENFILENAME ofn;
  STR filters[CFG_FILENAME_LENGTH];
  STR *pfilters;

  memcpy(filters, &FileType[SelectFileType], CFG_FILENAME_LENGTH);
  pfilters = &filters[0];

  ofn.lStructSize = sizeof(ofn);       /* Set all members to familiarize with */
  ofn.hwndOwner = DlgHWND;                            /* the possibilities... */
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
	break;
  case FSEL_KEY:
	ofn.lpstrInitialDir = iniGetLastUsedKeyDir(wgui_ini);
    break;
  case FSEL_HDF:
	ofn.lpstrInitialDir = iniGetLastUsedCfgDir(wgui_ini);
    break;
  case FSEL_WFC:
	ofn.lpstrInitialDir = iniGetLastUsedCfgDir(wgui_ini);
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
  ofn.lCustData = (long) NULL;
  ofn.lpfnHook = NULL;
  ofn.lpTemplateName = NULL;
  return GetOpenFileName(&ofn);
}

BOOLE wguiSaveFile(HWND DlgHWND,
		     STR *filename, 
		     ULO filenamesize,
		     STR *title,
		     SelectFileFlags SelectFileType) {
  OPENFILENAME ofn;
  STR filters[CFG_FILENAME_LENGTH];
  STR *pfilters;

  memcpy(filters, &FileType[SelectFileType], CFG_FILENAME_LENGTH);
  pfilters = &filters[0];

  ofn.lStructSize = sizeof(ofn);       /* Set all members to familiarize with */
  ofn.hwndOwner = DlgHWND;                            /* the possibilities... */
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
  ofn.lpstrInitialDir = NULL; // the initial dir also could be remembered (ini-file)
  ofn.lpstrTitle = title;
  ofn.Flags = OFN_EXPLORER |
			  OFN_HIDEREADONLY |
			  OFN_OVERWRITEPROMPT;
  ofn.nFileOffset = 0;
  ofn.nFileExtension = 0;
  ofn.lpstrDefExt = &".wfc";
  ofn.lCustData = (long) NULL;
  ofn.lpfnHook = NULL;
  ofn.lpTemplateName = NULL;
  return GetSaveFileName(&ofn);
}

BOOLE wguiSelectDirectory(HWND DlgHWND,
			  STR *filename, 
			  ULO filenamesize,
			  STR *title) {
  OPENFILENAME ofn;
  STR *filters = NULL;

  ofn.lStructSize = sizeof(ofn);       /* Set all members to familiarize with */
  ofn.hwndOwner = DlgHWND;                            /* the possibilities... */
  ofn.hInstance = win_drv_hInstance;
  ofn.lpstrFilter = filters;
  ofn.lpstrCustomFilter = NULL;
  ofn.nMaxCustFilter = 0;
  ofn.nFilterIndex = 1;
  filename[0] = '\0';
  ofn.lpstrFile = filename;
  ofn.nMaxFile = filenamesize;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = title;
  ofn.Flags = OFN_EXPLORER |
              OFN_FILEMUSTEXIST |
              OFN_NOCHANGEDIR;
  ofn.nFileOffset = 0;
  ofn.nFileExtension = 0;
  ofn.lpstrDefExt = NULL;
  ofn.lCustData = (long) NULL;
  ofn.lpfnHook = NULL;
  ofn.lpTemplateName = NULL;
  return GetOpenFileName(&ofn);
}

/*============================================================================*/
/* Install history of configuration files into window menu                    */
/*============================================================================*/

void wguiRemoveAllHistory(void) {
	RemoveMenu(GetSubMenu(GetMenu(wgui_hDialog),0), ID_FILE_HISTORYCONFIGURATION0, MF_BYCOMMAND);
	RemoveMenu(GetSubMenu(GetMenu(wgui_hDialog),0), ID_FILE_HISTORYCONFIGURATION1, MF_BYCOMMAND);
	RemoveMenu(GetSubMenu(GetMenu(wgui_hDialog),0), ID_FILE_HISTORYCONFIGURATION2, MF_BYCOMMAND);
	RemoveMenu(GetSubMenu(GetMenu(wgui_hDialog),0), ID_FILE_HISTORYCONFIGURATION3, MF_BYCOMMAND);
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

void wguiInsertCfgIntoHistory(STR *cfgfilenametoinsert) {
	STR cfgfilename[CFG_FILENAME_LENGTH];
	ULO i;

	for (i=3; i>0; i--) {
		cfgfilename[0]='\0';
		strncat(cfgfilename, iniGetConfigurationHistoryFilename(wgui_ini, i-1), CFG_FILENAME_LENGTH);
		iniSetConfigurationHistoryFilename(wgui_ini, i, cfgfilename);
	}
	iniSetConfigurationHistoryFilename(wgui_ini, 0, cfgfilenametoinsert);
	wguiInstallHistoryIntoMenu();
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


/*============================================================================*/
/* Saves and loads configuration files (*.wfc)                                */
/*============================================================================*/

void wguiSaveConfigurationFileAs(cfg *conf, HWND hwndDlg) {
  STR filename[CFG_FILENAME_LENGTH];
  
  if (wguiSaveFile(hwndDlg,
		     filename, 
		     CFG_FILENAME_LENGTH, 
		     "Save As", 
		     FSEL_WFC)) {
    cfgSaveToFilename(wgui_cfg, filename);
	iniSetCurrentConfigurationFilename(wgui_ini, filename);
  }
}


void wguiOpenConfigurationFile(cfg *conf, HWND hwndDlg) {
  STR filename[CFG_FILENAME_LENGTH];
  STR pathname[CFG_FILENAME_LENGTH];
  BYT *strpointer;
  
  if (wguiSelectFile(hwndDlg,
		     filename, 
		     CFG_FILENAME_LENGTH, 
		     "Open", 
		     FSEL_WFC)) {
    cfgLoadFromFilename(wgui_cfg, filename);
	iniSetCurrentConfigurationFilename(wgui_ini, filename);
	
	// extract directory for ini-file
	strpointer = strrchr(filename, '\\');
	strncpy(pathname, filename, strlen(filename) - strlen(strpointer));
	pathname[strlen(filename) - strlen(strpointer)] = '\0';
	iniSetLastUsedCfgDir(wgui_ini, pathname);

	wguiInsertCfgIntoHistory(filename);
  }
}

/*============================================================================*/
/* CPU config                                                                 */
/*============================================================================*/

/* Install CPU config */

void wguiInstallCPUConfig(HWND DlgHWND, cfg *conf) {
  int slidervalue;
  HWND sliderHWND;

  /* Set CPU type */

  switch (cfgGetCPUType(conf)) {
    default:
    case M68000:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_68000), TRUE);
      break;
    case M68010:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_68010), TRUE);
      break;
    case M68020:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_68020), TRUE);
      break;
    case M68030:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_68030), TRUE);
      break;
    case M68EC20:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_68EC20), TRUE);
      break;
    case M68EC30:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_68EC30), TRUE);
      break;
  }

  /* Set CPU speed */

  sliderHWND = GetDlgItem(DlgHWND, IDC_SLIDER_CPU_SPEED);
  SendMessage(sliderHWND, TBM_SETRANGE, TRUE, (LPARAM) MAKELONG(0, 3));
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
  }
  SendMessage(sliderHWND, TBM_SETPOS, TRUE, (LPARAM) slidervalue);
}

/* Extract CPU config */

void wguiExtractCPUConfig(HWND DlgHWND, cfg *conf) {
  int slidervalue;
	
  /* Get CPU type */

  if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_68000)))
    cfgSetCPUType(conf, M68000);
  else if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_68010)))
    cfgSetCPUType(conf, M68010);
  else if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_68020)))
    cfgSetCPUType(conf, M68020);
  else if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_68030)))
    cfgSetCPUType(conf, M68030);
  else if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_68EC20)))
    cfgSetCPUType(conf, M68EC20);
  else if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_68EC30)))
    cfgSetCPUType(conf, M68EC30);
  else
    cfgSetCPUType(conf, M68000);

  /* Get CPU speed */

  slidervalue = SendMessage(GetDlgItem(DlgHWND, IDC_SLIDER_CPU_SPEED),
			    TBM_GETPOS, 
			    0, 
			    0);  
  switch (slidervalue) {
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
  }
}


/*============================================================================*/
/* Floppy config                                                              */
/*============================================================================*/

/* Install floppy config */

void wguiInstallFloppyConfig(HWND DlgHWND, cfg *conf) {

  /* Set floppy image names */

  Edit_SetText(GetDlgItem(DlgHWND, IDC_EDIT_DF0_IMAGENAME),
	       cfgGetDiskImage(conf, 0));
  Edit_SetText(GetDlgItem(DlgHWND, IDC_EDIT_DF1_IMAGENAME),
	       cfgGetDiskImage(conf, 1));
  Edit_SetText(GetDlgItem(DlgHWND, IDC_EDIT_DF2_IMAGENAME),
	       cfgGetDiskImage(conf, 2));
  Edit_SetText(GetDlgItem(DlgHWND, IDC_EDIT_DF3_IMAGENAME),
	       cfgGetDiskImage(conf, 3));

  /* Set drive enabled check boxes */

  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_DF0_ENABLED),
		  cfgGetDiskEnabled(conf, 0));
  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_DF1_ENABLED),
		  cfgGetDiskEnabled(conf, 1));
  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_DF2_ENABLED),
		  cfgGetDiskEnabled(conf, 2));
  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_DF3_ENABLED),
		  cfgGetDiskEnabled(conf, 3));

  /* Set drive readonly check boxes */

  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_DF0_READONLY),
		  cfgGetDiskReadOnly(conf, 0));
  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_DF1_READONLY),
		  cfgGetDiskReadOnly(conf, 1));
  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_DF2_READONLY),
		  cfgGetDiskReadOnly(conf, 2));
  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_DF3_READONLY),
		  cfgGetDiskReadOnly(conf, 3));

  /* Set fast DMA check box */

  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_FAST_DMA),
		  cfgGetDiskFast(conf));
}

/* set floppy images in main window */

void wguiInstallFloppyMain(HWND DlgHWND, cfg *conf) {

  int i;

  /* Set floppy image names */

  Edit_SetText(GetDlgItem(DlgHWND, IDC_EDIT_DF0_IMAGENAME_MAIN),
	       cfgGetDiskImage(conf, 0));
  Edit_SetText(GetDlgItem(DlgHWND, IDC_EDIT_DF1_IMAGENAME_MAIN),
	       cfgGetDiskImage(conf, 1));
  Edit_SetText(GetDlgItem(DlgHWND, IDC_EDIT_DF2_IMAGENAME_MAIN),
	       cfgGetDiskImage(conf, 2));
  Edit_SetText(GetDlgItem(DlgHWND, IDC_EDIT_DF3_IMAGENAME_MAIN),
	       cfgGetDiskImage(conf, 3));

  /* Enable or Disable disk drive boxes */

  Edit_Enable(GetDlgItem(DlgHWND, IDC_EDIT_DF0_IMAGENAME_MAIN),
			cfgGetDiskEnabled(conf,0) == TRUE);
  Edit_Enable(GetDlgItem(DlgHWND, IDC_EDIT_DF1_IMAGENAME_MAIN),
			cfgGetDiskEnabled(conf,1) == TRUE);
  Edit_Enable(GetDlgItem(DlgHWND, IDC_EDIT_DF2_IMAGENAME_MAIN),
			cfgGetDiskEnabled(conf,2) == TRUE);
  Edit_Enable(GetDlgItem(DlgHWND, IDC_EDIT_DF3_IMAGENAME_MAIN),
			cfgGetDiskEnabled(conf,3) == TRUE);
	
  Button_Enable(GetDlgItem(DlgHWND, IDC_BUTTON_DF0_EJECT_MAIN),
			cfgGetDiskEnabled(conf,0) == TRUE);
  Button_Enable(GetDlgItem(DlgHWND, IDC_BUTTON_DF1_EJECT_MAIN),
			cfgGetDiskEnabled(conf,1) == TRUE);
  Button_Enable(GetDlgItem(DlgHWND, IDC_BUTTON_DF2_EJECT_MAIN),
			cfgGetDiskEnabled(conf,2) == TRUE);
  Button_Enable(GetDlgItem(DlgHWND, IDC_BUTTON_DF3_EJECT_MAIN),
			cfgGetDiskEnabled(conf,3) == TRUE);

  Button_Enable(GetDlgItem(DlgHWND, IDC_BUTTON_DF0_FILEDIALOG_MAIN),
			cfgGetDiskEnabled(conf,0) == TRUE);
  Button_Enable(GetDlgItem(DlgHWND, IDC_BUTTON_DF1_FILEDIALOG_MAIN),
			cfgGetDiskEnabled(conf,1) == TRUE);
  Button_Enable(GetDlgItem(DlgHWND, IDC_BUTTON_DF2_FILEDIALOG_MAIN),
			cfgGetDiskEnabled(conf,2) == TRUE);
  Button_Enable(GetDlgItem(DlgHWND, IDC_BUTTON_DF3_FILEDIALOG_MAIN),
			cfgGetDiskEnabled(conf,3) == TRUE); 

  if (cfgGetDiskEnabled(conf,0) == TRUE) {
    SendMessage(GetDlgItem(DlgHWND, IDC_IMAGE_DF0_LED_MAIN), 
		STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) LoadBitmap(win_drv_hInstance, 
		MAKEINTRESOURCE(IDB_DISKDRIVE_LED_OFF)));
  } else {
    SendMessage(GetDlgItem(DlgHWND, IDC_IMAGE_DF0_LED_MAIN), 
		STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) LoadBitmap(win_drv_hInstance, 
		MAKEINTRESOURCE(IDB_DISKDRIVE_LED_DISABLED)));
  }

  if (cfgGetDiskEnabled(conf,1) == TRUE) {
    SendMessage(GetDlgItem(DlgHWND, IDC_IMAGE_DF1_LED_MAIN), 
		STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) LoadBitmap(win_drv_hInstance, 
		MAKEINTRESOURCE(IDB_DISKDRIVE_LED_OFF)));
  } else {
    SendMessage(GetDlgItem(DlgHWND, IDC_IMAGE_DF1_LED_MAIN), 
		STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) LoadBitmap(win_drv_hInstance, 
		MAKEINTRESOURCE(IDB_DISKDRIVE_LED_DISABLED)));
  }

  if (cfgGetDiskEnabled(conf,2) == TRUE) {
    SendMessage(GetDlgItem(DlgHWND, IDC_IMAGE_DF2_LED_MAIN), 
		STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) LoadBitmap(win_drv_hInstance, 
		MAKEINTRESOURCE(IDB_DISKDRIVE_LED_OFF)));
  } else {
    SendMessage(GetDlgItem(DlgHWND, IDC_IMAGE_DF2_LED_MAIN), 
		STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) LoadBitmap(win_drv_hInstance, 
		MAKEINTRESOURCE(IDB_DISKDRIVE_LED_DISABLED)));
  }

  if (cfgGetDiskEnabled(conf,3) == TRUE) {
    SendMessage(GetDlgItem(DlgHWND, IDC_IMAGE_DF3_LED_MAIN), 
		STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) LoadBitmap(win_drv_hInstance, 
		MAKEINTRESOURCE(IDB_DISKDRIVE_LED_OFF)));
  } else {
    SendMessage(GetDlgItem(DlgHWND, IDC_IMAGE_DF3_LED_MAIN), 
		STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) LoadBitmap(win_drv_hInstance, 
		MAKEINTRESOURCE(IDB_DISKDRIVE_LED_DISABLED)));
  }
}

/* Extract floppy config */

void wguiExtractFloppyConfig(HWND DlgHWND, cfg *conf) {
  char tmp[CFG_FILENAME_LENGTH];

  /* Get floppy disk image names */

  Edit_GetText(GetDlgItem(DlgHWND, IDC_EDIT_DF0_IMAGENAME),
	       tmp,
	       CFG_FILENAME_LENGTH);
  cfgSetDiskImage(conf, 0, tmp);
  Edit_GetText(GetDlgItem(DlgHWND, IDC_EDIT_DF1_IMAGENAME),
	       tmp,
	       CFG_FILENAME_LENGTH);
  cfgSetDiskImage(conf, 1, tmp);
  Edit_GetText(GetDlgItem(DlgHWND, IDC_EDIT_DF2_IMAGENAME),
	       tmp,
	       CFG_FILENAME_LENGTH);
  cfgSetDiskImage(conf, 2, tmp);
  Edit_GetText(GetDlgItem(DlgHWND, IDC_EDIT_DF3_IMAGENAME),
	       tmp,
	       CFG_FILENAME_LENGTH);
  cfgSetDiskImage(conf, 3, tmp);

  /* Get drives enabled */

  cfgSetDiskEnabled(conf,
		    0,
		    Button_GetCheck(GetDlgItem(DlgHWND,IDC_CHECK_DF0_ENABLED)));
  cfgSetDiskEnabled(conf,
		    1,
		    Button_GetCheck(GetDlgItem(DlgHWND,IDC_CHECK_DF1_ENABLED)));
  cfgSetDiskEnabled(conf,
		    2,
		    Button_GetCheck(GetDlgItem(DlgHWND,IDC_CHECK_DF2_ENABLED)));
  cfgSetDiskEnabled(conf,
		    3,
		    Button_GetCheck(GetDlgItem(DlgHWND,IDC_CHECK_DF3_ENABLED)));

  /* Get drives readonly */

  cfgSetDiskReadOnly(conf,
		     0,
		   Button_GetCheck(GetDlgItem(DlgHWND,IDC_CHECK_DF0_READONLY)));
  cfgSetDiskReadOnly(conf,
		     1,
		   Button_GetCheck(GetDlgItem(DlgHWND,IDC_CHECK_DF1_READONLY)));
  cfgSetDiskReadOnly(conf,
		     2,
		   Button_GetCheck(GetDlgItem(DlgHWND,IDC_CHECK_DF2_READONLY)));
  cfgSetDiskReadOnly(conf,
		     3,
		   Button_GetCheck(GetDlgItem(DlgHWND,IDC_CHECK_DF3_READONLY)));

  /* Get fast DMA */

  cfgSetDiskFast(conf,
		 Button_GetCheck(GetDlgItem(DlgHWND,IDC_CHECK_FAST_DMA)));
}

/* Extract floppy config from main window */

void wguiExtractFloppyMain(HWND DlgHWND, cfg *conf) {
  char tmp[CFG_FILENAME_LENGTH];

  /* Get floppy disk image names */

  if (cfgGetDiskEnabled(conf, 0) == FALSE) {
	Edit_GetText(GetDlgItem(DlgHWND, IDC_EDIT_DF0_IMAGENAME_MAIN),
		tmp, CFG_FILENAME_LENGTH);
	cfgSetDiskImage(conf, 0, tmp);
  }
  if (cfgGetDiskEnabled(conf, 1) == FALSE) {
	Edit_GetText(GetDlgItem(DlgHWND, IDC_EDIT_DF1_IMAGENAME_MAIN),
		tmp,
	    CFG_FILENAME_LENGTH);
	cfgSetDiskImage(conf, 1, tmp);
  }
  if (cfgGetDiskEnabled(conf, 2) == FALSE) {
    Edit_GetText(GetDlgItem(DlgHWND, IDC_EDIT_DF2_IMAGENAME_MAIN),
	    tmp,
	    CFG_FILENAME_LENGTH);
	cfgSetDiskImage(conf, 2, tmp);
  }
  if (cfgGetDiskEnabled(conf, 3) == FALSE) {
	Edit_GetText(GetDlgItem(DlgHWND, IDC_EDIT_DF3_IMAGENAME_MAIN),
	    tmp,
	    CFG_FILENAME_LENGTH);
	cfgSetDiskImage(conf, 3, tmp);
  }
 
}



/*============================================================================*/
/* Memory config                                                              */
/*============================================================================*/

/* Install memory config */

void wguiInstallMemoryConfig(HWND DlgHWND, cfg *conf) {
  HWND ChipChoice = GetDlgItem(DlgHWND, IDC_COMBO_CHIP);
  HWND FastChoice = GetDlgItem(DlgHWND, IDC_COMBO_FAST);
  HWND BogoChoice = GetDlgItem(DlgHWND, IDC_COMBO_BOGO);
  ULO fastindex;
	
  /* Add choice choices */

  ComboBox_AddString(ChipChoice, "256 KB");
  ComboBox_AddString(ChipChoice, "512 KB");
  ComboBox_AddString(ChipChoice, "768 KB");
  ComboBox_AddString(ChipChoice, "1024 KB");
  ComboBox_AddString(ChipChoice, "1280 KB");
  ComboBox_AddString(ChipChoice, "1536 KB");
  ComboBox_AddString(ChipChoice, "1792 KB");
  ComboBox_AddString(ChipChoice, "2048 KB");
  
  ComboBox_AddString(FastChoice, "0 MB");
  ComboBox_AddString(FastChoice, "1 MB");
  ComboBox_AddString(FastChoice, "2 MB");
  ComboBox_AddString(FastChoice, "4 MB");
  ComboBox_AddString(FastChoice, "8 MB");

  ComboBox_AddString(BogoChoice, "0 KB");
  ComboBox_AddString(BogoChoice, "256 KB");
  ComboBox_AddString(BogoChoice, "512 KB");
  ComboBox_AddString(BogoChoice, "768 KB");
  ComboBox_AddString(BogoChoice, "1024 KB");
  ComboBox_AddString(BogoChoice, "1280 KB");
  ComboBox_AddString(BogoChoice, "1536 KB");
  ComboBox_AddString(BogoChoice, "1792 KB");

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
  ComboBox_SetCurSel(ChipChoice, (cfgGetChipSize(conf) / 0x40000) - 1);
  ComboBox_SetCurSel(FastChoice, fastindex);
  ComboBox_SetCurSel(BogoChoice, cfgGetBogoSize(conf) / 0x40000);

  /* Set current ROM and key file names */

  Edit_SetText(GetDlgItem(DlgHWND, IDC_EDIT_KICKSTART),
	       cfgGetKickImage(conf));
  Edit_SetText(GetDlgItem(DlgHWND, IDC_EDIT_KEYFILE),
	       cfgGetKey(conf));
}

/* Extract memory config */

void wguiExtractMemoryConfig(HWND DlgHWND, cfg *conf) {
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

  cursel = ComboBox_GetCurSel(GetDlgItem(DlgHWND, IDC_COMBO_CHIP));
  if (cursel > 7) cursel = 7;
  cfgSetChipSize(conf, sizes1[cursel + 1]);

  cursel = ComboBox_GetCurSel(GetDlgItem(DlgHWND, IDC_COMBO_BOGO));
  if (cursel > 7) cursel = 7;
  cfgSetBogoSize(conf, sizes1[cursel]);

  cursel = ComboBox_GetCurSel(GetDlgItem(DlgHWND, IDC_COMBO_FAST));
  if (cursel > 4) cursel = 4;
  cfgSetFastSize(conf, sizes2[cursel]);

  /* Get current kickstart image and keyfile */
      
  Edit_GetText(GetDlgItem(DlgHWND, IDC_EDIT_KICKSTART),
	       tmp,
	       CFG_FILENAME_LENGTH);
  cfgSetKickImage(conf, tmp);
  Edit_GetText(GetDlgItem(DlgHWND, IDC_EDIT_KEYFILE),
	       tmp,
	       CFG_FILENAME_LENGTH);
  cfgSetKey(conf, tmp);
}

/*============================================================================*/
/* Blitter config                                                             */
/*============================================================================*/

/* Install Blitter config */

void wguiInstallBlitterConfig(HWND DlgHWND, cfg *conf) {

  /* Set blitter operation type */

  if (cfgGetBlitterFast(conf))
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_BLITTER_IMMEDIATE), TRUE);
  else
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_BLITTER_NORMAL), TRUE);

  /* Set blitter chipset type */

  if (cfgGetECSBlitter(conf))
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_BLITTER_ECS), TRUE);
  else
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_BLITTER_OCS), TRUE);
}

/* Extract Blitter config */

void wguiExtractBlitterConfig(HWND DlgHWND, cfg *conf) {
  int slidervalue;
	
  /* Get current blitter operation type */

  cfgSetBlitterFast(conf, 
		    Button_GetCheck(GetDlgItem(DlgHWND,
					       IDC_RADIO_BLITTER_IMMEDIATE)));
  /* Get current blitter chipset type */

  cfgSetECSBlitter(conf, 
		   Button_GetCheck(GetDlgItem(DlgHWND,
					      IDC_RADIO_BLITTER_ECS)));
}


/*============================================================================*/
/* Sound config                                                               */
/*============================================================================*/

/* Install sound config */

void wguiInstallSoundConfig(HWND DlgHWND, cfg *conf) {

  /* Set sound emulation type */

  switch (cfgGetSoundEmulation(conf)) {
    case SOUND_NONE:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_NO_EMULATION), TRUE);
      break;
    case SOUND_PLAY:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_PLAY), TRUE);
      break;
    case SOUND_EMULATE:
      Button_SetCheck(GetDlgItem(DlgHWND,
				 IDC_RADIO_SOUND_SILENT_EMULATION),
		      TRUE);
      break;
  }

  /* Set sound rate */

  switch (cfgGetSoundRate(conf)) {
    case SOUND_44100:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_44100), TRUE);
      break;
    case SOUND_31300:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_31300), TRUE);
      break;
    case SOUND_22050:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_22050), TRUE);
      break;
    case SOUND_15650:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_15650), TRUE);
      break;
  }

  /* Set sound channels */

  if (cfgGetSoundStereo(conf))
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_STEREO), TRUE);
  else
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_MONO), TRUE);

  /* Set sound bits */

  if (cfgGetSound16Bits(conf))
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_16BITS), TRUE);
  else
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_8BITS), TRUE);

  /* Set sound filter */

  switch (cfgGetSoundFilter(conf)) {
    case SOUND_FILTER_ORIGINAL:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_FILTER_ORIGINAL), 
		      TRUE);
      break;
    case SOUND_FILTER_ALWAYS:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_FILTER_ALWAYS), 
		      TRUE);
      break;
    case SOUND_FILTER_NEVER:
      Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_FILTER_NEVER),
		      TRUE);
      break;
  }

   /* Set sound WAV dump */

  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_SOUND_WAV),
		  cfgGetSoundWAVDump(conf));

  /* set sound hardware notification */
  switch (cfgGetSoundNotification(conf)) {
    case SOUND_DSOUND_NOTIFICATION:
	  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_SOUND_NOTIFICATION), TRUE);
	  break;
    case SOUND_MMTIMER_NOTIFICATION:
	  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_SOUND_NOTIFICATION), FALSE);
	  break;
  }
  
  /* set slider of buffer length */
  SendMessage(GetDlgItem(DlgHWND, IDC_SLIDER_SOUND_BUFFER_LENGTH), TBM_SETRANGE, 0, MAKELONG(10,80));
  SendMessage(GetDlgItem(DlgHWND, IDC_SLIDER_SOUND_BUFFER_LENGTH), TBM_SETPOS, 1, cfgGetSoundBufferLength(conf));

}

/* Extract sound config */

void wguiExtractSoundConfig(HWND DlgHWND, cfg *conf) {
	
  /* Get current sound emulation type */

  if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_NO_EMULATION)))
    cfgSetSoundEmulation(conf, SOUND_NONE);
  else if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_PLAY)))
    cfgSetSoundEmulation(conf, SOUND_PLAY);
  else if (Button_GetCheck(GetDlgItem(DlgHWND,
				      IDC_RADIO_SOUND_SILENT_EMULATION)))
    cfgSetSoundEmulation(conf, SOUND_EMULATE);

  /* Get current sound rate */

  if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_44100)))
    cfgSetSoundRate(conf, SOUND_44100);
  else if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_31300)))
    cfgSetSoundRate(conf, SOUND_31300);
  else if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_22050)))
    cfgSetSoundRate(conf, SOUND_22050);
  else if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_15650)))
    cfgSetSoundRate(conf, SOUND_15650);

  /* Get current sound channels */

  cfgSetSoundStereo(conf,
		    Button_GetCheck(GetDlgItem(DlgHWND,
					       IDC_RADIO_SOUND_STEREO)));

  /* Get current sound bits */

  cfgSetSound16Bits(conf,
		    Button_GetCheck(GetDlgItem(DlgHWND,
					       IDC_RADIO_SOUND_16BITS)));

  /* Get current sound filter */

  if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_FILTER_ORIGINAL)))
    cfgSetSoundFilter(conf, SOUND_FILTER_ORIGINAL);
  else if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_FILTER_ALWAYS)))
    cfgSetSoundFilter(conf, SOUND_FILTER_ALWAYS);
  else if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SOUND_FILTER_NEVER)))
    cfgSetSoundFilter(conf, SOUND_FILTER_NEVER);

  /* Get current sound WAV dump */
  cfgSetSoundWAVDump(conf, Button_GetCheck(GetDlgItem(DlgHWND, IDC_CHECK_SOUND_WAV)));

  /* get notify option */
  if (Button_GetCheck(GetDlgItem(DlgHWND, IDC_CHECK_SOUND_NOTIFICATION)))
    cfgSetSoundNotification(conf, SOUND_DSOUND_NOTIFICATION);
  else if (!Button_GetCheck(GetDlgItem(DlgHWND, IDC_CHECK_SOUND_NOTIFICATION)))
    cfgSetSoundNotification(conf, SOUND_MMTIMER_NOTIFICATION);

  /* get slider of buffer length */
  cfgSetSoundBufferLength(conf, SendMessage(GetDlgItem(DlgHWND, IDC_SLIDER_SOUND_BUFFER_LENGTH), TBM_GETPOS, 0, 0));
}


/*============================================================================*/
/* Gameport config                                                            */
/*============================================================================*/

/* Install gameport config */

void wguiInstallGameportConfig(HWND DlgHWND, cfg *conf) {
  HWND gpChoice[2];
  ULO gpindex, i;
  HWND st;
  int setting, port;

  gpChoice[0] = GetDlgItem(DlgHWND, IDC_COMBO_GAMEPORT1);
  gpChoice[1] = GetDlgItem(DlgHWND, IDC_COMBO_GAMEPORT2);

  /* Set current gameport selections */

  for (i = 0; i < 2; i++) {
    ComboBox_AddString(gpChoice[i], "None");
    ComboBox_AddString(gpChoice[i], "Keyboard Layout 1");
    ComboBox_AddString(gpChoice[i], "Keyboard Layout 2");
    ComboBox_AddString(gpChoice[i], "Mouse");
    ComboBox_AddString(gpChoice[i], "Joystick 1");
	ComboBox_AddString(gpChoice[i], "Joystick 2");
  
    switch (cfgGetGameport(conf, i)) {
      case GP_NONE:
	gpindex = 0;
	break;
      case GP_JOYKEY0:
	gpindex = 1;
	break;
      case GP_JOYKEY1:
	gpindex = 2;
	break;
      case GP_MOUSE0:
	gpindex = 3;
	break;
      case GP_ANALOG0:
	gpindex = 4;
	break;
		case GP_ANALOG1:
	gpindex = 5;
	break;
    }
    ComboBox_SetCurSel(gpChoice[i], gpindex);
  }

  for( port = 0; port < MAX_JOYKEY_PORT; port ++ )
    for ( setting = 0; setting < MAX_JOYKEY_VALUE; setting++) {
      st = GetDlgItem(DlgHWND, gameport_keys_labels[port][setting]);
      SetWindowText( st, kbdDrvKeyPrettyString( kbdDrvJoystickReplacementGet( gameport_keys_events[port][setting] )));
    }
}

/* Extract gameport config */

void wguiExtractGameportConfig(HWND DlgHWND, cfg *conf) {
  HWND gpChoice[2];
  ULO gpsel, i;
  gameport_inputs gpt[6] = {GP_NONE,
			    GP_JOYKEY0,
			    GP_JOYKEY1,
			    GP_MOUSE0,
			    GP_ANALOG0,
				GP_ANALOG1};

  gpChoice[0] = GetDlgItem(DlgHWND, IDC_COMBO_GAMEPORT1);
  gpChoice[1] = GetDlgItem(DlgHWND, IDC_COMBO_GAMEPORT2);

  /* Get current gameport inputs */

  for (i = 0; i < 2; i++) {
    gpsel = ComboBox_GetCurSel(gpChoice[i]);
    if (gpsel > 5) gpsel = 0;
    cfgSetGameport(conf, i, gpt[gpsel]);
  }
}


/*============================================================================*/
/* Various config                                                             */
/*============================================================================*/

/* Install various config */

void wguiInstallVariousConfig(HWND DlgHWND, cfg *conf) {

  /* Set measure speed */

  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_VARIOUS_SPEED),
		  cfgGetMeasureSpeed(conf));

  /* Set draw LED */

  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_VARIOUS_LED),
		  cfgGetScreenDrawLEDs(conf));

  /* Set autoconfig disable */

  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_AUTOCONFIG_DISABLE),
		  !cfgGetUseAutoconfig(conf));
}

/* Extract various config */

void wguiExtractVariousConfig(HWND DlgHWND, cfg *conf) {
  cfgSetMeasureSpeed(conf, 
		     Button_GetCheck(GetDlgItem(DlgHWND,
						IDC_CHECK_VARIOUS_SPEED)));
  cfgSetScreenDrawLEDs(conf, 
		       Button_GetCheck(GetDlgItem(DlgHWND,
						  IDC_CHECK_VARIOUS_LED)));
  cfgSetUseAutoconfig(conf,
		      !Button_GetCheck(GetDlgItem(DlgHWND,
						  IDC_CHECK_AUTOCONFIG_DISABLE)));
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
  sprintf(stmp, "FELLOW%d", i);
  lvi.iItem = i;
  lvi.pszText = stmp;
  lvi.cchTextMax = strlen(stmp);
  lvi.iSubItem = 0;
  if (!add) ListView_SetItem(lvHWND, &lvi);
  else ListView_InsertItem(lvHWND, &lvi);
  lvi.pszText = hf->filename;
  lvi.cchTextMax = strlen(hf->filename);
  lvi.iSubItem = 1;
  ListView_SetItem(lvHWND, &lvi);
  sprintf(stmp, "%s", (hf->readonly) ? "R" : "RW");
  lvi.pszText = stmp;
  lvi.cchTextMax = strlen(stmp);
  lvi.iSubItem = 2;
  ListView_SetItem(lvHWND, &lvi);
  sprintf(stmp, "%d", hf->sectorspertrack);
  lvi.pszText = stmp;
  lvi.cchTextMax = strlen(stmp);
  lvi.iSubItem = 3;
  ListView_SetItem(lvHWND, &lvi);
  sprintf(stmp, "%d", hf->surfaces);
  lvi.pszText = stmp;
  lvi.cchTextMax = strlen(stmp);
  lvi.iSubItem = 4;
  ListView_SetItem(lvHWND, &lvi);
  sprintf(stmp, "%d", hf->reservedblocks);
  lvi.pszText = stmp;
  lvi.cchTextMax = strlen(stmp);
  lvi.iSubItem = 5;
  ListView_SetItem(lvHWND, &lvi);
  sprintf(stmp, "%d", hf->bytespersector);
  lvi.pszText = stmp;
  lvi.cchTextMax = strlen(stmp);
  lvi.iSubItem = 6;
  ListView_SetItem(lvHWND, &lvi);
}

/* Install hardfile config */

#define HARDFILE_COLS 7
void wguiInstallHardfileConfig(HWND DlgHWND, cfg *conf) {
  LV_COLUMN lvc;
  HWND lvHWND = GetDlgItem(DlgHWND, IDC_LIST_HARDFILES);
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
    lvc.cchTextMax = strlen(colheads[i]);
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

void wguiExtractHardfileConfig(HWND DlgHWND, cfg *conf) {
}


/* Execute hardfile add or edit data */

cfg_hardfile *wgui_current_hardfile_edit = NULL;
ULO wgui_current_hardfile_edit_index = 0;

/* Run a hardfile edit or add dialog */

BOOLE wguiHardfileAdd(HWND DlgHWND, 
		      cfg *conf, 
		      BOOLE add, 
		      ULO index,
		      cfg_hardfile *target) {
  wgui_current_hardfile_edit = target;
  wgui_current_hardfile_edit_index = index;
  if (add) cfgSetHardfileUnitDefaults(target);
  return DialogBox(win_drv_hInstance,
		   MAKEINTRESOURCE(IDD_HARDFILE_ADD),
		   DlgHWND,
		   wguiHardfileAddDialogProc) == IDOK;
}

BOOLE wguiHardfileCreate(HWND DlgHWND,
						 cfg *conf,
						 ULO index,
						 cfg_hardfile *target) {
  wgui_current_hardfile_edit = target;
  wgui_current_hardfile_edit_index = index;
  cfgSetHardfileUnitDefaults(target);
  return DialogBox(win_drv_hInstance,
		   MAKEINTRESOURCE(IDD_HARDFILE_CREATE),
		   DlgHWND,
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
  sprintf(stmp, "DH%d", i);
  lvi.iItem = i;
  lvi.pszText = stmp;
  lvi.cchTextMax = strlen(stmp);
  lvi.iSubItem = 0;
  if (!add) ListView_SetItem(lvHWND, &lvi);
  else ListView_InsertItem(lvHWND, &lvi);
  lvi.pszText = fs->volumename;
  lvi.cchTextMax = strlen(fs->volumename);
  lvi.iSubItem = 1;
  ListView_SetItem(lvHWND, &lvi);
  lvi.pszText = fs->rootpath;
  lvi.cchTextMax = strlen(fs->rootpath);
  lvi.iSubItem = 2;
  ListView_SetItem(lvHWND, &lvi);
  sprintf(stmp, "%s", (fs->readonly) ? "R" : "RW");
  lvi.pszText = stmp;
  lvi.cchTextMax = strlen(stmp);
  lvi.iSubItem = 3;
  ListView_SetItem(lvHWND, &lvi);
}

/* Install filesystem config */

#define FILESYSTEM_COLS 4
void wguiInstallFilesystemConfig(HWND DlgHWND, cfg *conf) {
  LV_COLUMN lvc;
  HWND lvHWND = GetDlgItem(DlgHWND, IDC_LIST_FILESYSTEMS);
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
    lvc.cchTextMax = strlen(colheads[i]);
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
  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_AUTOMOUNT_FILESYSTEMS),
		  cfgGetFilesystemAutomountDrives(conf));
}

/* Extract filesystem config */

void wguiExtractFilesystemConfig(HWND DlgHWND, cfg *conf) {
  cfgSetFilesystemAutomountDrives(conf,
				  Button_GetCheck(GetDlgItem(DlgHWND,
							     IDC_CHECK_AUTOMOUNT_FILESYSTEMS)));
}


/* Execute filesystem add or edit data */

cfg_filesys *wgui_current_filesystem_edit = NULL;
ULO wgui_current_filesystem_edit_index = 0;

/* Run a filesystem edit or add dialog */

BOOLE wguiFilesystemAdd(HWND DlgHWND, 
			cfg *conf, 
			BOOLE add, 
			ULO index,
			cfg_filesys *target) {
  wgui_current_filesystem_edit = target;
  if (add) cfgSetFilesystemUnitDefaults(target);
  wgui_current_filesystem_edit_index = index;
  return DialogBox(win_drv_hInstance,
		   MAKEINTRESOURCE(IDD_FILESYSTEM_ADD),
		   DlgHWND,
		   wguiFilesystemAddDialogProc) == IDOK;
}

/*============================================================================*/
/* display config                                                              */
/*============================================================================*/

/* install display config */

void wguiInstallDisplayConfig(HWND DlgHWND, cfg *conf) {
  HWND screenAreaSliderHWND					= GetDlgItem(DlgHWND, IDC_SLIDER_SCREEN_AREA);
	HWND screenAreaSliderTextHWND			= GetDlgItem(DlgHWND, IDC_STATIC_SCREEN_AREA);
  HWND frameSkippingSliderHWND			= GetDlgItem(DlgHWND, IDC_SLIDER_FRAME_SKIPPING);
  HWND colorBitsComboboxHWND				= GetDlgItem(DlgHWND, IDC_COMBO_COLOR_BITS);
	HWND fullscreenHWND								= GetDlgItem(DlgHWND, IDC_CHECK_FULLSCREEN);
	HWND verticalscalingHWND					= GetDlgItem(DlgHWND, IDC_CHECK_VERTICAL_SCALE);
	HWND horizontalscalingHWND				= GetDlgItem(DlgHWND, IDC_CHECK_HORIZONTAL_SCALE);
	HWND scanlinesHWND								= GetDlgItem(DlgHWND, IDC_CHECK_SCANLINES);
	HWND interlaceHWND								= GetDlgItem(DlgHWND, IDC_CHECK_INTERLACE);
	HWND multiplebuffersHWND					= GetDlgItem(DlgHWND, IDC_CHECK_MULTIPLE_BUFFERS);
	HWND frameSkippingSliderTextHWND	= GetDlgItem(DlgHWND, IDC_STATIC_FRAME_SKIPPING);
	
	
	ULO comboboxid;
  LON resindex, i;
  STR stmp[32];

	// match available resolutions with configuration
	pwgui_dm_match = wguiMatchResolution();

	// fill combobox for colorbit depth
	ComboBox_AddString(colorBitsComboboxHWND, "256 colors"); 
	comboboxid = 0;
	pwgui_dm->comboxbox8bitindex = comboboxid; comboboxid++; 
	if (pwgui_dm->numberof16bit > 0) { 
		ComboBox_AddString(colorBitsComboboxHWND, "high color (16 bit)"); 
		pwgui_dm->comboxbox16bitindex = comboboxid; comboboxid++; 
	}
  if (pwgui_dm->numberof24bit > 0) { 
		ComboBox_AddString(colorBitsComboboxHWND, "true color (24 bit)"); 
		pwgui_dm->comboxbox24bitindex = comboboxid; comboboxid++; 
	}
  if (pwgui_dm->numberof32bit > 0) { 
		ComboBox_AddString(colorBitsComboboxHWND, "true color (32 bit)"); 
		pwgui_dm->comboxbox32bitindex = comboboxid; comboboxid++; 
	}

	ComboBox_Enable(colorBitsComboboxHWND, TRUE);
	ComboBox_SetCurSel(colorBitsComboboxHWND, wguiGetComboboxIndexFromColorBits(pwgui_dm_match->colorbits));
	
	// add multiple buffer option
  Button_SetCheck(multiplebuffersHWND, cfgGetUseMultipleGraphicalBuffers(conf));
		
  // set fullscreen button check
	if (pwgui_dm_match->windowed) {
		// windowed 
		// colorbits can't be selected through WinFellow, desktop setting will be used
		ComboBox_SetCurSel(colorBitsComboboxHWND, wguiGetComboboxIndexFromColorBits(GetDeviceCaps(GetWindowDC(GetDesktopWindow()), BITSPIXEL)));
		ComboBox_Enable(colorBitsComboboxHWND, FALSE);
		Button_SetCheck(fullscreenHWND, FALSE);
		// disable multiplebuffers
		Button_Enable(multiplebuffersHWND, FALSE);
	} else {
		// fullscreen
		ComboBox_Enable(colorBitsComboboxHWND, TRUE);
		Button_SetCheck(fullscreenHWND, TRUE);
		// enable the checkbox for multiplebuffers
		Button_Enable(multiplebuffersHWND, TRUE);
	}

  // add horizontal pixel scale option
	Button_SetCheck(horizontalscalingHWND, cfgGetHorizontalScale(conf) == 2);

	// add vertical pixel scale option
	Button_SetCheck(verticalscalingHWND, cfgGetVerticalScale(conf) == 2);
	
	// add scanline option
	if (cfgGetScanlines(conf)) {
		Button_SetCheck(scanlinesHWND, TRUE);
		Button_SetCheck(verticalscalingHWND, FALSE);
	} else {
		Button_SetCheck(scanlinesHWND, FALSE);
	}

	// add interlace compensation option
  Button_SetCheck(interlaceHWND, cfgGetDeinterlace(conf));

  // add screen area 
	if (pwgui_dm_match->windowed) {
		// windowed
		wguiSetSliderRange(screenAreaSliderHWND, 0, (pwgui_dm->numberofwindowed - 1));
	} else {
		switch (pwgui_dm_match->colorbits) {
			case 8:
				wguiSetSliderRange(screenAreaSliderHWND, 0, (pwgui_dm->numberof8bit - 1));
				break;
			case 16:
				wguiSetSliderRange(screenAreaSliderHWND, 0, (pwgui_dm->numberof16bit - 1));
				break;
			case 24:
				wguiSetSliderRange(screenAreaSliderHWND, 0, (pwgui_dm->numberof24bit - 1));
				break;
			case 32:
				wguiSetSliderRange(screenAreaSliderHWND, 0, (pwgui_dm->numberof32bit - 1));
				break;
		}
	}
	wguiSetSliderPosition(screenAreaSliderHWND, pwgui_dm_match->id);
	wguiSetSliderTextAccordingToPosition(screenAreaSliderHWND, screenAreaSliderTextHWND, &wguiGetResolutionStrWithIndex);

  // add frame skipping rate choices 
  wguiSetSliderRange(frameSkippingSliderHWND, 0, 24);
	wguiSetSliderPosition(frameSkippingSliderHWND, cfgGetFrameskipRatio(conf));
	wguiSetSliderTextAccordingToPosition(frameSkippingSliderHWND, frameSkippingSliderTextHWND, &wguiGetFrameSkippingStrWithIndex);

	// add blitter selection radio buttons
	wguiInstallBlitterConfig(DlgHWND, conf);
}

/* extract display config */

void wguiExtractDisplayConfig(HWND DlgHWND, cfg *conf) {
  HWND screenAreaSliderHWND					= GetDlgItem(DlgHWND, IDC_SLIDER_SCREEN_AREA);
	HWND screenAreaSliderTextHWND			= GetDlgItem(DlgHWND, IDC_STATIC_SCREEN_AREA);
  HWND frameSkippingSliderHWND			= GetDlgItem(DlgHWND, IDC_SLIDER_FRAME_SKIPPING);
  HWND colorBitsComboboxHWND				= GetDlgItem(DlgHWND, IDC_COMBO_COLOR_BITS);
	HWND fullscreenHWND								= GetDlgItem(DlgHWND, IDC_CHECK_FULLSCREEN);
	HWND verticalscalingHWND					= GetDlgItem(DlgHWND, IDC_CHECK_VERTICAL_SCALE);
	HWND horizontalscalingHWND				= GetDlgItem(DlgHWND, IDC_CHECK_HORIZONTAL_SCALE);
	HWND scanlinesHWND								= GetDlgItem(DlgHWND, IDC_CHECK_SCANLINES);
	HWND interlaceHWND								= GetDlgItem(DlgHWND, IDC_CHECK_INTERLACE);
	HWND multiplebuffersHWND					= GetDlgItem(DlgHWND, IDC_CHECK_MULTIPLE_BUFFERS);
	HWND frameSkippingSliderTextHWND	= GetDlgItem(DlgHWND, IDC_STATIC_FRAME_SKIPPING);
		
	ULO comboboxid;
  LON resindex, i;
  STR stmp[32];

	// get current colorbits
	cfgSetScreenColorBits(conf, wguiGetColorBitsFromComboboxIndex(wguiGetComboboxCurrentSelection(colorBitsComboboxHWND)));
	
	// get multiplebuffer check
	cfgSetUseMultipleGraphicalBuffers(conf, Button_GetCheck(multiplebuffersHWND));

	// get fullscreen check
	cfgSetScreenWindowed(conf, !Button_GetCheck(fullscreenHWND));

	// get scaling
  cfgSetVerticalScale(conf, (Button_GetCheck(verticalscalingHWND)) ? 2 : 1);
  cfgSetHorisontalScale(conf, (Button_GetCheck(horizontalscalingHWND)) ? 2 : 1);
	cfgSetScanlines(conf, Button_GetCheck(scanlinesHWND));
	cfgSetDeinterlace(conf, Button_GetCheck(interlaceHWND));

	// get height and width
	if (cfgGetScreenWindowed(conf)) {
		cfgSetScreenWidth(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->reswindowed, wguiGetSliderPosition(screenAreaSliderHWND))))->width);
		cfgSetScreenHeight(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->reswindowed, wguiGetSliderPosition(screenAreaSliderHWND))))->height);
	} else {
		switch(cfgGetScreenColorBits(conf)) {
			case 8:
				cfgSetScreenWidth(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res8bit, wguiGetSliderPosition(screenAreaSliderHWND))))->width);
				cfgSetScreenHeight(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res8bit, wguiGetSliderPosition(screenAreaSliderHWND))))->height);
				break;
			case 16:
				cfgSetScreenWidth(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res16bit, wguiGetSliderPosition(screenAreaSliderHWND))))->width);
				cfgSetScreenHeight(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res16bit, wguiGetSliderPosition(screenAreaSliderHWND))))->height);
				break;
			case 24:
				cfgSetScreenWidth(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res24bit, wguiGetSliderPosition(screenAreaSliderHWND))))->width);
				cfgSetScreenHeight(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res24bit, wguiGetSliderPosition(screenAreaSliderHWND))))->height);
				break;
			case 32:
				cfgSetScreenWidth(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res32bit, wguiGetSliderPosition(screenAreaSliderHWND))))->width);
				cfgSetScreenHeight(conf, ((wgui_drawmode *) listNode(listIndex(pwgui_dm->res32bit, wguiGetSliderPosition(screenAreaSliderHWND))))->height);
				break;
		}
	}
	
  // get frame skipping rate choice
  cfgSetFrameskipRatio(conf, wguiGetSliderPosition(frameSkippingSliderHWND));

	// get blitter selection radio buttons
	wguiExtractBlitterConfig(DlgHWND, conf);
}

/*============================================================================*/
/* Screen config                                                              */
/*============================================================================*/

/* Install screen config */

void wguiInstallScreenConfig(HWND DlgHWND, cfg *conf) {
  HWND ResChoice = GetDlgItem(DlgHWND, IDC_COMBO_SCREEN_RESOLUTION);
  HWND SkipChoice = GetDlgItem(DlgHWND, IDC_COMBO_SCREEN_FRAMESKIP);
  felist *reslist;
  LON resindex, i;
  STR stmp[32];

  /* Add resolution choices */

  resindex = -1;
  i = 0;
  for (reslist = drawGetModes();
       reslist != NULL; 
       reslist = listNext(reslist)) {
    draw_mode *dm = listNode(reslist);
    ComboBox_AddString(ResChoice, dm->name);
    if ((dm->width == cfgGetScreenWidth(conf)) &&
	(dm->height == cfgGetScreenHeight(conf)) &&
	(dm->windowed == cfgGetScreenWindowed(conf)) &&
	((dm->windowed) ||
	 ((!dm->windowed) &&
	  ((dm->bits == cfgGetScreenColorBits(conf)) &&
	   ((cfgGetScreenRefresh(conf) == 0) ||
	    (dm->refresh == cfgGetScreenRefresh(conf)))))))
      resindex = i;
    i++;
  }
  if (resindex == -1) resindex = 0;
  ComboBox_SetCurSel(ResChoice, resindex);

  /* Add frameskip rate choices */

  for (i = 1; i <= 25; i++) {
    sprintf(stmp, "1 / %d", i);
    ComboBox_AddString(SkipChoice, stmp);
  }
  ComboBox_SetCurSel(SkipChoice, cfgGetFrameskipRatio(conf));

  /* Add horisontal scale options */

  if (cfgGetHorizontalScale(conf) == 1)
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SCREEN_HSCALE_SINGLE), TRUE);
  else
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SCREEN_HSCALE_DOUBLE), TRUE);

  /* Add vertical scale options */

  if (cfgGetScanlines(conf))
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SCREEN_VSCALE_SCANLINES),
		    TRUE);
  else if (cfgGetVerticalScale(conf) == 1)
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SCREEN_VSCALE_SINGLE), TRUE);
  else
    Button_SetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SCREEN_VSCALE_DOUBLE), TRUE);
  Button_SetCheck(GetDlgItem(DlgHWND, IDC_CHECK_SCREEN_VSCALE_INTERLACE),
		  cfgGetDeinterlace(conf));
}

/* Extract screen config */

void wguiExtractScreenConfig(HWND DlgHWND, cfg *conf) {
  ULO resindex;
  draw_mode *resmode;
  BOOLE check;

  /* Get current resolution */

  resindex = ComboBox_GetCurSel(GetDlgItem(DlgHWND,
					   IDC_COMBO_SCREEN_RESOLUTION));
  resmode = (draw_mode *) listNode(listIndex(drawGetModes(), resindex));
  cfgSetScreenWidth(conf, resmode->width);
  cfgSetScreenHeight(conf, resmode->height);
  cfgSetScreenWindowed(conf, resmode->windowed);
  if (resmode->windowed) {
    cfgSetScreenRefresh(conf, 0);
    cfgSetScreenColorBits(conf, 0);
  }
  else {
    cfgSetScreenRefresh(conf, resmode->refresh);
    cfgSetScreenColorBits(conf, resmode->bits);
  }

  /* Get current frameskip */

  cfgSetFrameskipRatio(conf,
		       ComboBox_GetCurSel(GetDlgItem(DlgHWND,
					      IDC_COMBO_SCREEN_FRAMESKIP)));

  /* Get current horisontal scale */

  check = Button_GetCheck(GetDlgItem(DlgHWND, IDC_RADIO_SCREEN_HSCALE_SINGLE));
  cfgSetHorisontalScale(conf, (check) ? 1 : 2);

  /* Get current vertical scale */

  check = Button_GetCheck(GetDlgItem(DlgHWND,
				     IDC_RADIO_SCREEN_VSCALE_SCANLINES));
  if (check) {
    cfgSetVerticalScale(conf, 1);
    cfgSetScanlines(conf, TRUE);
  }
  else {
    check = Button_GetCheck(GetDlgItem(DlgHWND,
				       IDC_RADIO_SCREEN_VSCALE_SINGLE));
    cfgSetScanlines(conf, FALSE);
    cfgSetVerticalScale(conf, (check) ? 1 : 2);
  }
  cfgSetDeinterlace(conf, Button_GetCheck(GetDlgItem(DlgHWND,
					   IDC_CHECK_SCREEN_VSCALE_INTERLACE)));
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


/*============================================================================*/
/* Dialog Procedure for the CPU property sheet                                */
/*============================================================================*/

BOOL CALLBACK wguiCPUDialogProc(HWND hwndDlg,
				UINT uMsg,
				WPARAM wParam,
				LPARAM lParam) {
  switch (uMsg) {
    case WM_INITDIALOG:
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

void wguiSelectDiskImage(cfg *conf, HWND hwndDlg, HWND hwndEdit, ULO index) {
  STR filename[CFG_FILENAME_LENGTH];
  STR pathname[CFG_FILENAME_LENGTH];
  BYT *strpointer;


  if (wguiSelectFile(hwndDlg,
		     filename, 
		     CFG_FILENAME_LENGTH, 
		     "Select Diskimage", 
		     FSEL_ADF)) {
    cfgSetDiskImage(conf, index, filename);
	
	strpointer = strrchr(filename, '\\');
	strncpy(pathname, filename, strlen(filename) - strlen(strpointer));
	pathname[strlen(filename) - strlen(strpointer)] = '\0';
	cfgSetLastUsedDiskDir(conf, pathname);
	
    Edit_SetText(hwndEdit, cfgGetDiskImage(conf, index));
  }
}

BOOL CALLBACK wguiFloppyDialogProc(HWND hwndDlg,
				   UINT uMsg,
				   WPARAM wParam,
				   LPARAM lParam) {

  switch (uMsg) {
    case WM_INITDIALOG:
      wguiInstallFloppyConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
	switch (LOWORD(wParam)) {
	  case IDC_BUTTON_DF0_FILEDIALOG:
	    wguiSelectDiskImage(wgui_cfg,
				hwndDlg,
				GetDlgItem(hwndDlg, IDC_EDIT_DF0_IMAGENAME),
				0);
	    break;
	  case IDC_BUTTON_DF1_FILEDIALOG:
	    wguiSelectDiskImage(wgui_cfg,
				hwndDlg,
				GetDlgItem(hwndDlg, IDC_EDIT_DF1_IMAGENAME),
				1);
	    break;
	  case IDC_BUTTON_DF2_FILEDIALOG:
	    wguiSelectDiskImage(wgui_cfg,
				hwndDlg,
				GetDlgItem(hwndDlg, IDC_EDIT_DF2_IMAGENAME),
				2);
	    break;
	  case IDC_BUTTON_DF3_FILEDIALOG:
	    wguiSelectDiskImage(wgui_cfg,
				hwndDlg,
				GetDlgItem(hwndDlg, IDC_EDIT_DF3_IMAGENAME),
				3);
	    break;
	  case IDC_BUTTON_DF0_EJECT:
	    cfgSetDiskImage(wgui_cfg, 0, "");
	    Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_DF0_IMAGENAME),
		         cfgGetDiskImage(wgui_cfg, 0));
	    break;
	  case IDC_BUTTON_DF1_EJECT:
	    cfgSetDiskImage(wgui_cfg, 1, "");
	    Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_DF1_IMAGENAME),
		         cfgGetDiskImage(wgui_cfg, 1));
	    break;
	  case IDC_BUTTON_DF2_EJECT:
	    cfgSetDiskImage(wgui_cfg, 2, "");
	    Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_DF2_IMAGENAME),
		         cfgGetDiskImage(wgui_cfg, 2));
	    break;
	  case IDC_BUTTON_DF3_EJECT:
	    cfgSetDiskImage(wgui_cfg, 3, "");
	    Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_DF3_IMAGENAME),
		         cfgGetDiskImage(wgui_cfg, 3));
	    break;
	  default:
	    break;
	}      
      break;
    case WM_DESTROY:
      wguiExtractFloppyConfig(hwndDlg, wgui_cfg);
      break;
  }
  return FALSE;
}


/*============================================================================*/
/* Dialog Procedure for the memory property sheet                             */
/*============================================================================*/

BOOL CALLBACK wguiMemoryDialogProc(HWND hwndDlg,
				   UINT uMsg,
				   WPARAM wParam,
				   LPARAM lParam) {
  STR filename[CFG_FILENAME_LENGTH];
  STR pathname[CFG_FILENAME_LENGTH];
  BYT *strpointer;

  switch (uMsg) {
    case WM_INITDIALOG:
      wguiInstallMemoryConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
	switch (LOWORD(wParam)) {
	  case IDC_BUTTON_KICKSTART_FILEDIALOG:
	    if (wguiSelectFile(hwndDlg, 
			       filename, 
			       CFG_FILENAME_LENGTH,
			       "Select Kickstart ROM File", 
			       FSEL_ROM)) {
	      cfgSetKickImage(wgui_cfg, filename);
		  
		  // extract directory for ini-file
	      strpointer = strrchr(filename, '\\');
		  strncpy(pathname, filename, strlen(filename) - strlen(strpointer));
		  pathname[strlen(filename) - strlen(strpointer)] = '\0';
		  iniSetLastUsedKickImageDir(wgui_ini, pathname);

		  Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_KICKSTART), cfgGetKickImage(wgui_cfg));
	    }
	    break;
	  case IDC_BUTTON_KEYFILE_FILEDIALOG:
	    if (wguiSelectFile(hwndDlg, 
			       filename, 
			       CFG_FILENAME_LENGTH,
			       "Select Keyfile", 
			       FSEL_KEY)) {
	      cfgSetKey(wgui_cfg, filename);

		  // extract directory for ini-file
		  strpointer = strrchr(filename, '\\');
		  strncpy(pathname, filename, strlen(filename) - strlen(strpointer));
		  pathname[strlen(filename) - strlen(strpointer)] = '\0';
		  iniSetLastUsedKeyDir(wgui_ini, pathname);

	      Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_KEYFILE),
			   cfgGetKey(wgui_cfg));
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

ULO wguiGetNumberOfScreenAreas(ULO colorbits) {
						
	switch (colorbits) {
		case 8:
			return pwgui_dm->numberof8bit;
		case 16:
			return pwgui_dm->numberof16bit;
		case 24:
			return pwgui_dm->numberof24bit;
		case 32:
			return pwgui_dm->numberof32bit;
	}
	return pwgui_dm->numberof8bit;
}

BOOL CALLBACK wguiDisplayDialogProc(HWND hwndDlg,
				   UINT uMsg,
				   WPARAM wParam,
				   LPARAM lParam) {
	STR buffer[255];
	ULO position;
	ULO comboboxIndexColorBits;
	ULO selectedColorBits;
	ULO availableScreenAreas;

  switch (uMsg) {
    case WM_INITDIALOG:
      wguiInstallDisplayConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
			switch ((int) LOWORD(wParam)) {
				case IDC_CHECK_FULLSCREEN:
					switch (HIWORD(wParam)) {
						case BN_CLICKED:
							if (!Button_GetCheck(GetDlgItem(hwndDlg, IDC_CHECK_FULLSCREEN))) {
								// the checkbox was unchecked - going to windowed
								wguiSetSliderRange(GetDlgItem(hwndDlg, IDC_SLIDER_SCREEN_AREA), 0, (pwgui_dm->numberofwindowed - 1));					
								wguiSetSliderPosition(GetDlgItem(hwndDlg, IDC_SLIDER_SCREEN_AREA), 0);
								pwgui_dm_match = listNode(pwgui_dm->reswindowed);
								wguiSetSliderTextAccordingToPosition(GetDlgItem(hwndDlg, IDC_SLIDER_SCREEN_AREA), GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_AREA), &wguiGetResolutionStrWithIndex);
								ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_COMBO_COLOR_BITS), wguiGetComboboxIndexFromColorBits(GetDeviceCaps(GetWindowDC(GetDesktopWindow()), BITSPIXEL)));
								ComboBox_Enable(GetDlgItem(hwndDlg, IDC_COMBO_COLOR_BITS), FALSE);
								Button_Enable(GetDlgItem(hwndDlg, IDC_CHECK_MULTIPLE_BUFFERS), FALSE);
							} else {
								// the checkbox was checked 
								comboboxIndexColorBits = wguiGetComboboxCurrentSelection(GetDlgItem(hwndDlg, IDC_COMBO_COLOR_BITS));
								selectedColorBits = wguiGetColorBitsFromComboboxIndex(comboboxIndexColorBits);
								wguiSetSliderPosition(GetDlgItem(hwndDlg, IDC_SLIDER_SCREEN_AREA), 0);
								wguiSetSliderRange(GetDlgItem(hwndDlg, IDC_SLIDER_SCREEN_AREA), 0, (wguiGetNumberOfScreenAreas(selectedColorBits) - 1));					
								pwgui_dm_match = listNode(wguiGetMatchingList(FALSE, wguiGetColorBitsFromComboboxIndex(comboboxIndexColorBits)));
								wguiSetSliderTextAccordingToPosition(GetDlgItem(hwndDlg, IDC_SLIDER_SCREEN_AREA), GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_AREA), &wguiGetResolutionStrWithIndex);
								ComboBox_Enable(GetDlgItem(hwndDlg, IDC_COMBO_COLOR_BITS), TRUE);
								Button_Enable(GetDlgItem(hwndDlg, IDC_CHECK_MULTIPLE_BUFFERS), TRUE);
							}
							break;
					}
					break;
				case IDC_COMBO_COLOR_BITS:
					switch (HIWORD(wParam)) {
						case CBN_SELCHANGE:
							comboboxIndexColorBits = wguiGetComboboxCurrentSelection(GetDlgItem(hwndDlg, IDC_COMBO_COLOR_BITS));
							selectedColorBits = wguiGetColorBitsFromComboboxIndex(comboboxIndexColorBits);
							wguiSetSliderPosition(GetDlgItem(hwndDlg, IDC_SLIDER_SCREEN_AREA), 0);
							wguiSetSliderRange(GetDlgItem(hwndDlg, IDC_SLIDER_SCREEN_AREA), 0, (wguiGetNumberOfScreenAreas(selectedColorBits) - 1));					
							pwgui_dm_match = listNode(wguiGetMatchingList(!Button_GetCheck(GetDlgItem(hwndDlg, IDC_CHECK_FULLSCREEN)), wguiGetColorBitsFromComboboxIndex(comboboxIndexColorBits)));
							wguiSetSliderTextAccordingToPosition(GetDlgItem(hwndDlg, IDC_SLIDER_SCREEN_AREA), GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_AREA), &wguiGetResolutionStrWithIndex);
							break;
					}
					break;
				case IDC_CHECK_SCANLINES:
					switch (HIWORD(wParam)) {
						case BN_CLICKED:
							if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_CHECK_SCANLINES))) {
								// scanlines was checked
								Button_SetCheck(GetDlgItem(hwndDlg, IDC_CHECK_VERTICAL_SCALE), FALSE);
								Button_SetCheck(GetDlgItem(hwndDlg, IDC_CHECK_HORIZONTAL_SCALE), FALSE);
							} 
							break;
					}
					break;
				case IDC_CHECK_VERTICAL_SCALE:
					switch (HIWORD(wParam)) {
						case BN_CLICKED:
							if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_CHECK_VERTICAL_SCALE))) {
								// vertical scale was checked
								Button_SetCheck(GetDlgItem(hwndDlg, IDC_CHECK_SCANLINES), FALSE);
							} 
							break;
					}
					break;
				case IDC_CHECK_HORIZONTAL_SCALE:
					switch (HIWORD(wParam)) {
						case BN_CLICKED:
							if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_CHECK_HORIZONTAL_SCALE))) {
								// horizontal scale was checked
								Button_SetCheck(GetDlgItem(hwndDlg, IDC_CHECK_SCANLINES), FALSE);
							} 
							break;
					}
					break;
			}
      break;
		case WM_NOTIFY:
			switch ((int) wParam) {
			case IDC_SLIDER_SCREEN_AREA:
				wguiSetSliderTextAccordingToPosition(GetDlgItem(hwndDlg, IDC_SLIDER_SCREEN_AREA), GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_AREA), &wguiGetResolutionStrWithIndex);
				break;
			case IDC_SLIDER_FRAME_SKIPPING:
				wguiSetSliderTextAccordingToPosition(GetDlgItem(hwndDlg, IDC_SLIDER_FRAME_SKIPPING), GetDlgItem(hwndDlg, IDC_STATIC_FRAME_SKIPPING), &wguiGetFrameSkippingStrWithIndex);
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

BOOL CALLBACK wguiSoundDialogProc(HWND hwndDlg,
				  UINT uMsg,
				  WPARAM wParam,
				  LPARAM lParam) {
  long buffer_length;
  STR buffer[16];

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
	  buffer_length = SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_SOUND_BUFFER_LENGTH), TBM_GETPOS, 0, 0);
	  sprintf(buffer, "%d", buffer_length);
	  Static_SetText(GetDlgItem(hwndDlg, IDC_STATIC_BUFFER_LENGTH), buffer);
	  break;
  }
  return FALSE;
}


/*============================================================================*/
/* Dialog Procedure for the filesystem property sheet                         */
/*============================================================================*/

BOOL CALLBACK wguiFilesystemAddDialogProc(HWND hwndDlg,
					  UINT uMsg,
					  WPARAM wParam,
					  LPARAM lParam) {
  switch (uMsg) {
    case WM_INITDIALOG:
      {
	STR stmp[16];
	Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_FILESYSTEM_ADD_VOLUMENAME),
		     wgui_current_filesystem_edit->volumename);
	Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_FILESYSTEM_ADD_ROOTPATH),
		     wgui_current_filesystem_edit->rootpath);
	Button_SetCheck(GetDlgItem(hwndDlg, IDC_CHECK_FILESYSTEM_ADD_READONLY),
			wgui_current_filesystem_edit->readonly);
      }
      return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
	switch (LOWORD(wParam)) {
          case IDC_BUTTON_FILESYSTEM_ADD_DIRDIALOG:
	    if (wguiSelectDirectory(hwndDlg, 
				    wgui_current_filesystem_edit->rootpath, 
				    CFG_FILENAME_LENGTH,
				    "Select Filesystem Root")) {
	      Edit_SetText(GetDlgItem(hwndDlg,
				      IDC_EDIT_FILESYSTEM_ADD_ROOTPATH),
			   wgui_current_filesystem_edit->rootpath);
	    }
	    break;
	  case IDOK:
	  {
	    Edit_GetText(GetDlgItem(hwndDlg,
				    IDC_EDIT_FILESYSTEM_ADD_VOLUMENAME),
			 wgui_current_filesystem_edit->volumename,
			 64);
	    if (wgui_current_filesystem_edit->volumename[0] == '\0') {
	      MessageBox(hwndDlg,
			 "You must specify a volume name",
			 "Edit Filesystem",
			 0);
	      break;
	    }
	    Edit_GetText(GetDlgItem(hwndDlg,
				    IDC_EDIT_FILESYSTEM_ADD_ROOTPATH),
			 wgui_current_filesystem_edit->rootpath,
			 CFG_FILENAME_LENGTH);
	    if (wgui_current_filesystem_edit->rootpath[0] == '\0') {
	      MessageBox(hwndDlg,
			 "You must specify a root path",
			 "Edit Filesystem",
			 0);
	      break;
	    }
	    wgui_current_filesystem_edit->readonly =
	            Button_GetCheck(GetDlgItem(hwndDlg,
					    IDC_CHECK_FILESYSTEM_ADD_READONLY));
	  }
	  case IDCANCEL:
	    EndDialog(hwndDlg, LOWORD(wParam));
	    return TRUE;
	}
      break;
    }
  return FALSE;
}


BOOL CALLBACK wguiFilesystemDialogProc(HWND hwndDlg,
				       UINT uMsg,
				       WPARAM wParam,
				       LPARAM lParam) {
  switch (uMsg) {
    case WM_INITDIALOG:
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
		int i;
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

BOOL CALLBACK wguiHardfileCreateDialogProc(HWND hwndDlg,
					UINT uMsg,
					WPARAM wParam,
					LPARAM lParam) {
  switch (uMsg) {
    case WM_INITDIALOG:
      {
		Edit_SetText(GetDlgItem(hwndDlg, IDC_CREATE_HARDFILE_NAME), "");
		Edit_SetText(GetDlgItem(hwndDlg, IDC_CREATE_HARDFILE_SIZE), "0");
	  }
	return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
		switch (LOWORD(wParam)) {
		  case IDOK:
			{
				STR stmp[32];
				fhfile_dev hfile;
				BYT *strpointer;
				STR fname[CFG_FILENAME_LENGTH];
				
				Edit_GetText(GetDlgItem(hwndDlg, IDC_CREATE_HARDFILE_NAME),
					hfile.filename, 256);

				strncpy(fname, hfile.filename, CFG_FILENAME_LENGTH);
				strupr(fname);
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
				Edit_GetText(GetDlgItem(hwndDlg, IDC_CREATE_HARDFILE_SIZE),
					stmp, 32);
				hfile.size = atoi(stmp);

				if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_CHECK_CREATE_HARDFILE_MEGABYTES)) == TRUE) {
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
		}
		break;
    }
  return FALSE;
}
	
BOOL CALLBACK wguiHardfileAddDialogProc(HWND hwndDlg,
					UINT uMsg,
					WPARAM wParam,
					LPARAM lParam) {
  switch (uMsg) {
    case WM_INITDIALOG:
      {
	STR stmp[16];
	sprintf(stmp, "%d", wgui_current_hardfile_edit_index);
	Static_SetText(GetDlgItem(hwndDlg, IDC_STATIC_HARDFILE_ADD_UNIT), stmp);
	Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_HARDFILE_ADD_FILENAME),
		     wgui_current_hardfile_edit->filename);
	sprintf(stmp, "%d", wgui_current_hardfile_edit->sectorspertrack);
	Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_HARDFILE_ADD_SECTORS), stmp);
	sprintf(stmp, "%d", wgui_current_hardfile_edit->surfaces);
	Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_HARDFILE_ADD_SURFACES), stmp);
	sprintf(stmp, "%d", wgui_current_hardfile_edit->reservedblocks);
	Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_HARDFILE_ADD_RESERVED), stmp);
	sprintf(stmp, "%d", wgui_current_hardfile_edit->bytespersector);
	Edit_SetText(GetDlgItem(hwndDlg, 
				IDC_EDIT_HARDFILE_ADD_BYTES_PER_SECTOR),
		     stmp);
      }
      return TRUE;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
	switch (LOWORD(wParam)) {
          case IDC_BUTTON_HARDFILE_ADD_FILEDIALOG:
	    if (wguiSelectFile(hwndDlg, 
			       wgui_current_hardfile_edit->filename, 
			       CFG_FILENAME_LENGTH,
			       "Select Hardfile", 
			       FSEL_HDF)) {
	      Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_HARDFILE_ADD_FILENAME),
			   wgui_current_hardfile_edit->filename);
	    }
	    break;
	  case IDOK:
	  {
	    STR stmp[32];
	    Edit_GetText(GetDlgItem(hwndDlg, IDC_EDIT_HARDFILE_ADD_FILENAME),
			 wgui_current_hardfile_edit->filename,
			 256);
	    if (wgui_current_hardfile_edit->filename[0] == '\0') {
	      MessageBox(hwndDlg,
			 "You must specify a hardfile name",
			 "Edit Hardfile",
			 0);
	      break;
	    }
	    Edit_GetText(GetDlgItem(hwndDlg, IDC_EDIT_HARDFILE_ADD_SECTORS),
			 stmp,
			 32);
	    if (atoi(stmp) < 1) {
	      MessageBox(hwndDlg,
			 "Sectors Per Track must be 1 or higher",
			 "Edit Hardfile",
			 0);
	      break;
	    }
	    wgui_current_hardfile_edit->sectorspertrack = atoi(stmp);
	    Edit_GetText(GetDlgItem(hwndDlg, IDC_EDIT_HARDFILE_ADD_SURFACES),
			 stmp,
			 32);
	    if (atoi(stmp) < 1) {
	      MessageBox(hwndDlg,
			 "The number of surfaces must be 1 or higher",
			 "Edit Hardfile",
			 0);
	      break;
	    }
	    wgui_current_hardfile_edit->surfaces = atoi(stmp);
	    Edit_GetText(GetDlgItem(hwndDlg, IDC_EDIT_HARDFILE_ADD_RESERVED),
			 stmp,
			 32);
	    if (atoi(stmp) < 1) {
	      MessageBox(hwndDlg,
			 "The number of reserved blocks must be 1 or higher",
			 "Edit Hardfile",
			 0);
	      break;
	    }
	    wgui_current_hardfile_edit->reservedblocks = atoi((char *) stmp);
	    Edit_GetText(GetDlgItem(hwndDlg,
				    IDC_EDIT_HARDFILE_ADD_BYTES_PER_SECTOR),
			 stmp,
			 32);
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


BOOL CALLBACK wguiHardfileDialogProc(HWND hwndDlg,
				     UINT uMsg,
				     WPARAM wParam,
				     LPARAM lParam) {
  switch (uMsg) {
    case WM_INITDIALOG:
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
		int i;
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

BOOL CALLBACK wguiGameportDialogProc(HWND hwndDlg,
				     UINT uMsg,
				     WPARAM wParam,
				     LPARAM lParam) {
  HWND gpChoice[2];
  int i;

  gpChoice[0] = GetDlgItem(hwndDlg, IDC_COMBO_GAMEPORT1);
  gpChoice[1] = GetDlgItem(hwndDlg, IDC_COMBO_GAMEPORT2);
    
  switch (uMsg) {
    case WM_INITDIALOG:
      wguiInstallGameportConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
            if (wgui_action == WGUI_NO_ACTION)
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

BOOL CALLBACK wguiVariousDialogProc(HWND hwndDlg,
				    UINT uMsg,
				    WPARAM wParam,
				    LPARAM lParam)
{
  switch (uMsg) {
    case WM_INITDIALOG:
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
/* Dialog Procedure for the screen property sheet                             */
/*============================================================================*/

BOOL CALLBACK wguiScreenDialogProc(HWND hwndDlg,
				   UINT uMsg,
				   WPARAM wParam,
				   LPARAM lParam) {
  switch (uMsg) {
    case WM_INITDIALOG:
      wguiInstallScreenConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      break;
    case WM_DESTROY:
      wguiExtractScreenConfig(hwndDlg, wgui_cfg);
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

int wguiConfigurationDialog(void)
{
  int i;
  PROPSHEETPAGE propertysheets[PROP_SHEETS];
  PROPSHEETHEADER propertysheetheader;
  
  for (i = 0; i < PROP_SHEETS; i++)  {
    propertysheets[i].dwSize = sizeof(PROPSHEETPAGE);
    		if (wgui_propsheetICON[i] != 0) {
			propertysheets[i].dwFlags = PSP_USEHICON;
		} else {
			propertysheets[i].dwFlags = PSP_DEFAULT;
		}
    propertysheets[i].hInstance = win_drv_hInstance;
    propertysheets[i].pszTemplate = MAKEINTRESOURCE(wgui_propsheetRID[i]);
    propertysheets[i].hIcon = LoadIcon(win_drv_hInstance, MAKEINTRESOURCE(wgui_propsheetICON[i]));
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
  propertysheetheader.nStartPage = 3;
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

BOOL CALLBACK wguiAboutDialogProc(HWND hwndDlg,
			          UINT uMsg,
			          WPARAM wParam,
			          LPARAM lParam) {
  switch (uMsg) {
    case WM_INITDIALOG:
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

void wguiAbout(HWND DlgHWND) {
  DialogBox(win_drv_hInstance,
            MAKEINTRESOURCE(IDD_ABOUT),
            DlgHWND,
	    wguiAboutDialogProc);
}

  
/*============================================================================*/
/* DialogProc for our main Dialog                                             */
/*============================================================================*/

BOOL CALLBACK wguiDialogProc(HWND hwndDlg,
			     UINT uMsg,
			     WPARAM wParam,
			     LPARAM lParam) {
  switch (uMsg) {
    case WM_INITDIALOG:
	  SendMessage(hwndDlg, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) LoadIcon(win_drv_hInstance, MAKEINTRESOURCE(IDI_ICON_WINFELLOW)));
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
		wguiConfigurationDialog();
	    break;
	  case IDC_HARD_RESET:
	    fellowPreStartReset(TRUE);
			SendMessage(GetDlgItem(wgui_hDialog, IDC_IMAGE_POWER_LED_MAIN), 
				STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) LoadBitmap(win_drv_hInstance, 
				MAKEINTRESOURCE(IDB_POWER_LED_OFF)));
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
	    wguiSelectDiskImage(wgui_cfg,
				hwndDlg,
				GetDlgItem(hwndDlg, IDC_EDIT_DF0_IMAGENAME_MAIN),
				0);
	    break;
	  case IDC_BUTTON_DF1_FILEDIALOG_MAIN:
	    wguiSelectDiskImage(wgui_cfg,
				hwndDlg,
				GetDlgItem(hwndDlg, IDC_EDIT_DF1_IMAGENAME_MAIN),
				1);
	    break;
	  case IDC_BUTTON_DF2_FILEDIALOG_MAIN:
	    wguiSelectDiskImage(wgui_cfg,
				hwndDlg,
				GetDlgItem(hwndDlg, IDC_EDIT_DF2_IMAGENAME_MAIN),
				2);
	    break;
	  case IDC_BUTTON_DF3_FILEDIALOG_MAIN:
	    wguiSelectDiskImage(wgui_cfg,
				hwndDlg,
				GetDlgItem(hwndDlg, IDC_EDIT_DF3_IMAGENAME_MAIN),
				3);
	    break;
	  case IDC_BUTTON_DF0_EJECT_MAIN:
	    cfgSetDiskImage(wgui_cfg, 0, "");
	    Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_DF0_IMAGENAME_MAIN),
		         cfgGetDiskImage(wgui_cfg, 0));
	    break;
	  case IDC_BUTTON_DF1_EJECT_MAIN:
	    cfgSetDiskImage(wgui_cfg, 1, "");
	    Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_DF1_IMAGENAME_MAIN),
		         cfgGetDiskImage(wgui_cfg, 1));
	    break;
	  case IDC_BUTTON_DF2_EJECT_MAIN:
	    cfgSetDiskImage(wgui_cfg, 2, "");
	    Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_DF2_IMAGENAME_MAIN),
		         cfgGetDiskImage(wgui_cfg, 2));
	    break;
	  case IDC_BUTTON_DF3_EJECT_MAIN:
	    cfgSetDiskImage(wgui_cfg, 3, "");
	    Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_DF3_IMAGENAME_MAIN),
		         cfgGetDiskImage(wgui_cfg, 3));
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

BOOL wguiCheckEmulationNecessities(void) {
	return ((fopen(cfgGetKickImage(wgui_cfg), "rb")) != NULL);
}	

BOOLE wguiEnter(void) {
  BOOLE quit_emulator = FALSE;
  BOOLE debugger_start = FALSE;
  RECT dialogRect;

  do {
    MSG myMsg;
    BOOLE end_loop = FALSE;
    
    wgui_action = WGUI_NO_ACTION;
  
    wgui_hDialog = CreateDialog(win_drv_hInstance,
			        MAKEINTRESOURCE(IDD_MAIN),
			        NULL,
			        wguiDialogProc); 
	SetWindowPos(wgui_hDialog, NULL, iniGetMainWindowXPos(wgui_ini), iniGetMainWindowYPos(wgui_ini), -1, -1, SWP_NOSIZE);
    wguiStartupPost();
	wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
	

	// install history into menu
	wguiInstallHistoryIntoMenu();
    ShowWindow(wgui_hDialog, win_drv_nCmdShow);

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
		  wgui_action = WGUI_NO_ACTION;
		  break;
		case WGUI_LOAD_HISTORY0:
		  cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
          if (cfgLoadFromFilename(wgui_cfg, iniGetConfigurationHistoryFilename(wgui_ini, 0)) == FALSE) {
			wguiDeleteCfgFromHistory(0);
		  } else {
			iniSetCurrentConfigurationFilename(wgui_ini, iniGetConfigurationHistoryFilename(wgui_ini, 0));
		  }
		  wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
		  wgui_action = WGUI_NO_ACTION;
		  break;
		case WGUI_LOAD_HISTORY1:
		  cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
		  if (cfgLoadFromFilename(wgui_cfg, iniGetConfigurationHistoryFilename(wgui_ini, 1)) == FALSE) {
			wguiDeleteCfgFromHistory(1);
		  } else {
			iniSetCurrentConfigurationFilename(wgui_ini, iniGetConfigurationHistoryFilename(wgui_ini, 1));
			wguiPutCfgInHistoryOnTop(1);
		  } 
		  wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
		  wgui_action = WGUI_NO_ACTION;
		  break;
		case WGUI_LOAD_HISTORY2:
		  cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
		  if (cfgLoadFromFilename(wgui_cfg, iniGetConfigurationHistoryFilename(wgui_ini, 2)) == FALSE) {
			wguiDeleteCfgFromHistory(2);
		  } else {
			iniSetCurrentConfigurationFilename(wgui_ini, iniGetConfigurationHistoryFilename(wgui_ini, 2));
			wguiPutCfgInHistoryOnTop(2);
		  } 
		  wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
		  wgui_action = WGUI_NO_ACTION;
		  break;
		case WGUI_LOAD_HISTORY3:
		  cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
		  if (cfgLoadFromFilename(wgui_cfg, iniGetConfigurationHistoryFilename(wgui_ini, 3)) == FALSE) {
			wguiDeleteCfgFromHistory(3);
		  } else {
			iniSetCurrentConfigurationFilename(wgui_ini, iniGetConfigurationHistoryFilename(wgui_ini, 3));
			wguiPutCfgInHistoryOnTop(3);
		  } 
		  wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
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
    cfgSaveToFilename(wgui_cfg, iniGetCurrentConfigurationFilename(wgui_ini));
	
	// save main window position
	GetWindowRect(wgui_hDialog, &dialogRect);
	iniSetMainWindowPosition(wgui_ini, dialogRect.left, dialogRect.top);

    DestroyWindow(wgui_hDialog);
    if (!quit_emulator && debugger_start) {
      debugger_start = FALSE;
      wdbgDebugSessionRun(NULL);
    }
    else if (!quit_emulator) winDrvEmulationStart();
  } while (!quit_emulator);
  return quit_emulator;
}


/*============================================================================*/
/* Called at the start of Fellow execution                                    */
/*============================================================================*/

/*============================================================================*/
/* Called at the end of Fellow initialization                                 */
/*============================================================================*/

void wguiStartup(void) {
  wgui_cfg = cfgManagerGetCurrentConfig(&cfg_manager);
  wgui_ini = iniManagerGetCurrentInitdata(&ini_manager);
	wguiConvertDrawModeListToGuiDrawModes(pwgui_dm);
}

void wguiStartupPost(void) {

	wguiInstallFloppyMain(wgui_hDialog, wgui_cfg);
	if (wgui_emulation_state) {
		SendMessage(GetDlgItem(wgui_hDialog, IDC_IMAGE_POWER_LED_MAIN), 
			STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) LoadBitmap(win_drv_hInstance, 
			MAKEINTRESOURCE(IDB_POWER_LED_ON)));
	} else {
		SendMessage(GetDlgItem(wgui_hDialog, IDC_IMAGE_POWER_LED_MAIN), 
			STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) LoadBitmap(win_drv_hInstance, 
			MAKEINTRESOURCE(IDB_POWER_LED_OFF)));
	}

}

/*============================================================================*/
/* Called at the end of Fellow execution                                      */
/*============================================================================*/

void wguiShutdown(void) {
}


#endif /* WGUI */
