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
/* 2000/12/10:                                                                */
/* - the mod-ripper now works                                                 */
/* 2000/12/09:                                                                */
/* - added the clipping variables from graph.c                                */
/* - floppy, copper, events, screen and sprite state added                    */
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
/* - verify sound, graph, sprite, copper                                      */
/*============================================================================*/

#include "defs.h"

#ifdef WGUI

#include <windef.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <prsht.h>

#include "gui_general.h"
#include "gui_debugger.h"

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
#include "floppy.h"
#include "blit.h"
#include "bus.h"
#include "copper.h"
#include "graph.h"
#include "sound.h"
#include "sprite.h"

/*=======================================================*/
/* external references not exported by the include files */
/*=======================================================*/

/* copper.c */
extern ULO copcon, cop1lc, cop2lc;
extern ULO copper_ptr, copper_next;

/* floppy.c */
extern ULO dsklen, dsksync, dskpt;

/* sprite.c */
extern ULO sprpt[8];
extern ULO sprite_ddf_kill;
extern ULO sprx[8];
extern ULO spry[8];
extern ULO sprly[8];
extern ULO spratt[8];
extern ULO sprite_state[8];

/* graph.c */
extern ULO ddfstrt, ddfstop;
extern ULO diwxleft, diwxright, diwytop, diwybottom;
extern ULO evenscroll, evenhiscroll, oddscroll, oddhiscroll;
extern ULO graph_DIW_first_visible;
extern ULO graph_DIW_last_visible;
extern ULO graph_DDF_start;
extern ULO draw_right;
extern ULO draw_top;
extern ULO draw_bottom;

/* sound.c */
extern soundStateFunc audstate[4];
extern ULO audlenw[4];
extern ULO audper[4];
extern ULO audvol[4];
extern ULO audlen[4];
extern ULO audpercounter[4];

#ifndef soundState1
#define soundState1 ASMRENAME(soundState1)
extern void soundState1(ULO channelno);
#endif
#ifndef soundState2
#define soundState2 ASMRENAME(soundState2)
extern void soundState2(ULO channelno);
#endif
#ifndef soundState3
#define soundState3 ASMRENAME(soundState3)
extern void soundState3(ULO channelno);
#endif
#ifndef soundState5
#define soundState5 ASMRENAME(soundState5)
extern void soundState5(ULO channelno);
#endif



/*===============================*/
/* Handle of the main dialog box */
/*===============================*/

static HWND wdbg_hDialog;

#define WDBG_CPU_REGISTERS_X 24
#define WDBG_CPU_REGISTERS_Y 26
#define WDBG_DISASSEMBLY_X 16
#define WDBG_DISASSEMBLY_Y 96
#define WDBG_DISASSEMBLY_LINES 20
#define WDBG_DISASSEMBLY_INDENT 16
#define WDBG_STRLEN 1024

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
BOOL CALLBACK wdbgFloppyDialogProc(HWND hwndDlg, UINT uMsg,
				   WPARAM wParam, LPARAM lParam);
BOOL CALLBACK wdbgCopperDialogProc(HWND hwndDlg, UINT uMsg,
				   WPARAM wParam, LPARAM lParam);
BOOL CALLBACK wdbgSpriteDialogProc(HWND hwndDlg, UINT uMsg,
				   WPARAM wParam, LPARAM lParam);
BOOL CALLBACK wdbgScreenDialogProc(HWND hwndDlg, UINT uMsg,
				   WPARAM wParam, LPARAM lParam);
BOOL CALLBACK wdbgEventDialogProc(HWND hwndDlg, UINT uMsg,
				  WPARAM wParam, LPARAM lParam);
BOOL CALLBACK wdbgSoundDialogProc(HWND hwndDlg, UINT uMsg,
				  WPARAM wParam, LPARAM lParam);

/*==============================================================*/
/* the various sheets of the main dialog and their dialog procs */
/*==============================================================*/

#define DEBUG_PROP_SHEETS 9

UINT wdbg_propsheetRID[DEBUG_PROP_SHEETS] = {
  IDD_DEBUG_CPU, IDD_DEBUG_MEMORY, IDD_DEBUG_CIA, IDD_DEBUG_FLOPPY,
  IDD_DEBUG_COPPER, IDD_DEBUG_SPRITES, IDD_DEBUG_SCREEN,
  IDD_DEBUG_EVENTS, IDD_DEBUG_SOUND
};

typedef BOOL(CALLBACK * wdbgDlgProc) (HWND, UINT, WPARAM, LPARAM);

wdbgDlgProc wdbg_propsheetDialogProc[DEBUG_PROP_SHEETS] = {
  wdbgCPUDialogProc, wdbgMemoryDialogProc, wdbgCIADialogProc,
  wdbgFloppyDialogProc, wdbgCopperDialogProc, wdbgSpriteDialogProc,
  wdbgScreenDialogProc, wdbgEventDialogProc, wdbgSoundDialogProc
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
/* helper functions                                                           */
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

/*============================================================================*/
/* Updates the CPU state                                                      */
/*============================================================================*/

void wdbgUpdateCPUState(HWND hwndDlg)
{
  STR s[WDBG_STRLEN];
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
  STR st[WDBG_STRLEN];
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
  STR s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    ULO y = WDBG_CPU_REGISTERS_Y;
    ULO x = WDBG_CPU_REGISTERS_X;
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
/* Updates the floppy state                                                   */
/*============================================================================*/

void wdbgUpdateFloppyState(HWND hwndDlg)
{
  STR s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    ULO y = WDBG_CPU_REGISTERS_Y;
    ULO x = WDBG_CPU_REGISTERS_X;
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

    for (i = 0; i < 4; i++) {
      sprintf(s, "DF%d:", i);
      y = wdbgLineOut(hDC, s, x, y);

      sprintf(s, "Track-%d Sel-%d Motor-%d Side-%d WP-%d Cached-%s",
	      floppy[i].track,
	      floppy[i].sel,
	      floppy[i].motor,
	      floppy[i].side,
	      floppy[i].writeprot, floppy[i].cached ? "No " : "Yes");
      y = wdbgLineOut(hDC, s, x, y);
    }

    sprintf(s, "Dskpt-%.6X dsklen-%.4X dsksync-%.4X", dskpt, dsklen, dsksync);
    y = wdbgLineOut(hDC, s, x, y);

    if (floppy_DMA_started)
      strcpy(s, "Disk DMA running");
    else
      strcpy(s, "Disk DMA stopped");
    y = wdbgLineOut(hDC, s, x, y);

    strcpy(s, "Transfer settings:");
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Drive: %d  Wordsleft: %d  Wait: %d  Dst: %X",
	    floppy_DMA.drive,
	    floppy_DMA.wordsleft, floppy_DMA.wait, floppy_DMA.dst);
    y = wdbgLineOut(hDC, s, x, y);

#ifdef ALIGN_CHECK
    sprintf(s, "Busloop: %X  eol: %X copemu: %X", checkadr, end_of_line,
	    copemu);
    y = wdbgLineOut(hDC, s, x, y);
    sprintf(s, "68000strt:%X busstrt:%X copperstrt:%X memorystrt:%X",
	    m68000start, busstart, copperstart, memorystart);
    y = wdbgLineOut(hDC, s, x, y);
    sprintf(s, "sndstrt:%X sbstrt:%X blitstrt:%X", soundstart, sblaststart,
	    blitstart);
    y = wdbgLineOut(hDC, s, x, y);
    sprintf(s, "drawstrt:%X graphemstrt:%X", drawstart, graphemstart);
    y = wdbgLineOut(hDC, s, x, y);
#endif

    DeleteDC(hDC_image);
    DeleteObject(myarrow);
    DeleteObject(myfont);
    EndPaint(hwndDlg, &paint_struct);
  }
}

/*============================================================================*/
/* Updates the copper state                                                   */
/*============================================================================*/

void wdbgUpdateCopperState(HWND hwndDlg)
{
  STR s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    ULO y = WDBG_CPU_REGISTERS_Y;
    ULO x = WDBG_CPU_REGISTERS_X;
    ULO i, list1, list2, atpc;
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

    list1 = (cop1lc & 0xfffffe);
    list2 = (cop2lc & 0xfffffe);

    /* @@@@@ changed register names ?
       atpc  = (curcopptr & 0xfffffe); */
    atpc = (copper_ptr & 0xfffffe);	/* Make sure debug doesn't trap odd ex */

    /* @@@@@ changed register names ?
       sprintf(s, "Cop1lc-%.6X Cop2lc-%.6X Copcon-%d Copper PC - %.6X", cop1lc, cop2lc, copcon, curcopptr); */
    sprintf(s, "Cop1lc-%.6X Cop2lc-%.6X Copcon-%d Copper PC - %.6X", cop1lc,
	    cop2lc, copcon, copper_ptr);
    y = wdbgLineOut(hDC, s, x, y);

    /* @@@@@ changed register names ? 
       sprintf(s, "Next cycle - %d  Y - %d  X - %d", nxtcopaccess, (nxtcopaccess != -1) ? (nxtcopaccess/228) : 0, (nxtcopaccess != -1) ? (nxtcopaccess % 228) : 0); */
    sprintf(s, "Next cycle - %d  Y - %d  X - %d", copper_next,
	    (copper_next != -1) ? (copper_next / 228) : 0,
	    (copper_next != -1) ? (copper_next % 228) : 0);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "List 1:        List 2:        At PC:");
    y = wdbgLineOut(hDC, s, x, y);

    for (i = 0; i < 16; i++) {
      sprintf(s, "$%.4X, $%.4X   $%.4X, $%.4X   $%.4X, $%.4X",
	      fetw(list1),
	      fetw(list1 + 2),
	      fetw(list2), fetw(list2 + 2), fetw(atpc), fetw(atpc + 2));
      y = wdbgLineOut(hDC, s, x, y);
      list1 += 4;
      list2 += 4;
      atpc += 4;
    }

    DeleteDC(hDC_image);
    DeleteObject(myarrow);
    DeleteObject(myfont);
    EndPaint(hwndDlg, &paint_struct);
  }
}

/*============================================================================*/
/* Updates the sprite state                                                   */
/*============================================================================*/

void wdbgUpdateSpriteState(HWND hwndDlg)
{
  STR s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    ULO y = WDBG_CPU_REGISTERS_Y;
    ULO x = WDBG_CPU_REGISTERS_X;
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

    sprintf(s, "Spr0pt-%.6X Spr1pt-%.6X Spr2pt-%.6X Spr3pt - %.6X", sprpt[0],
	    sprpt[1], sprpt[2], sprpt[3]);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Spr4pt-%.6X Spr5pt-%.6X Spr6pt-%.6X Spr7pt - %.6X", sprpt[4],
	    sprpt[5], sprpt[6], sprpt[7]);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "SpriteDDFkill-%X", sprite_ddf_kill);
    y = wdbgLineOut(hDC, s, x, y);

    for (i = 0; i < 8; i++) {
      sprintf(s,
	      "Spr%dX-%d Spr%dStartY-%d Spr%dStopY-%d Spr%dAttached-%d Spr%dstate-%d",
	      i, sprx[i], i, spry[i], i, sprly[i], i, spratt[i], i,
	      sprite_state[i]);
      y = wdbgLineOut(hDC, s, x, y);
    }

    DeleteDC(hDC_image);
    DeleteObject(myarrow);
    DeleteObject(myfont);
    EndPaint(hwndDlg, &paint_struct);
  }
}

/*============================================================================*/
/* Updates the screen state                                                   */
/*============================================================================*/

void wdbgUpdateScreenState(HWND hwndDlg)
{
  STR s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    ULO y = WDBG_CPU_REGISTERS_Y;
    ULO x = WDBG_CPU_REGISTERS_X;
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

    sprintf(s,
       "clipleftx-%.3d   cliprightx-%.3d         cliptop-%.3d           clipbot-%d",
       draw_left,
       draw_right,
       draw_top,
       draw_bottom);
    y = wdbgLineOut(hDC, s, x, y);

    /* @@@@@ changed register names ?
       sprintf(s,
       "DDFStartpos-%.3d DIWfirstvisiblepos-%.3d DIWlastvisiblepos-%.3d",
       DDFstartpos, DIWfirstvisiblepos, DIWlastvisiblepos); */
    sprintf(s,
	    "DDFStartpos-%.3d DIWfirstvisiblepos-%.3d DIWlastvisiblepos-%.3d",
	    graph_DDF_start, graph_DIW_first_visible, graph_DIW_last_visible);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s,
	    "diwxleft-%.3d    diwxright-%.3d          diwytop-%.3d           diwybot-%.3d",
	    diwxleft, diwxright, diwytop, diwybottom);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "ddfstrt-%.3d     ddfstop-%.3d", ddfstrt, ddfstop);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s,
	    "oddscroll-%.3d   oddhiscroll-%.3d        evenscroll-%.3d        evenhiscroll-%.3d",
	    oddscroll, oddhiscroll, evenscroll, evenhiscroll);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "intena-%.4X     intreq-%.4X", intena, intreq);
    y = wdbgLineOut(hDC, s, x, y);

    DeleteDC(hDC_image);
    DeleteObject(myarrow);
    DeleteObject(myfont);
    EndPaint(hwndDlg, &paint_struct);
  }
}

/*============================================================================*/
/* Updates the event state                                                    */
/*============================================================================*/

void wdbgUpdateEventState(HWND hwndDlg)
{
  STR s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    ULO y = WDBG_CPU_REGISTERS_Y;
    ULO x = WDBG_CPU_REGISTERS_X;
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

    sprintf(s, "Next Cpu      - %d", cpu_next);
    y = wdbgLineOut(hDC, s, x, y);

    /* @@@@@ changed register name ?
       sprintf(s, "Next Copper   - %d", nxtcopaccess); */
    sprintf(s, "Next Copper   - %d", copper_next);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next EOL      - %d", eol_next);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next Blitter  - %d", blitend);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next IRQ      - %d", irq_next);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next EOF      - %d", eof_next);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Lvl2 - %d", lvl2_next);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Lvl3 - %d", lvl3_next);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Lvl4 - %d", lvl4_next);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Lvl5 - %d", lvl5_next);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Lvl6 - %d", lvl6_next);
    y = wdbgLineOut(hDC, s, x, y);

    DeleteDC(hDC_image);
    DeleteObject(myarrow);
    DeleteObject(myfont);
    EndPaint(hwndDlg, &paint_struct);
  }
}

/*============================================================================*/
/* Updates the sound state                                                    */
/*============================================================================*/

void wdbgUpdateSoundState(HWND hwndDlg)
{
  STR s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    ULO y = WDBG_CPU_REGISTERS_Y;
    ULO x = WDBG_CPU_REGISTERS_X;
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

    sprintf(s, "A0: %X A1: %X A2: %X A3: %X A5: %X", soundState0, soundState1,
	    soundState2, soundState3, soundState5);
    y = wdbgLineOut(hDC, s, x, y);

    for (i = 0; i < 4; i++) {
      sprintf(s,
	      "Ch%i State: %2d Lenw: %5d Len: %5d per: %5d Pcnt: %5X Vol: %5d",
	      i,
	      (audstate[i] == soundState0) ? 0 :
	         (audstate[i] == soundState1) ? 1 :
	         (audstate[i] == soundState2) ? 2 :
	         (audstate[i] == soundState3) ? 3 :
	         (audstate[i] == soundState5) ? 5 : -1, audlenw[i],
	      audlen[i], audper[i], audpercounter[i], audvol[i]);
      y = wdbgLineOut(hDC, s, x, y);
    }

    sprintf(s, "dmacon: %X", dmacon);
    y = wdbgLineOut(hDC, s, x, y);

    DeleteDC(hDC_image);
    DeleteObject(myarrow);
    DeleteObject(myfont);
    EndPaint(hwndDlg, &paint_struct);
  }
}

/*============*/
/* MOD Ripper */
/*============*/

/* Module-Ripper functions */
static int mods_found = 0;	// no. of mods found

/* Saves mem from address start to address end in file *name */
static BOOLE wdbgSaveMem(char *name, ULO start, ULO end)
{
  ULO i;
  FILE *modfile;

  if ((modfile = fopen(name, "w+b")) == NULL)
    return FALSE;
  for (i = start; i <= end; i++)
    fputc(fetb(i), modfile);
  fclose(modfile);
  return TRUE;
}

static void wdbgScanModules(char *searchstring, int channels, HWND hWnd)
{
  ULO i, j;
  char file_name[38];		// File name for the module-file
  char dummy_string[80];
  char message[2048];
  char module_name[34];		// Name of the module
  int result;
  ULO start;			// address where the module starts
  ULO end;				// address where the module ends
  int sample_size;		// no. of all sample-data used in bytes
  int pattern_size;		// no. of all pattern-data used in bytes
  int song_length;		// how many patterns does the song play?
  int max_pattern;		// highest pattern used

  for (i = 0; i <= 0xffffff; i++) {
    if ((fetb(i + 0) == searchstring[0])
	&& (fetb(i + 1) == searchstring[1])
	&& (fetb(i + 2) == searchstring[2])
	&& (fetb(i + 3) == searchstring[3])
      ) {
      /* Searchstring found, now calc size */
      start = i - 0x438;
	  fellowAddLog("MODRIP: Matching ID for %s at 0x%06x.\n", searchstring, i);

      sample_size = 0;		/* Get SampleSize */
      for (j = 0; j <= 30; j++) {
	sample_size += (256 * fetb(start + 0x2a + j * 0x1e)
			+ fetb(start + 0x2b + j * 0x1e)) * 2;
      }

      song_length = fetb(start + 0x3b6);

      max_pattern = 0;		/* Scan for max. ammount of patterns */

      for (j = 0; j <= song_length; j++) {
	if (max_pattern < fetb(start + 0x3b8 + j)) {
	  max_pattern = fetb(start + 0x3b8 + j);
	}
      }

      pattern_size = (max_pattern + 1) * 64 * 4 * channels;
      end = start + sample_size + pattern_size + 0x43b;

      if ((end <= 0xffffff) && (end - start < 1000000)) {
	for (j = 0; j < 20; j++) {
	  module_name[j] = fetb(start + j);
	}
	module_name[20] = 0;	/* Get module name */

	/* Set filename for the module-file */
	if (strlen(module_name) > 2) {
	  strcpy(file_name, module_name);
	  strcat(file_name, ".mod");
	}
	else {
	  sprintf(file_name, "mod%d.mod", mods_found++);
	}

	sprintf(message, "Module found at $%X\n", start);
	sprintf(dummy_string, "Module name: %s\n", module_name);
	strcat(message, dummy_string);
	sprintf(dummy_string, "Module size: %d Bytes\n", end - start);
	strcat(message, dummy_string);
	sprintf(dummy_string, "Patterns used: %d\n", max_pattern);
	strcat(message, dummy_string);
	sprintf(dummy_string, "Save module as %s?", file_name);
	strcat(message, dummy_string);
	result = MessageBox(hWnd, message, "Module found.", MB_YESNO);

	if (result == IDYES) {
	  wdbgSaveMem(file_name, start, end);
	}
      }
    }
  }
}

/*============================================================================*/
/* Invokes the mod-ripping                                                    */
/*============================================================================*/

void wdbgModrip(HWND hwndDlg)
{
  HCURSOR BusyCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
  if(BusyCursor) SetCursor(BusyCursor);

  wdbgScanModules("M.K.", 4, hwndDlg);
  wdbgScanModules("4CHN", 4, hwndDlg);
  wdbgScanModules("6CHN", 6, hwndDlg);
  wdbgScanModules("8CHN", 8, hwndDlg);
  wdbgScanModules("FLT4", 4, hwndDlg);
  wdbgScanModules("FLT8", 8, hwndDlg);

  BusyCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
  if(BusyCursor) SetCursor(BusyCursor);

  MessageBox(hwndDlg, "Module Ripper finished.", "Finished.", MB_OK);

}

/*============================================================================*/
/* DialogProc for our CPU dialog                                              */
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
/* DialogProc for our floppy dialog                                           */
/*============================================================================*/

BOOL CALLBACK wdbgFloppyDialogProc(HWND hwndDlg,
				   UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
    case WM_INITDIALOG:
      return TRUE;
    case WM_PAINT:
      wdbgUpdateFloppyState(hwndDlg);
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
/* DialogProc for our copper dialog                                           */
/*============================================================================*/

BOOL CALLBACK wdbgCopperDialogProc(HWND hwndDlg,
				   UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
    case WM_INITDIALOG:
      return TRUE;
    case WM_PAINT:
      wdbgUpdateCopperState(hwndDlg);
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
/* DialogProc for our sprite dialog                                           */
/*============================================================================*/

BOOL CALLBACK wdbgSpriteDialogProc(HWND hwndDlg,
				   UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
    case WM_INITDIALOG:
      return TRUE;
    case WM_PAINT:
      wdbgUpdateSpriteState(hwndDlg);
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
/* DialogProc for our screen dialog                                           */
/*============================================================================*/

BOOL CALLBACK wdbgScreenDialogProc(HWND hwndDlg,
				   UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
    case WM_INITDIALOG:
      return TRUE;
    case WM_PAINT:
      wdbgUpdateScreenState(hwndDlg);
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
/* DialogProc for our event dialog                                            */
/*============================================================================*/

BOOL CALLBACK wdbgEventDialogProc(HWND hwndDlg,
				  UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
    case WM_INITDIALOG:
      return TRUE;
    case WM_PAINT:
      wdbgUpdateEventState(hwndDlg);
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
/* DialogProc for our sound dialog                                            */
/*============================================================================*/

BOOL CALLBACK wdbgSoundDialogProc(HWND hwndDlg,
				  UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
    case WM_INITDIALOG:
      return TRUE;
    case WM_PAINT:
      wdbgUpdateSoundState(hwndDlg);
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
	case IDC_DEBUG_MODRIP:
	  wdbgModrip(hwndDlg);
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
