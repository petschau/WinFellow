/* @(#) $Id: WDBG.C,v 1.26.2.6 2008-02-13 19:24:35 peschau Exp $ */
/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/*                                                                         */
/* Windows GUI code for debugger                                           */
/*                                                                         */
/* Authors: Torsten Enderling (carfesh@gmx.net)                            */
/*          Petter Schau (peschau@online.no)                               */
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

#include "defs.h"

#ifdef WGUI

#include <windef.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <prsht.h>

#include "gui_general.h"
#include "gui_debugger.h"

#include "fellow.h"
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
#include "modrip.h"
#include "blit.h"
#include "wgui.h"


/*=======================================================*/
/* external references not exported by the include files */
/*=======================================================*/

/* blit.c */
extern ULO bltcon, bltafwm, bltalwm, bltapt, bltbpt, bltcpt, bltdpt, bltamod, bltbmod;
extern ULO bltcmod, bltdmod, bltadat, bltbdat, bltcdat, bltzero;

/* copper.c */
extern ULO copcon, cop1lc, cop2lc;
extern ULO copper_ptr;

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

/* sound.c */
extern soundStateFunc audstate[4];
extern ULO audlenw[4];
extern ULO audper[4];
extern ULO audvol[4];
extern ULO audlen[4];
extern ULO audpercounter[4];

/*===============================*/
/* Handle of the main dialog box */
/*===============================*/

HWND wdbg_hDialog;

#define WDBG_CPU_REGISTERS_X 24
#define WDBG_CPU_REGISTERS_Y 26
#define WDBG_DISASSEMBLY_X 16
#define WDBG_DISASSEMBLY_Y 96
#define WDBG_DISASSEMBLY_LINES 20
#define WDBG_DISASSEMBLY_INDENT 16
#define WDBG_STRLEN 2048

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
BOOL CALLBACK wdbgBlitterDialogProc(HWND hwndDlg, UINT uMsg,
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

#define DEBUG_PROP_SHEETS 10

UINT wdbg_propsheetRID[DEBUG_PROP_SHEETS] = {
  IDD_DEBUG_CPU,     IDD_DEBUG_MEMORY,  IDD_DEBUG_CIA, 
  IDD_DEBUG_FLOPPY,  IDD_DEBUG_BLITTER, IDD_DEBUG_COPPER, 
  IDD_DEBUG_SPRITES, IDD_DEBUG_SCREEN,  IDD_DEBUG_EVENTS, 
  IDD_DEBUG_SOUND
};

typedef BOOL(CALLBACK * wdbgDlgProc) (HWND, UINT, WPARAM, LPARAM);

wdbgDlgProc wdbg_propsheetDialogProc[DEBUG_PROP_SHEETS] = {
  wdbgCPUDialogProc,    wdbgMemoryDialogProc,  wdbgCIADialogProc,
  wdbgFloppyDialogProc, wdbgBlitterDialogProc, wdbgCopperDialogProc, 
  wdbgSpriteDialogProc, wdbgScreenDialogProc,  wdbgEventDialogProc, 
  wdbgSoundDialogProc
};

/*===========================*/
/* debugger main dialog proc */
/*===========================*/

INT_PTR wdbgSessionMainDialog(void)
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
	  cpuGetDReg(0), cpuGetDReg(1), cpuGetDReg(2), cpuGetDReg(3), cpuGetDReg(4), cpuGetDReg(5), cpuGetDReg(6), cpuGetDReg(7));
  return s;
}

STR *wdbgGetAddressRegistersStr(STR * s)
{
  sprintf(s,
	  "A0: %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X :A7",
	  cpuGetAReg(0), cpuGetAReg(1), cpuGetAReg(2), cpuGetAReg(3), cpuGetAReg(4), cpuGetAReg(5), cpuGetAReg(6), cpuGetAReg(7));
  return s;
}

STR *wdbgGetSpecialRegistersStr(STR * s)
{
  sprintf(s,
	  "USP:%.8X SSP:%.8X SR:%.4X FRAME: %d y: %d x: %d",
	  (cpuGetSR() & 0x2000) ? cpu_usp : cpuGetAReg(7),
	  (cpuGetSR() & 0x2000) ? cpuGetAReg(7) : cpu_ssp,
	  cpuGetSR(), draw_frame_count, graph_raster_y, graph_raster_x);
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
  TextOut(hDC, x, y, s, (int) strlen(s));
  GetTextExtentPoint32(hDC, s, (int) strlen(s), (LPSIZE) & text_size);
  return y + text_size.cy;
}

STR *wdbgGetDisassemblyLineStr(STR * s, ULO * disasm_pc)
{
//  *disasm_pc = cpuDisOpcode(*disasm_pc, s);
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
    disasm_pc = cpuGetPC();
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
		memoryReadLong(memory_adress + i * 16 + 0),
		memoryReadLong(memory_adress + i * 16 + 4),
		memoryReadLong(memory_adress + i * 16 + 8),
		memoryReadLong(memory_adress + i * 16 + 12));

	for (j = 0; j < 16; j++) {
	  k = memoryReadByte(memory_adress + i * 16 + j) & 0xff;
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
		memoryReadLong(memory_adress + i * 32),
		memoryReadLong(memory_adress + i * 32 + 4),
		memoryReadLong(memory_adress + i * 32 + 8),
		memoryReadLong(memory_adress + i * 32 + 12),
		memoryReadLong(memory_adress + i * 32 + 16),
		memoryReadLong(memory_adress + i * 32 + 20),
		memoryReadLong(memory_adress + i * 32 + 24),
		memoryReadLong(memory_adress + i * 32 + 28));
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
      y++;
    }

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

      sprintf(s, "Track-%d Sel-%d Motor-%d Side-%d WP-%d",
	      floppy[i].track,
	      floppy[i].sel,
	      floppy[i].motor,
	      floppy[i].side,
	      floppy[i].writeprot);
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

    sprintf(s, "Wordsleft: %d  Wait: %d  Dst: %X",
	    floppy_DMA.wordsleft, floppy_DMA.wait, floppy_DMA.dskpt);
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
/* Updates the blitter state                                                  */
/*============================================================================*/

void wdbgUpdateBlitterState(HWND hwndDlg)
{
  STR s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    ULO y = WDBG_CPU_REGISTERS_Y;
    ULO x = WDBG_CPU_REGISTERS_X;
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

    sprintf(s, "bltcon:  %08X bltafwm: %08X bltalwm: %08X", 
      bltcon, bltafwm, bltalwm);
    y = wdbgLineOut(hDC, s, x, y);

	sprintf(s, "");
    y = wdbgLineOut(hDC, s, x, y);

	sprintf(s, "bltapt:  %08X bltbpt:  %08X bltcpt:  %08X bltdpt:  %08X ",
      bltapt, bltbpt, bltcpt, bltdpt);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "bltamod: %08X bltbmod: %08X bltcmod: %08X bltdmod: %08X ",
      bltamod, bltbmod, bltcmod, bltdmod);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "bltadat: %08X bltbdat: %08X bltcdat: %08X bltzero: %08X ",
      bltadat, bltbdat, bltcdat, bltzero);
    y = wdbgLineOut(hDC, s, x, y);

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

    atpc = (copper_ptr & 0xfffffe);	/* Make sure debug doesn't trap odd ex */

    sprintf(s, "Cop1lc-%.6X Cop2lc-%.6X Copcon-%d Copper PC - %.6X", cop1lc,
	    cop2lc, copcon, copper_ptr);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next cycle - %d  Y - %d  X - %d", copperEvent.cycle,
	    (copperEvent.cycle != -1) ? (copperEvent.cycle / 228) : 0,
	    (copperEvent.cycle != -1) ? (copperEvent.cycle % 228) : 0);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "List 1:        List 2:        At PC:");
    y = wdbgLineOut(hDC, s, x, y);

    for (i = 0; i < 16; i++) {
      sprintf(s, "$%.4X, $%.4X   $%.4X, $%.4X   $%.4X, $%.4X",
	      memoryReadWord(list1),
	      memoryReadWord(list1 + 2),
	      memoryReadWord(list2), memoryReadWord(list2 + 2), memoryReadWord(atpc), memoryReadWord(atpc + 2));
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
  STR s[128];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    ULO y = WDBG_CPU_REGISTERS_Y;
    ULO x = WDBG_CPU_REGISTERS_X;
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

    sprintf(s, "DIWSTRT - %.4X DIWSTOP - %.4X DDFSTRT - %.4X DDFSTOP - %.4X LOF     - %.4X",
	    diwstrt, diwstop, ddfstrt, ddfstop, lof);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "BPLCON0 - %.4X BPLCON1 - %.4X BPLCON2 - %.4X BPL1MOD - %.4X BPL2MOD - %.4X",
	    bplcon0, bplcon1, bplcon2, bpl1mod, bpl2mod);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "BPL1PT -%.6X BPL2PT -%.6X BPL3PT -%.6X BPL4PT -%.6X BPL5PT -%.6X",
            bpl1pt, bpl2pt, bpl3pt, bpl4pt, bpl5pt);
    y = wdbgLineOut(hDC, s, x, y);
      
    sprintf(s, "BPL6PT -%.6X DMACON  - %.4X", bpl6pt, dmacon);
    y = wdbgLineOut(hDC, s, x, y);
    y++;

    sprintf(s, "Host window clip envelope (Hor) (Ver): (%d, %d) (%d, %d)",
            draw_left, draw_right, draw_top, draw_bottom);
    y = wdbgLineOut(hDC, s, x, y);
    
    sprintf(s, "Even/odd scroll (lores/hires): (%d, %d) (%d, %d)",
            evenscroll, evenhiscroll, oddscroll, oddhiscroll);
    y = wdbgLineOut(hDC, s, x, y);
    
    sprintf(s, "Visible bitplane data envelope (Hor) (Ver): (%d, %d) (%d, %d)",
	    diwxleft, diwxright, diwytop, diwybottom);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "DDF (First output cylinder, length in words): (%d, %d)",
	    graph_DDF_start, graph_DDF_word_count);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "DIW (First visible pixel, last visible pixel + 1): (%d, %d)",
	    graph_DIW_first_visible, graph_DIW_last_visible);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Raster beam position (x, y): (%d, %d)",
	    graph_raster_x, graph_raster_y);
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

    sprintf(s, "Next Cpu      - %d", cpuEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next Copper   - %d", copperEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next EOL      - %d", eolEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next Blitter  - %d", blitterEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next IRQ      - %d", irqEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next EOF      - %d", eofEvent.cycle);
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

/*
    sprintf(s, "A0: %X A1: %X A2: %X A3: %X A5: %X", soundState0, soundState1,
	    soundState2, soundState3, soundState5);
    y = wdbgLineOut(hDC, s, x, y);
*/
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
/* DialogProc for our blitter dialog                                          */
/*============================================================================*/

BOOL CALLBACK wdbgBlitterDialogProc(HWND hwndDlg,
				   UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
    case WM_INITDIALOG:
	  Button_SetCheck(GetDlgItem(hwndDlg, IDC_DEBUG_LOGBLT), blitterGetOperationLog());
      return TRUE;
    case WM_PAINT:
      wdbgUpdateBlitterState(hwndDlg);
      break;
	case WM_DESTROY:
	  blitterSetOperationLog(Button_GetCheck(GetDlgItem(hwndDlg, IDC_DEBUG_LOGBLT)));
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
	  modripRIP();
	  break;
	case IDC_DEBUG_DUMPCHIP:
	  modripChipDump();
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

  if (wguiCheckEmulationNecessities()) {
    wdbg_hDialog = parent;
    fellowEmulationStart();
    if (fellowGetPreStartReset())
      fellowHardReset();
    wdbgSessionMainDialog();
    fellowEmulationStop();
  }
  else
    MessageBox(parent, "Specified KickImage does not exist", "Configuration Error", 0);
}


HWND wdeb_hDialog = 0;
ULO WDEB_DISASM_LINES = 54;

enum wdeb_actions
{
  WDEB_NO_ACTION,
  WDEB_INIT_DIALOG,
  WDEB_EXIT
};

enum wdeb_actions wdeb_action;

void wdebInitInstructionColumns()
{
  ULO i;
  LV_COLUMN lv_col;
  LV_ITEM lv_item;
  HWND hList=GetDlgItem(wdeb_hDialog, IDC_LST_INSTRUCTIONS);
  memset(&lv_col, 0, sizeof(lv_col));
  lv_col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

  lv_col.pszText = "Address";
  lv_col.cx = 64;
  lv_col.iSubItem = 0;
  ListView_InsertColumn(hList, 0, &lv_col);

  lv_col.pszText = "Opcode data";
  lv_col.cx = 128;
  lv_col.iSubItem = 1;
  ListView_InsertColumn(hList, 1, &lv_col);

  lv_col.pszText = "Instruction";
  lv_col.cx = 64;
  lv_col.iSubItem = 2;
  ListView_InsertColumn(hList, 2, &lv_col);

  lv_col.pszText = "Operands";
  lv_col.cx = 128;
  lv_col.iSubItem = 3;
  ListView_InsertColumn(hList, 3, &lv_col);

  memset(&lv_item, 0, sizeof(lv_item));
  lv_item.mask = LVIF_TEXT;
  for (i = 0; i < WDEB_DISASM_LINES; ++i)
  {
    lv_item.pszText = "x";
    lv_item.iItem = i;
    lv_item.iSubItem = 0;
    ListView_InsertItem(hList, &lv_item);

    lv_item.pszText = "y";
    lv_item.iItem = i;
    lv_item.iSubItem = 1;
    ListView_SetItem(hList, &lv_item);

    lv_item.pszText = "z";
    lv_item.iItem = i;
    lv_item.iSubItem = 2;
    ListView_SetItem(hList, &lv_item);
  }

}

STR *wdbg_registernames[] = 
{"D0","D1","D2","D3","D4","D5","D6","D7",
"A0","A1","A2","A3","A4","A5","A6","A7",
"PC","USP","SSP","MSP","ISP","VBR","SR"};

void wdebInitRegisterColumns()
{
  ULO i;
  LV_COLUMN lv_col;
  LV_ITEM lv_item;
  HWND hList=GetDlgItem(wdeb_hDialog, IDC_LST_REGISTERS);
  memset(&lv_col, 0, sizeof(lv_col));
  lv_col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

  lv_col.pszText = "Register";
  lv_col.cx = 64;
  lv_col.iSubItem = 0;
  ListView_InsertColumn(hList, 0, &lv_col);

  lv_col.pszText = "Value";
  lv_col.cx = 64;
  lv_col.iSubItem = 1;
  ListView_InsertColumn(hList, 1, &lv_col);

  memset(&lv_item, 0, sizeof(lv_item));
  lv_item.mask = LVIF_TEXT;
  for (i = 0; i < 23; ++i)
  {
    lv_item.pszText = wdbg_registernames[i];
    lv_item.iItem = i;
    lv_item.iSubItem = 0;
    ListView_InsertItem(hList, &lv_item);

    lv_item.pszText = "";
    lv_item.iItem = i;
    lv_item.iSubItem = 1;
    ListView_SetItem(hList, &lv_item);
  }
}

void wdebInitInfoColumns()
{
  LV_COLUMN lv_col;
  HWND hList=GetDlgItem(wdeb_hDialog, IDC_LST_INFO);
  memset(&lv_col, 0, sizeof(lv_col));
  lv_col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

  lv_col.pszText = "Name";
  lv_col.cx = 64;
  lv_col.iSubItem = 0;
  ListView_InsertColumn(hList, 0, &lv_col);

  lv_col.pszText = "Value";
  lv_col.cx = 64;
  lv_col.iSubItem = 1;
  ListView_InsertColumn(hList, 1, &lv_col);
}

void wdebUpdateInstructionColumns()
{
  ULO i;
  LV_ITEM lv_item;
  HWND hList=GetDlgItem(wdeb_hDialog, IDC_LST_INSTRUCTIONS);
  ULO disasm_pc = cpuGetPC();
  STR inst_address[256];
  STR inst_data[256];
  STR inst_name[256];
  STR inst_operands[256];

  memset(&lv_item, 0, sizeof(lv_item));
  lv_item.mask = LVIF_TEXT;
  for (i = 0; i < WDEB_DISASM_LINES; i++)
  {
    inst_address[0] = '\0';
    inst_data[0] = '\0';
    inst_name[0] = '\0';
    inst_operands[0] = '\0';
    disasm_pc = cpuDisOpcode(disasm_pc, inst_address, inst_data, inst_name, inst_operands);
    lv_item.pszText = inst_address;
    lv_item.iItem = i;
    lv_item.iSubItem = 0;
    ListView_SetItem(hList, &lv_item);
    lv_item.pszText = inst_data;
    lv_item.iItem = i;
    lv_item.iSubItem = 1;
    ListView_SetItem(hList, &lv_item);
    lv_item.pszText = inst_name;
    lv_item.iItem = i;
    lv_item.iSubItem = 2;
    ListView_SetItem(hList, &lv_item);
    lv_item.pszText = inst_operands;
    lv_item.iItem = i;
    lv_item.iSubItem = 3;
    ListView_SetItem(hList, &lv_item);
  }
}

void wdebUpdateRegisterColumns()
{
  ULO i, j, col;
  LV_ITEM lv_item;
  HWND hList=GetDlgItem(wdeb_hDialog, IDC_LST_REGISTERS);
  ULO disasm_pc = cpuGetPC();
  STR tmp[16];

  memset(&lv_item, 0, sizeof(lv_item));
  lv_item.mask = LVIF_TEXT;

  col = 0;

  for (j = 0; j < 2; j++)
  {
    for (i = 0; i < 8; i++)
    {
      sprintf(tmp, "%.8X", cpuGetReg(j, i));
      lv_item.pszText = tmp;
      lv_item.iItem = col++;
      lv_item.iSubItem = 1;
      ListView_SetItem(hList, &lv_item);
    }
  }
  sprintf(tmp, "%.8X", cpuGetPC());
  lv_item.pszText = tmp;
  lv_item.iItem = col++;
  lv_item.iSubItem = 1;
  ListView_SetItem(hList, &lv_item);

  sprintf(tmp, "%.8X", cpu_usp);
  lv_item.pszText = tmp;
  lv_item.iItem = col++;
  lv_item.iSubItem = 1;
  ListView_SetItem(hList, &lv_item);

  sprintf(tmp, "%.8X", cpu_ssp);
  lv_item.pszText = tmp;
  lv_item.iItem = col++;
  lv_item.iSubItem = 1;
  ListView_SetItem(hList, &lv_item);

  sprintf(tmp, "%.8X", cpu_msp);
  lv_item.pszText = tmp;
  lv_item.iItem = col++;
  lv_item.iSubItem = 1;
  ListView_SetItem(hList, &lv_item);

  sprintf(tmp, "%.8X", cpu_ssp);
  lv_item.pszText = tmp;
  lv_item.iItem = col++;
  lv_item.iSubItem = 1;
  ListView_SetItem(hList, &lv_item);

  sprintf(tmp, "%.8X", cpuGetVbr());
  lv_item.pszText = tmp;
  lv_item.iItem = col++;
  lv_item.iSubItem = 1;
  ListView_SetItem(hList, &lv_item);

  sprintf(tmp, "%.4X", cpuGetSR());
  lv_item.pszText = tmp;
  lv_item.iItem = col++;
  lv_item.iSubItem = 1;
  ListView_SetItem(hList, &lv_item);

}

void wdebUpdateCpuDisplay()
{
  wdebUpdateInstructionColumns();
  wdebUpdateRegisterColumns();
}

INT_PTR CALLBACK wdebDebuggerDialogProc(HWND hwndDlg,
				     UINT uMsg,
				     WPARAM wParam,
				     LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      wdeb_action = WDEB_INIT_DIALOG;
      return TRUE;
    case WM_PAINT:
      wdebUpdateCpuDisplay();
      break;
    case WM_DESTROY:
      break;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
      {
	switch (LOWORD(wParam))
	{
	  case IDC_BTN_STEP1:
	    winDrvDebugStart(DBG_STEP, hwndDlg);
	    wdebUpdateCpuDisplay();
	    break;
	  case IDC_BTN_STEP_OVER:
	    winDrvDebugStart(DBG_STEP_OVER, hwndDlg);
	    wdebUpdateCpuDisplay();
	    break;
	  case IDC_BTN_RUN:
	    winDrvDebugStart(DBG_RUN, hwndDlg);
	    wdebUpdateCpuDisplay();
	    break;
	  case IDC_BTN_BREAK:
	    fellowRequestEmulationStop();
	    break;
	  case IDOK:
	  case IDCANCEL:
	    wdeb_action = WDEB_EXIT;
	    break;
	  default:
	    break;
	}
      }
      break;
    default:
      break;
  }
  return FALSE;
}

void wdebCreateDialog()
{
  wdeb_hDialog = CreateDialog(win_drv_hInstance, MAKEINTRESOURCE(IDD_DEBUGGER), NULL, wdebDebuggerDialogProc); 
  ShowWindow(wdeb_hDialog, win_drv_nCmdShow);
}

void wdebDoMessages()
{
  BOOLE end_loop = FALSE;
  MSG myMsg;
  while (!end_loop)
  {
    if (GetMessage(&myMsg, wdeb_hDialog, 0, 0))
    {
      if (!IsDialogMessage(wdeb_hDialog, &myMsg))
      {
	TranslateMessage(&myMsg);
	DispatchMessage(&myMsg);
      }
    }
    switch (wdeb_action)
    {
      case WDEB_INIT_DIALOG:
	wdebInitInstructionColumns();
	wdebInitRegisterColumns();
	wdebInitInfoColumns();
	wdebUpdateCpuDisplay();
	break;
      case WDEB_EXIT:
	end_loop = TRUE;
	break;
      default:
	break;
    }
    wdeb_action = WDEB_NO_ACTION;
  }
}

void wdebCloseDialog()
{
  DestroyWindow(wdeb_hDialog);
  wdeb_hDialog = 0;
}

void wdebDebug()
{
  /* The configuration has been activated, but we must prepare the modules */
  /* for emulation ourselves */

  if (wguiCheckEmulationNecessities())
  {
    fellowEmulationStart();
    if (fellowGetPreStartReset())
      fellowHardReset();

    wdeb_action = WDEB_NO_ACTION;
    wdebCreateDialog();
    wdebDoMessages();
    wdebCloseDialog();

    fellowEmulationStop();
  }
  else
    MessageBox(NULL, "Specified KickImage does not exist", "Configuration Error", 0);


}

#endif /* WGUI */
