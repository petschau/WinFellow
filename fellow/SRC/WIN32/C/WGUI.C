/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Windows GUI code                                                           */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "defs.h"

#ifdef WGUI

#include <windows.h>
#include <windowsx.h>
#include "resource.h"

#include <commctrl.h>
#include <prsht.h>

#include "windrv.h"
#include "sound.h"
#include "cpu.h"
#include "listtree.h"
#include "gameport.h"
#include "fhfile.h"
#include "config.h"
#include "draw.h"
#include "wdbg.h"



HWND wgui_hDialog;                           /* Handle of the main dialog box */
HWND wgui_hSplash;                             /* Handle of the splash window */
BOOL wgui_splash_terminate;
BOOL wgui_splash_timeout;
HANDLE wgui_hSplash_thread;                /* Handle of splash control thread */
cfg *wgui_cfg;                                   /* GUI copy of configuration */


/*============================================================================*/
/* Flags for various global events                                            */
/*============================================================================*/

typedef enum {
  WGUI_NO_ACTION,
  WGUI_START_EMULATION,
  WGUI_QUIT_EMULATOR,
  WGUI_CONFIGURATION,
  WGUI_LOADCONFIGURATION,
  WGUI_SAVECONFIGURATIONAS,
  WGUI_SAVECONFIGURATION,
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

#define PROP_SHEETS 10

UINT wgui_propsheetRID[PROP_SHEETS] = {IDD_CPU,
				       IDD_FLOPPY,
				       IDD_MEMORY,
				       IDD_BLITTER,
				       IDD_SOUND,
				       IDD_SCREEN,
				       IDD_FILESYSTEM,
				       IDD_HARDFILE,
				       IDD_GAMEPORT,
				       IDD_VARIOUS};

typedef BOOL (CALLBACK *wguiDlgProc)(HWND, UINT, WPARAM, LPARAM);

wguiDlgProc wgui_propsheetDialogProc[PROP_SHEETS] = {wguiCPUDialogProc,
						     wguiFloppyDialogProc,
						     wguiMemoryDialogProc,
						     wguiBlitterDialogProc,
						     wguiSoundDialogProc,
						     wguiScreenDialogProc,
						     wguiFilesystemDialogProc,
						     wguiHardfileDialogProc,
						     wguiGameportDialogProc,
						     wguiVariousDialogProc};


/*============================================================================*/
/* Runs a session in the file requester                                       */
/*============================================================================*/

typedef enum {
  FSEL_ROM = 0,
  FSEL_ADF = 1,
  FSEL_KEY = 2,
  FSEL_HDF = 3
} SelectFileFlags;

BOOLE wguiSelectFile(HWND DlgHWND,
		     STR *filename, 
		     ULO filenamesize,
		     STR *title,
		     SelectFileFlags SelectFileType) {
  OPENFILENAME ofn;
  STR *filters = NULL;

  switch (SelectFileType) {
    case FSEL_ROM:
      filters = 
           "ROM-Images\0*.rom\0ADF Diskfiles\0*.adf;*.adz;*.adf.gz;*.dms\0\0\0";
      break;
    case FSEL_ADF:
      filters = "ADF Diskfiles\0*.adf;*.adz;*.adf.gz;*.dms\0\0\0";
      break;
    case FSEL_KEY:
      filters = "Key-files\0*.key\0\0\0";
      break;
    case FSEL_HDF:
      filters = "Hardfiles\0*.hdf\0\0\0";
      break;
  }

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
/* Install configuration in the GUI components                                */
/*============================================================================*/

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

  cfgSetSoundWAVDump(conf,
		     Button_GetCheck(GetDlgItem(DlgHWND,
						IDC_CHECK_SOUND_WAV)));
}


/*============================================================================*/
/* Gameport config                                                            */
/*============================================================================*/

/* Install gameport config */

void wguiInstallGameportConfig(HWND DlgHWND, cfg *conf) {
  HWND gpChoice[2];
  ULO gpindex, i;

  gpChoice[0] = GetDlgItem(DlgHWND, IDC_COMBO_GAMEPORT1);
  gpChoice[1] = GetDlgItem(DlgHWND, IDC_COMBO_GAMEPORT2);

  /* Set current gameport selections */

  for (i = 0; i < 2; i++) {
    ComboBox_AddString(gpChoice[i], "None");
    ComboBox_AddString(gpChoice[i], "Joystick on Keyboard 1");
    ComboBox_AddString(gpChoice[i], "Joystick on Keyboard 2");
    ComboBox_AddString(gpChoice[i], "Mouse");
    ComboBox_AddString(gpChoice[i], "Analog Joystick");
  
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
    }
    ComboBox_SetCurSel(gpChoice[i], gpindex);
  }
}

/* Extract gameport config */

void wguiExtractGameportConfig(HWND DlgHWND, cfg *conf) {
  HWND gpChoice[2];
  ULO gpsel, i;
  gameport_inputs gpt[5] = {GP_NONE,
			    GP_JOYKEY0,
			    GP_JOYKEY1,
			    GP_MOUSE0,
			    GP_ANALOG0};

  gpChoice[0] = GetDlgItem(DlgHWND, IDC_COMBO_GAMEPORT1);
  gpChoice[1] = GetDlgItem(DlgHWND, IDC_COMBO_GAMEPORT2);

  /* Get current gameport inputs */

  for (i = 0; i < 2; i++) {
    gpsel = ComboBox_GetCurSel(gpChoice[i]);
    if (gpsel > 4) gpsel = 0;
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
						  IDC_CHECK_VARIOUS_SPEED)));
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

  if (cfgGetHorisontalScale(conf) == 1)
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

  if (wguiSelectFile(hwndDlg,
		     filename, 
		     CFG_FILENAME_LENGTH, 
		     "Select Diskimage", 
		     FSEL_ADF)) {
    cfgSetDiskImage(conf, index, filename);
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
	      Edit_SetText(GetDlgItem(hwndDlg, IDC_EDIT_KICKSTART),
			   cfgGetKickImage(wgui_cfg));
	    }
	    break;
	  case IDC_BUTTON_KEYFILE_FILEDIALOG:
	    if (wguiSelectFile(hwndDlg, 
			       filename, 
			       CFG_FILENAME_LENGTH,
			       "Select Keyfile", 
			       FSEL_KEY)) {
	      cfgSetKey(wgui_cfg, filename);
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
  switch (uMsg) {
    case WM_INITDIALOG:
      wguiInstallSoundConfig(hwndDlg, wgui_cfg);
      return TRUE;
    case WM_COMMAND:
      break;
    case WM_DESTROY:
      wguiExtractSoundConfig(hwndDlg, wgui_cfg);
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

BOOL wguiConfigurationDialog(void)
{
  int i;
  PROPSHEETPAGE propertysheets[PROP_SHEETS];
  PROPSHEETHEADER propertysheetheader;
  
  for (i = 0; i < PROP_SHEETS; i++)  {
    propertysheets[i].dwSize = sizeof(PROPSHEETPAGE);
    propertysheets[i].dwFlags = PSP_DEFAULT;
    propertysheets[i].hInstance = win_drv_hInstance;
    propertysheets[i].pszTemplate = MAKEINTRESOURCE(wgui_propsheetRID[i]);
    propertysheets[i].hIcon = NULL;
    propertysheets[i].pszTitle = NULL;
    propertysheets[i].pfnDlgProc = wgui_propsheetDialogProc[i];
    propertysheets[i].lParam = 0;
    propertysheets[i].pfnCallback = NULL;
    propertysheets[i].pcRefParent = NULL;
  }
  propertysheetheader.dwSize = sizeof(PROPSHEETHEADER);
  propertysheetheader.dwFlags = PSH_PROPSHEETPAGE;
  propertysheetheader.hwndParent = NULL;
  propertysheetheader.hInstance = win_drv_hInstance;
  propertysheetheader.hIcon = NULL;
  propertysheetheader.pszCaption = "Fellow Configuration";
  propertysheetheader.nPages = PROP_SHEETS;
  propertysheetheader.nStartPage = 0;
  propertysheetheader.ppsp = propertysheets;
  propertysheetheader.pfnCallback = NULL;
  return PropertySheet(&propertysheetheader);
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
      return TRUE;
    case WM_COMMAND:
      if (wgui_action == WGUI_NO_ACTION)
	switch (LOWORD(wParam)) {
	  case IDC_START_EMULATION:
	    wgui_action = WGUI_START_EMULATION;
	    break;
	  case ID_FILE_SAVECONFIGURATION:
		wgui_action = WGUI_SAVECONFIGURATION;
		break;
	  case IDCANCEL:
	  case ID_FILE_QUIT:
	  case IDC_QUIT_EMULATOR:
	    wgui_action = WGUI_QUIT_EMULATOR;
	    break;
	  case IDC_CONFIGURATION:
	    wguiConfigurationDialog();
	    break;
	  case IDC_HARD_RESET:
	    fellowPreStartReset(TRUE);
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
  }
  return FALSE;
}
 

/*============================================================================*/
/* DialogProc for our splash dialog window                                    */
/*============================================================================*/

BOOL CALLBACK wguiSplashProc(HWND hwndDlg,
			     UINT uMsg,
			     WPARAM wParam,
			     LPARAM lParam) {
  switch (uMsg) {
    case WM_INITDIALOG:
      return TRUE;
    case WM_TIMER:
      if (wParam == 1) {                  /* 1 is the ID of our splash timer */
	wgui_splash_timeout = TRUE;
	return 0;
      }
  }
  return FALSE;
}


/*============================================================================*/
/* Splash window thread, runs for at least 5 seconds                          */
/*============================================================================*/

DWORD WINAPI wguiSplashThreadProc(LPVOID pParam) {
  MSG myMsg;
  
  wgui_hSplash = CreateDialog(win_drv_hInstance,
			      MAKEINTRESOURCE(IDD_SPLASH),
			      NULL,
			      wguiSplashProc); 
  ShowWindow(wgui_hSplash, win_drv_nCmdShow);
  SetTimer(wgui_hSplash, 1, 1500, NULL);
  while (!wgui_splash_terminate || !wgui_splash_timeout)
    if (GetMessage(&myMsg, wgui_hSplash, 0, 0))
      DispatchMessage(&myMsg);
  DestroyWindow(wgui_hSplash);
  return 0;
}


/*============================================================================*/
/* Show splash window                                                         */
/*============================================================================*/

void wguiSplashWindowShow(void) {
  DWORD dwThreadId;
  wgui_splash_terminate = FALSE;
  wgui_hSplash_thread = CreateThread(NULL,                /* Security attr */
				     0,                   /* Stack Size */
				     wguiSplashThreadProc,/* Thread proc */
				     NULL,	          /* Thread param */
				     0,                   /* Creation flags */
				     &dwThreadId);        /* ThreadId */
}


/*============================================================================*/
/* Remove splash window                                                       */
/*============================================================================*/

void wguiSplashWindowHide(void) {
  wgui_splash_terminate = TRUE;
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
  MessageBox(NULL, text, "Fellow Amiga Emulator", 0);
}


/*============================================================================*/
/* Runs the GUI                                                               */
/*============================================================================*/

BOOLE wguiEnter(void) {
  BOOLE quit_emulator = FALSE;
  BOOLE debugger_start = FALSE;
  do {
    MSG myMsg;
    BOOLE end_loop = FALSE;
    
    wgui_action = WGUI_NO_ACTION;
  
    wgui_hDialog = CreateDialog(win_drv_hInstance,
			        MAKEINTRESOURCE(IDD_MAIN),
			        NULL,
			        wguiDialogProc); 
    ShowWindow(wgui_hDialog, win_drv_nCmdShow);
    while (!end_loop) {
      if (GetMessage(&myMsg, wgui_hDialog, 0, 0))
        if (!IsDialogMessage(wgui_hDialog, &myMsg))
	  DispatchMessage(&myMsg);
      switch (wgui_action) {
        case WGUI_START_EMULATION:
	  end_loop = TRUE;
	  cfgManagerSetCurrentConfig(&cfg_manager, wgui_cfg);
	  fellowPreStartReset(fellowGetPreStartReset() |
			      cfgManagerConfigurationActivate(&cfg_manager));
	  break;
        case WGUI_QUIT_EMULATOR:
	  end_loop = TRUE;
	  quit_emulator = TRUE;
	  break;
		case WGUI_SAVECONFIGURATION:
			//cfgSaveCurrentConfig
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
    cfgSaveToFilename(wgui_cfg, "conf.txt");
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

void wguiPreStartup(void) {
  wguiSplashWindowShow();
}


/*============================================================================*/
/* Called at the end of Fellow initialization                                 */
/*============================================================================*/

void wguiStartup(void) {
  wguiSplashWindowHide();
  WaitForSingleObject(wgui_hSplash_thread, INFINITE);
  wgui_cfg = cfgManagerGetCurrentConfig(&cfg_manager);
  cfgLoadFromFilename(wgui_cfg, "conf.txt");
}


/*============================================================================*/
/* Called at the end of Fellow execution                                      */
/*============================================================================*/

void wguiShutdown(void) {
/*  cfgManagerFreeConfig(&cfg_manager, wgui_cfg);*/
}


#endif /* WGUI */
