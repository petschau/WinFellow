/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Windows GUI code for debugger                                              */
/* Authors: Torsten Enderling (carfesh@gmx.net)                               */
/*          Petter Schau (peschau@online.no)                                  */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/
/*============================================================================*/
/* ChangeLog:                                                                 */
/* ----------                                                                 */
/* 2000/12/08:                                                                */
/* - CIA state added                                                          */
/* - memory dump added                                                        */
/* - fixed a bug which hung property sheets when launched the second time     */
/*   (the dialog procs must not destroy the dialog)                           */
/* - made the debugger a property sheet; it should use register               */
/*   tabs, but I think for the beginning they are too complicated to use;     */
/*   shall be fixed later                                                     */
/*                                                                            */
/* TODO:                                                                      */
/* -----                                                                      */
/* - how to use that PropSheet_CancelToClose(hwndDlg) ?                       */
/* - why isn't the bg-color updated on initialization of the window ?         */
/* DOS Fellow debugger functions still missing:                               */
/* cia, io, floppy, sound, copper, sprites, screen, events                    */
/*============================================================================*/

#include "defs.h"

#ifdef WGUI

#include <windef.h>
#include <windows.h>
#include <windowsx.h>

#include "gui_general.h"
#include "gui_debugger.h"

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
#include "cpudis.h"
#include "kbd.h"
#include "fmem.h"
#include "cia.h"

/*===============================*/
/* Handle of the main dialog box */
/*===============================*/

static HWND wdbg_hDialog;

#define WDBG_CPU_REGISTERS_X 24
#define WDBG_CPU_REGISTERS_Y 26
#define WDBG_DISASSEMBLY_X 16
#define WDBG_DISASSEMBLY_Y 96
#define WDBG_DISASSEMBLY_LINES 17
#define WDBG_DISASSEMBLY_INDENT 16

/*===================================*/
/* private variables for this module */
/*===================================*/

static ULO memory_padd = WDBG_DISASSEMBLY_LINES * 32;
static ULO memory_adress = 0;
static BOOLE memory_ascii = FALSE;

/*============*/
/* Prototypes */
/*============*/

BOOL CALLBACK wdbgCPUDialogProc(HWND hwndDlg, UINT uMsg,
				WPARAM wParam, LPARAM lParam);
BOOL CALLBACK wdbgMemoryDialogProc(HWND hwndDlg, UINT uMsg,
				   WPARAM wParam, LPARAM lParam);
BOOL CALLBACK wdbgCIADialogProc(HWND hwndDlg, UINT uMsg,
				WPARAM wParam, LPARAM lParam);

/*==============================================================*/
/* the various sheets of the main dialog and their dialog procs */
/*==============================================================*/

#define DEBUG_PROP_SHEETS 3

UINT wdbg_propsheetRID[DEBUG_PROP_SHEETS] = {
  IDD_DEBUG_CPU, IDD_DEBUG_MEMORY, IDD_DEBUG_CIA
    /*, IDD_..., */
};

typedef BOOL(CALLBACK * wdbgDlgProc) (HWND, UINT, WPARAM, LPARAM);

wdbgDlgProc wdbg_propsheetDialogProc[DEBUG_PROP_SHEETS] = {
  wdbgCPUDialogProc, wdbgMemoryDialogProc, wdbgCIADialogProc
};

/*===========================*/
/* debugger main dialog proc */
/*===========================*/

int wdbgSessionMainDialog(void)
{
  int i;
  PROPSHEETPAGE propertysheets[DEBUG_PROP_SHEETS];
  PROPSHEETHEADER propertysheetheader;

  for (i = 0; i < DEBUG_PROP_SHEETS; i++) {
    propertysheets[i].dwSize = sizeof(PROPSHEETPAGE);
    propertysheets[i].dwFlags = PSP_DEFAULT;
    propertysheets[i].hInstance = win_drv_hInstance;
    propertysheets[i].pszTemplate = MAKEINTRESOURCE(wdbg_propsheetRID[i]);
    propertysheets[i].hIcon = NULL;
    propertysheets[i].pszTitle = NULL;
    propertysheets[i].pfnDlgProc = wdbg_propsheetDialogProc[i];
    propertysheets[i].lParam = 0;
    propertysheets[i].pfnCallback = NULL;
    propertysheets[i].pcRefParent = NULL;
  }
  propertysheetheader.dwSize = sizeof(PROPSHEETHEADER);
  propertysheetheader.dwFlags =
    PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_NOAPPLYNOW;
  propertysheetheader.hwndParent = wdbg_hDialog;
  propertysheetheader.hInstance = win_drv_hInstance;
  propertysheetheader.hIcon =
    LoadIcon(win_drv_hInstance, MAKEINTRESOURCE(IDI_ICON_WINFELLOW));
  propertysheetheader.pszCaption = "WinFellow Debugger Session";
  propertysheetheader.nPages = DEBUG_PROP_SHEETS;
  propertysheetheader.nStartPage = 0;
  propertysheetheader.ppsp = propertysheets;
  propertysheetheader.pfnCallback = NULL;
  return PropertySheet(&propertysheetheader);
}

/*============================================================================*/
/* Updates the CPU state in the main window                                   */
/*============================================================================*/

STR *wdbgGetDataRegistersStr(STR * s)
{
  sprintf(s,
	  "D0: %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X :D7",
	  d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
  return s;
}

STR *wdbgGetAddressRegistersStr(STR * s)
{
  sprintf(s,
	  "A0: %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X :A7",
	  a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
  return s;
}

STR *wdbgGetSpecialRegistersStr(STR * s)
{
  sprintf(s,
	  "USP:%.8X SSP:%.8X SR:%.4X FRAME: %d y: %d x: %d",
	  (sr & 0x2000) ? usp : a[7],
	  (sr & 0x2000) ? a[7] : ssp,
	  sr, draw_frame_count, graph_raster_y, graph_raster_x);
  return s;
}

void wdbgExtendStringTo80Columns(STR * s)
{
  ULO i;
  BOOLE zero_found = FALSE;

  for (i = 0; i < 79; i++) {
    if (!zero_found)
      zero_found = (s[i] == '\0');
    if (zero_found)
      s[i] = ' ';
  }
  s[80] = '\0';
}

ULO wdbgLineOut(HDC hDC, STR * s, ULO x, ULO y)
{
  struct tagSIZE text_size;
  SetBkMode(hDC, OPAQUE);
  wdbgExtendStringTo80Columns(s);
  TextOut(hDC, x, y, s, strlen(s));
  GetTextExtentPoint32(hDC, s, strlen(s), (LPSIZE) & text_size);
  return y + text_size.cy;
}

STR *wdbgGetDisassemblyLineStr(STR * s, ULO * disasm_pc)
{
  *disasm_pc = disOpcode(*disasm_pc, s);
  return s;
}

void wdbgUpdateCPUState(HWND hwndDlg)
{
  STR s[128];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {
    ULO y = WDBG_CPU_REGISTERS_Y;
    ULO x = WDBG_CPU_REGISTERS_X;
    ULO disasm_pc;
    ULO i;
    HFONT myfont = CreateFont(8,
			      8,
			      0,
			      0,
			      FW_NORMAL,
			      FALSE,
			      FALSE,
			      FALSE,
			      DEFAULT_CHARSET,
			      OUT_DEFAULT_PRECIS,
			      CLIP_DEFAULT_PRECIS,
			      DEFAULT_QUALITY,
			      FF_DONTCARE | FIXED_PITCH,
			      "fixedsys");
    HBITMAP myarrow = LoadBitmap(win_drv_hInstance,
				 MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(s), x, y);
    x = WDBG_DISASSEMBLY_X;
    y = WDBG_DISASSEMBLY_Y;
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += WDBG_DISASSEMBLY_INDENT;
    disasm_pc = cpuGetPC(pc);
    for (i = 0; i < WDBG_DISASSEMBLY_LINES; i++)
      y = wdbgLineOut(hDC, wdbgGetDisassemblyLineStr(s, &disasm_pc), x, y);
    DeleteDC(hDC_image);
    DeleteObject(myarrow);
    DeleteObject(myfont);
    EndPaint(hwndDlg, &paint_struct);
  }
}

/*============================================================================*/
/* Updates the memory state                                                   */
/*============================================================================*/

void wdbgUpdateMemoryState(HWND hwndDlg)
{
  STR st[128];
  unsigned char k;
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    ULO y = WDBG_CPU_REGISTERS_Y;
    ULO x = WDBG_CPU_REGISTERS_X;
    ULO i, j;
    HFONT myfont = CreateFont(8,
			      8,
			      0,
			      0,
			      FW_NORMAL,
			      FALSE,
			      FALSE,
			      FALSE,
			      DEFAULT_CHARSET,
			      OUT_DEFAULT_PRECIS,
			      CLIP_DEFAULT_PRECIS,
			      DEFAULT_QUALITY,
			      FF_DONTCARE | FIXED_PITCH,
			      "fixedsys");

    HBITMAP myarrow = LoadBitmap(win_drv_hInstance,
				 MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(st), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(st), x, y);
    y = wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(st), x, y);
    x = WDBG_DISASSEMBLY_X;
    y = WDBG_DISASSEMBLY_Y;
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += WDBG_DISASSEMBLY_INDENT;

    for (i = 0; i < WDBG_DISASSEMBLY_LINES; i++) {
      if (memory_ascii) {
	sprintf(st, "%.6X %.8X%.8X %.8X%.8X ",
		(memory_adress + i * 16) & 0xffffff,
		fetl(memory_adress + i * 16 + 0),
		fetl(memory_adress + i * 16 + 4),
		fetl(memory_adress + i * 16 + 8),
		fetl(memory_adress + i * 16 + 12));

	for (j = 0; j < 16; j++) {
	  k = fetb(memory_adress + i * 16 + j) & 0xff;
	  if (k < 32)
	    st[j + 41] = '.';
	  else
	    st[j + 41] = k;
	}
	st[16 + 41] = '\0';
	y = wdbgLineOut(hDC, st, x, y);
      }
      else {
	sprintf(st, "%.6X %.8X%.8X %.8X%.8X %.8X%.8X %.8X%.8X",
		(memory_adress + i * 32) & 0xffffff,
		fetl(memory_adress + i * 32),
		fetl(memory_adress + i * 32 + 4),
		fetl(memory_adress + i * 32 + 8),
		fetl(memory_adress + i * 32 + 12),
		fetl(memory_adress + i * 32 + 16),
		fetl(memory_adress + i * 32 + 20),
		fetl(memory_adress + i * 32 + 24),
		fetl(memory_adress + i * 32 + 28));
	y = wdbgLineOut(hDC, st, x, y);
      }
    }

    DeleteDC(hDC_image);
    DeleteObject(myarrow);
    DeleteObject(myfont);
    EndPaint(hwndDlg, &paint_struct);
  }
}

/*============================================================================*/
/* Updates the CIA state                                                      */
/*============================================================================*/

void wdbgUpdateCIAState(HWND hwndDlg)
{
  STR s[128];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    ULO y = WDBG_CPU_REGISTERS_Y;
    ULO x = WDBG_CPU_REGISTERS_X;
    ULO i, txt_y;
    HFONT myfont = CreateFont(8,
			      8,
			      0,
			      0,
			      FW_NORMAL,
			      FALSE,
			      FALSE,
			      FALSE,
			      DEFAULT_CHARSET,
			      OUT_DEFAULT_PRECIS,
			      CLIP_DEFAULT_PRECIS,
			      DEFAULT_QUALITY,
			      FF_DONTCARE | FIXED_PITCH,
			      "fixedsys");

    HBITMAP myarrow = LoadBitmap(win_drv_hInstance,
				 MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(s), x, y);
    x = WDBG_DISASSEMBLY_X;
    y = WDBG_DISASSEMBLY_Y;
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += WDBG_DISASSEMBLY_INDENT;

    intena &= 0xffdf;
    intenar &= 0xffdf;

    for (i = 0; i < 2; i++) {
      sprintf(s, "Cia %s Registers:", (i == 0) ? "A" : "B");
      y = wdbgLineOut(hDC, s, x, y);

      sprintf(s, "CRA-%.2X CRB-%.2X IREQ-%.2X IMSK-%.2X SP-%.2X",
	      cia_cra[i], cia_crb[i], cia_icrreq[i], cia_icrmsk[i], cia_sp[i]);
      y = wdbgLineOut(hDC, s, x, y);

      sprintf(s, "EV-%.8X ALARM-%.8X", cia_ev[i], cia_evalarm[i]);
      y = wdbgLineOut(hDC, s, x, y);

      sprintf(s, "TA-%.4X TAHELP-%.8X TB-%.4X TBHELP-%.8X", cia_ta[i],
	      cia_taleft[i], cia_tb[i], cia_tbleft[i]);
      y = wdbgLineOut(hDC, s, x, y);

      sprintf(s, "TALATCH-%.4X TBLATCH-%.4X", cia_talatch[i], cia_tblatch[i]);
      y = wdbgLineOut(hDC, s, x, y);

      strcpy(s, "");
      y = wdbgLineOut(hDC, s, x, y);
    }

    if (cia_cra[i] & 1)
      strcpy(s, "Timer A started, ");
    else
      strcpy(s, "Timer A stopped, ");

    if (cia_cra[i] & 8)
      strcat(s, "One-shot mode");
    else
      strcat(s, "Continuous");

    y = wdbgLineOut(hDC, s, x, y);

    if (cia_crb[i] & 1)
      strcpy(s, "Timer B started, ");
    else
      strcpy(s, "Timer B stopped, ");

    if (cia_crb[i] & 8)
      strcat(s, "One-shot mode");
    else
      strcat(s, "Continuous");

    y = wdbgLineOut(hDC, s, x, y);

    DeleteDC(hDC_image);
    DeleteObject(myarrow);
    DeleteObject(myfont);
    EndPaint(hwndDlg, &paint_struct);
  }
}

/*============================================================================*/
/* DialogProc for our cpu dialog                                             */
/*============================================================================*/

BOOL CALLBACK wdbgCPUDialogProc(HWND hwndDlg,
				UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
    case WM_INITDIALOG:
      return TRUE;
    case WM_PAINT:
      wdbgUpdateCPUState(hwndDlg);
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
	case IDC_DEBUG_CPU_STEP:
	  if (winDrvDebugStart(DBG_STEP, hwndDlg)) {
	    InvalidateRect(hwndDlg, NULL, FALSE);
	    SetFocus(hwndDlg);
	  }
	  break;
	case IDC_DEBUG_CPU_STEP_OVER:
	  if (winDrvDebugStart(DBG_STEP_OVER, hwndDlg)) {
	    InvalidateRect(hwndDlg, NULL, FALSE);
	    SetFocus(hwndDlg);
	  }
	  break;
	case IDC_DEBUG_CPU_RUN:
	  if (winDrvDebugStart(DBG_RUN, hwndDlg)) {
	    InvalidateRect(hwndDlg, NULL, FALSE);
	    SetFocus(hwndDlg);
	  }
	  break;
	case IDC_DEBUG_CPU_BREAK:
	  fellowRequestEmulationStop();
	  InvalidateRect(hwndDlg, NULL, FALSE);
	  break;
	default:
	  break;
      }
      break;
    default:
      break;
  }
  return FALSE;
}

/*============================================================================*/
/* DialogProc for our memory dialog                                           */
/*============================================================================*/

BOOL CALLBACK wdbgMemoryDialogProc(HWND hwndDlg,
				   UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
    case WM_INITDIALOG:
      return TRUE;
    case WM_PAINT:
      wdbgUpdateMemoryState(hwndDlg);
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
	case IDC_DEBUG_MEMORY_UP:
	  memory_adress = (memory_adress - 32) & 0xffffff;
	  InvalidateRect(hwndDlg, NULL, FALSE);
	  SetFocus(hwndDlg);
	  break;
	case IDC_DEBUG_MEMORY_DOWN:
	  memory_adress = (memory_adress + 32) & 0xffffff;
	  InvalidateRect(hwndDlg, NULL, FALSE);
	  SetFocus(hwndDlg);
	  break;
	case IDC_DEBUG_MEMORY_PGUP:
	  memory_adress = (memory_adress - memory_padd) & 0xffffff;
	  InvalidateRect(hwndDlg, NULL, FALSE);
	  SetFocus(hwndDlg);
	  break;
	case IDC_DEBUG_MEMORY_PGDN:
	  memory_adress = (memory_adress + memory_padd) & 0xffffff;
	  InvalidateRect(hwndDlg, NULL, FALSE);
	  SetFocus(hwndDlg);
	  break;
	case IDC_DEBUG_MEMORY_ASCII:
	  memory_ascii = TRUE;
	  memory_padd = WDBG_DISASSEMBLY_LINES * 16;
	  InvalidateRect(hwndDlg, NULL, FALSE);
	  SetFocus(hwndDlg);
	  break;
	case IDC_DEBUG_MEMORY_HEX:
	  memory_ascii = FALSE;
	  memory_padd = WDBG_DISASSEMBLY_LINES * 32;
	  InvalidateRect(hwndDlg, NULL, FALSE);
	  SetFocus(hwndDlg);
	  break;
	default:
	  break;
      }
      break;
    default:
      break;
  }
  return FALSE;
}

/*============================================================================*/
/* DialogProc for our CIA dialog                                              */
/*============================================================================*/

BOOL CALLBACK wdbgCIADialogProc(HWND hwndDlg,
				UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
    case WM_INITDIALOG:
      return TRUE;
    case WM_PAINT:
      wdbgUpdateCIAState(hwndDlg);
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
	case IDC_DEBUG_MEMORY_UP:
	  break;
	default:
	  break;
      }
      break;
    default:
      break;
  }
  return FALSE;
}

/*============================================================================*/
/* Runs the debugger                                                          */
/*============================================================================*/

void wdbgDebugSessionRun(HWND parent)
{
  BOOLE quit_emulator = FALSE;
  BOOLE debugger_start = FALSE;

  /* The configuration has been activated, but we must prepare the modules */
  /* for emulation ourselves */

  wdbg_hDialog = parent;
  fellowEmulationStart();
  if (fellowGetPreStartReset())
    fellowHardReset();
  wdbgSessionMainDialog();
  fellowEmulationStop();
}

#endif /* WGUI */
