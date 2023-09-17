/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Windows GUI code for debugger                                           */
/*                                                                         */
/* Authors: Torsten Enderling                                              */
/*          Petter Schau                                                   */
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
#include "CpuModule.h"

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
#include "ListTree.h"
#include "gameport.h"
#include "config.h"
#include "draw.h"
#include "CpuModule_Disassembler.h"
#include "kbd.h"
#include "fmem.h"
#include "cia.h"
#include "floppy.h"
#include "blit.h"
#include "bus.h"
#include "CopperRegisters.h"
#include "graph.h"
#include "sound.h"
#include "sprite.h"
#include "SpriteRegisters.h"
#include "modrip.h"
#include "blit.h"
#include "wgui.h"

/*===============================*/
/* Handle of the main dialog box */
/*===============================*/

HWND wdbg_hDialog;

#ifdef FELLOW_USE_LEGACY_DEBUGGER
// the legacy debugger has more features, but is text-based and has been replaced
// by a more modern version; the code is retained for educational reasons

/*=======================================================*/
/* external references not exported by the include files */
/*=======================================================*/

/* floppy.c */
extern uint32_t dsklen, dsksync, dskpt;

/* sprite.c */
extern uint32_t sprpt[8];
extern uint32_t sprx[8];
extern uint32_t spry[8];
extern uint32_t sprly[8];
extern uint32_t spratt[8];
extern uint32_t sprite_state[8];

/* sound.c */
extern soundStateFunc audstate[4];
extern uint32_t audlenw[4];
extern uint32_t audper[4];
extern uint32_t audvol[4];
extern uint32_t audlen[4];
extern uint32_t audpercounter[4];

/* cia.c */

typedef struct cia_state_
{
  uint32_t ta;
  uint32_t tb;
  uint32_t ta_rem;
  uint32_t tb_rem;
  uint32_t talatch;
  uint32_t tblatch;
  int32_t taleft;
  int32_t tbleft;
  uint32_t evalarm;
  uint32_t evlatch;
  uint32_t evlatching;
  uint32_t evwritelatch;
  uint32_t evwritelatching;
  uint32_t evalarmlatch;
  uint32_t evalarmlatching;
  uint32_t ev;
  uint8_t icrreq;
  uint8_t icrmsk;
  uint8_t cra;
  uint8_t crb;
  uint8_t pra;
  uint8_t prb;
  uint8_t ddra;
  uint8_t ddrb;
  uint8_t sp;
} cia_state;

extern cia_state cia[2];

/* blit.c */

typedef struct blitter_state_
{
  uint32_t bltcon;
  uint32_t bltafwm;
  uint32_t bltalwm;
  uint32_t bltapt;
  uint32_t bltbpt;
  uint32_t bltcpt;
  uint32_t bltdpt;
  uint32_t bltamod;
  uint32_t bltbmod;
  uint32_t bltcmod;
  uint32_t bltdmod;
  uint32_t bltadat;
  uint32_t bltbdat;
  uint32_t bltbdat_original;
  uint32_t bltcdat;
  uint32_t bltzero;
  uint32_t height;
  uint32_t width;
  uint32_t a_shift_asc;
  uint32_t a_shift_desc;
  uint32_t b_shift_asc;
  uint32_t b_shift_desc;
  BOOLE started;
  BOOLE dma_pending;
  uint32_t cycle_length;
  uint32_t cycle_free;
} blitter_state;

extern blitter_state blitter;

/* text layout definitions */

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

static uint32_t memory_padd = WDBG_DISASSEMBLY_LINES * 32;
static uint32_t memory_adress = 0;
static BOOLE memory_ascii = FALSE;

static float g_DPIScaleX = 1, g_DPIScaleY = 1;

/*============*/
/* Prototypes */
/*============*/

INT_PTR CALLBACK wdbgCPUDialogProc(HWND hwndDlg, UINT uMsg,
				WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgMemoryDialogProc(HWND hwndDlg, UINT uMsg,
				   WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgCIADialogProc(HWND hwndDlg, UINT uMsg,
				WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgFloppyDialogProc(HWND hwndDlg, UINT uMsg,
				   WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgBlitterDialogProc(HWND hwndDlg, UINT uMsg,
				   WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgCopperDialogProc(HWND hwndDlg, UINT uMsg,
				   WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgSpriteDialogProc(HWND hwndDlg, UINT uMsg,
				   WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgScreenDialogProc(HWND hwndDlg, UINT uMsg,
				   WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgEventDialogProc(HWND hwndDlg, UINT uMsg,
				  WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgSoundDialogProc(HWND hwndDlg, UINT uMsg,
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

typedef INT_PTR (CALLBACK * wdbgDlgProc) (HWND, UINT, WPARAM, LPARAM);

wdbgDlgProc wdbg_propsheetDialogProc[DEBUG_PROP_SHEETS] = {
  wdbgCPUDialogProc,    wdbgMemoryDialogProc,  wdbgCIADialogProc,
  wdbgFloppyDialogProc, wdbgBlitterDialogProc, wdbgCopperDialogProc, 
  wdbgSpriteDialogProc, wdbgScreenDialogProc,  wdbgEventDialogProc, 
  wdbgSoundDialogProc
};

/*===============================*/
/* calculate DIP scaling factors */
/*===============================*/

void wdbgInitializeDPIScale(HWND hwnd)
{
  HDC hdc = GetDC(hwnd);
  g_DPIScaleX = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
  g_DPIScaleY = GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
  ReleaseDC(hwnd, hdc);
}

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

  wdbgInitializeDPIScale(wdbg_hDialog);

  return PropertySheet(&propertysheetheader);
}

/*============================================================================*/
/* helper functions                                                           */
/*============================================================================*/

char *wdbgGetDataRegistersStr(char * s)
{
  sprintf(s,
	  "D0: %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X :D7",
	  cpuGetDReg(0), cpuGetDReg(1), cpuGetDReg(2), cpuGetDReg(3), cpuGetDReg(4), cpuGetDReg(5), cpuGetDReg(6), cpuGetDReg(7));
  return s;
}

char *wdbgGetAddressRegistersStr(char * s)
{
  sprintf(s,
	  "A0: %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X :A7",
	  cpuGetAReg(0), cpuGetAReg(1), cpuGetAReg(2), cpuGetAReg(3), cpuGetAReg(4), cpuGetAReg(5), cpuGetAReg(6), cpuGetAReg(7));
  return s;
}

char *wdbgGetSpecialRegistersStr(char * s)
{
  sprintf(s,
	  "USP:%.8X SSP:%.8X SR:%.4X FRAME: %u y: %u x: %u",
	  cpuGetUspAutoMap(), cpuGetSspAutoMap(), cpuGetSR(), draw_frame_count, busGetRasterY(), busGetRasterX());
  return s;
}

void wdbgExtendStringTo80Columns(char * s)
{
  uint32_t i;
  BOOLE zero_found = FALSE;

  for (i = 0; i < 79; i++) {
    if (!zero_found)
      zero_found = (s[i] == '\0');
    if (zero_found)
      s[i] = ' ';
  }
  s[80] = '\0';
}

uint32_t wdbgLineOut(HDC hDC, char * s, uint32_t x, uint32_t y)
{
  struct tagSIZE text_size;
  SetBkMode(hDC, OPAQUE);
  wdbgExtendStringTo80Columns(s);
  TextOut(hDC, x, y, s, (int) strlen(s));
  GetTextExtentPoint32(hDC, s, (int) strlen(s), (LPSIZE) & text_size);
  return y + text_size.cy;
}

char *wdbgGetDisassemblyLineStr(char * s, uint32_t * disasm_pc)
{
  char inst_address[256]  = "";
  char inst_data[256]     = "";
  char inst_name[256]     = "";
  char inst_operands[256] = "";

  *disasm_pc = cpuDisOpcode(*disasm_pc, inst_address, inst_data, inst_name, inst_operands);
  sprintf(s, "%-9s %-18s %-11s %s", inst_address, inst_data, inst_name, inst_operands);
  return s;
}

/*============================================================================*/
/* Updates the CPU state                                                      */
/*============================================================================*/

void wdbgUpdateCPUState(HWND hwndDlg)
{
  char s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {
    uint32_t y = (uint32_t)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    uint32_t x = (uint32_t)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    uint32_t disasm_pc;
    uint32_t i;
    HFONT myfont = CreateFont((uint32_t)(8*g_DPIScaleY),
            (uint32_t)(8 * g_DPIScaleX),
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
    x = (uint32_t)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (uint32_t)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (uint32_t)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);
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
  char st[WDBG_STRLEN];
  unsigned char k;
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    uint32_t y = (uint32_t)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    uint32_t x = (uint32_t)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    uint32_t i, j;
    HFONT myfont = CreateFont((uint32_t)(8 * g_DPIScaleY),
      (uint32_t)(8 * g_DPIScaleX),
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
    x = (uint32_t)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (uint32_t)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (uint32_t)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    for (i = 0; i < WDBG_DISASSEMBLY_LINES; i++) {
      if (memory_ascii)
      {
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
      else 
      {
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
  char s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {
    uint32_t y = (uint32_t)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    uint32_t x = (uint32_t)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    uint32_t i;
    HFONT myfont = CreateFont((uint32_t)(8 * g_DPIScaleY),
      (uint32_t)(8 * g_DPIScaleX),
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
    x = (uint32_t)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (uint32_t)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (uint32_t)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    for (i = 0; i < 2; i++) {
      sprintf(s, "CIA %s Registers:", (i == 0) ? "A" : "B");
      y = wdbgLineOut(hDC, s, x, y);

      sprintf(s, "  CRA-%.2X CRB-%.2X IREQ-%.2X IMSK-%.2X SP-%.2X",
	      cia[i].cra, cia[i].crb, cia[i].icrreq, cia[i].icrmsk, cia[i].sp);
      y = wdbgLineOut(hDC, s, x, y);

      sprintf(s, "  EV-%.8X ALARM-%.8X", cia[i].ev, cia[i].evalarm);
      y = wdbgLineOut(hDC, s, x, y);

      sprintf(s, "  TA-%.4X TAHELP-%.8X TB-%.4X TBHELP-%.8X", cia[i].ta,
	      cia[i].taleft, cia[i].tb, cia[i].tbleft);
      y = wdbgLineOut(hDC, s, x, y);

      sprintf(s, "  TALATCH-%.4X TBLATCH-%.4X", cia[i].talatch, cia[i].tblatch);
      y = wdbgLineOut(hDC, s, x, y);

      strcpy(s, "");
      y = wdbgLineOut(hDC, s, x, y);

      if (cia[i].cra & 1)
        strcpy(s, "  Timer A started, ");
      else
        strcpy(s, "  Timer A stopped, ");

      if (cia[i].cra & 8)
        strcat(s, "One-shot mode");
      else
        strcat(s, "Continuous");

      y = wdbgLineOut(hDC, s, x, y);

      if (cia[i].crb & 1)
        strcpy(s, "  Timer B started, ");
      else
        strcpy(s, "  Timer B stopped, ");

      if (cia[i].crb & 8)
        strcat(s, "One-shot mode");
      else
        strcat(s, "Continuous");

      y = wdbgLineOut(hDC, s, x, y);

      strcpy(s, "");
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
  char s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    uint32_t y = (uint32_t)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    uint32_t x = (uint32_t)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    uint32_t i;
    HFONT myfont = CreateFont((uint32_t)(8 * g_DPIScaleY),
      (uint32_t)(8 * g_DPIScaleX),
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
    x = (uint32_t)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (uint32_t)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (uint32_t)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    for (i = 0; i < 4; i++)
    {
      sprintf(s, "DF%u: Track-%u Sel-%d Motor-%d Side-%d WP-%d",
        i,
	      floppy[i].track,
	      floppy[i].sel,
	      floppy[i].motor,
	      floppy[i].side,
	      floppy[i].writeprotconfig);
      y = wdbgLineOut(hDC, s, x, y);
    }

    strcpy(s, "");
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Dskpt-%.6X dsklen-%.4X dsksync-%.4X", dskpt, dsklen, dsksync);
    y = wdbgLineOut(hDC, s, x, y);

    if (floppy_DMA_started)
      strcpy(s, "Disk DMA running");
    else
      strcpy(s, "Disk DMA stopped");
    y = wdbgLineOut(hDC, s, x, y);

    strcpy(s, "Transfer settings:");
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Wordsleft: %u  Wait: %u  Dst: %X",
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
  char s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    uint32_t y = (uint32_t)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    uint32_t x = (uint32_t)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont((uint32_t)(8 * g_DPIScaleY),
      (uint32_t)(8 * g_DPIScaleX),
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
    x = (uint32_t)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (uint32_t)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (uint32_t)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    sprintf(s, "bltcon:  %08X bltafwm: %08X bltalwm: %08X", 
      blitter.bltcon, blitter.bltafwm, blitter.bltalwm);
    y = wdbgLineOut(hDC, s, x, y);

	sprintf(s, "");
    y = wdbgLineOut(hDC, s, x, y);

	sprintf(s, "bltapt:  %08X bltbpt:  %08X bltcpt:  %08X bltdpt:  %08X ",
    blitter.bltapt, blitter.bltbpt, blitter.bltcpt, blitter.bltdpt);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "bltamod: %08X bltbmod: %08X bltcmod: %08X bltdmod: %08X ",
      blitter.bltamod, blitter.bltbmod, blitter.bltcmod, blitter.bltdmod);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "bltadat: %08X bltbdat: %08X bltcdat: %08X bltzero: %08X ",
      blitter.bltadat, blitter.bltbdat, blitter.bltcdat, blitter.bltzero);
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
  char s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    uint32_t y = (uint32_t)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    uint32_t x = (uint32_t)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    uint32_t i, list1, list2, atpc;
    HFONT myfont = CreateFont((uint32_t)(8 * g_DPIScaleY),
      (uint32_t)(8 * g_DPIScaleX),
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
    x = (uint32_t)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (uint32_t)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (uint32_t)(WDBG_DISASSEMBLY_INDENT* g_DPIScaleX);

    list1 = (copper_registers.cop1lc & 0xfffffe);
    list2 = (copper_registers.cop2lc & 0xfffffe);

    atpc = (copper_registers.copper_pc & 0xfffffe);	/* Make sure debug doesn't trap odd ex */

    sprintf(s, "Cop1lc-%.6X Cop2lc-%.6X Copcon-%u Copper PC - %.6X", copper_registers.cop1lc,
      copper_registers.cop2lc, copper_registers.copcon, copper_registers.copper_pc);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next cycle - %u  Y - %u  X - %u", copperEvent.cycle,
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
  char s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    uint32_t y = (uint32_t)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    uint32_t x = (uint32_t)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont((uint32_t)(8 * g_DPIScaleY),
      (uint32_t)(8 * g_DPIScaleX),
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
    x = (uint32_t)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (uint32_t)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (uint32_t)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    sprintf(s, "Spr0pt-%.6X Spr1pt-%.6X Spr2pt-%.6X Spr3pt - %.6X", sprite_registers.sprpt[0],
      sprite_registers.sprpt[1], sprite_registers.sprpt[2], sprite_registers.sprpt[3]);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Spr4pt-%.6X Spr5pt-%.6X Spr6pt-%.6X Spr7pt - %.6X", sprite_registers.sprpt[4],
      sprite_registers.sprpt[5], sprite_registers.sprpt[6], sprite_registers.sprpt[7]);
    y = wdbgLineOut(hDC, s, x, y);

    //for (uint32_t i = 0; i < 8; i++) {
    //  sprintf(s,
	   //   "Spr%uX-%u Spr%uStartY-%u Spr%uStopY-%u Spr%uAttached-%u Spr%ustate-%u",
	   //   i, sprx[i], i, spry[i], i, sprly[i], i, spratt[i], i,
	   //   sprite_state[i]);
    //  y = wdbgLineOut(hDC, s, x, y);
    //}

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
  char s[128];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    uint32_t y = (uint32_t)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    uint32_t x = (uint32_t)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont((uint32_t)(8 * g_DPIScaleY),
      (uint32_t)(8 * g_DPIScaleX),
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
    x = (uint32_t)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (uint32_t)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (uint32_t)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    sprintf(s, "DIWSTRT - %.4X DIWSTOP - %.4X DDFSTRT - %.4X DDFSTOP - %.4X LOF     - %.4X",
	    diwstrt, diwstop, ddfstrt, ddfstop, lof);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "BPLCON0 - %.4X BPLCON1 - %.4X BPLCON2 - %.4X BPL1MOD - %.4X BPL2MOD - %.4X",
	    _core.Registers.BplCon0, bplcon1, _core.Registers.BplCon2, bpl1mod, bpl2mod);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "BPL1PT -%.6X BPL2PT -%.6X BPL3PT -%.6X BPL4PT -%.6X BPL5PT -%.6X",
            bpl1pt, bpl2pt, bpl3pt, bpl4pt, bpl5pt);
    y = wdbgLineOut(hDC, s, x, y);
      
    sprintf(s, "BPL6PT -%.6X DMACON  - %.4X", bpl6pt, dmacon);
    y = wdbgLineOut(hDC, s, x, y);
    y++;

    const draw_rect& clip = drawGetOutputClip();
    sprintf(s, "Host window clip envelope (Hor) (Ver): (%u, %u) (%u, %u)",
            clip.left, clip.right, clip.top, clip.bottom);
    y = wdbgLineOut(hDC, s, x, y);
    
    sprintf(s, "Even/odd scroll (lores/hires): (%u, %u) (%u, %u)",
            evenscroll, evenhiscroll, oddscroll, oddhiscroll);
    y = wdbgLineOut(hDC, s, x, y);
    
    sprintf(s, "Visible bitplane data envelope (Hor) (Ver): (%u, %u) (%u, %u)",
	    diwxleft, diwxright, diwytop, diwybottom);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "DDF (First output cylinder, length in words): (%u, %u)",
	    graph_DDF_start, graph_DDF_word_count);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "DIW (First visible pixel, last visible pixel + 1): (%u, %u)",
	    graph_DIW_first_visible, graph_DIW_last_visible);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Raster beam position (x, y): (%u, %u)",
	    busGetRasterX(), busGetRasterY());
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
  char s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    uint32_t y = (uint32_t)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    uint32_t x = (uint32_t)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont((uint32_t)(8 * g_DPIScaleY),
      (uint32_t)(8 * g_DPIScaleX),
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
    x = (uint32_t)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (uint32_t)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (uint32_t)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    sprintf(s, "Next CPU      - %u", cpuEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next Copper   - %u", copperEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next EOL      - %u", eolEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next Blitter  - %u", blitterEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next EOF      - %u", eofEvent.cycle);
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
  char s[WDBG_STRLEN];
  HDC hDC;
  PAINTSTRUCT paint_struct;

  hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != NULL) {

    uint32_t y = (uint32_t)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    uint32_t x = (uint32_t)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    uint32_t i;
    HFONT myfont = CreateFont((uint32_t)(8 * g_DPIScaleY),
      (uint32_t)(8 * g_DPIScaleX),
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
    x = (uint32_t)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (uint32_t)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (uint32_t)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    sprintf(s, "A0: %X A1: %X A2: %X A3: %X A5: %X", soundState0, soundState1,
	    soundState2, soundState3, soundState5);
    y = wdbgLineOut(hDC, s, x, y);

    for (i = 0; i < 4; i++) {
      sprintf(s,
	      "Ch%u State: %2d Lenw: %5u Len: %5u per: %5u Pcnt: %5X Vol: %5u",
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

INT_PTR CALLBACK wdbgCPUDialogProc(HWND hwndDlg,
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

INT_PTR CALLBACK wdbgMemoryDialogProc(HWND hwndDlg,
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

INT_PTR CALLBACK wdbgCIADialogProc(HWND hwndDlg,
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

INT_PTR CALLBACK wdbgFloppyDialogProc(HWND hwndDlg,
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

INT_PTR CALLBACK wdbgBlitterDialogProc(HWND hwndDlg,
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

INT_PTR CALLBACK wdbgCopperDialogProc(HWND hwndDlg,
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

INT_PTR CALLBACK wdbgSpriteDialogProc(HWND hwndDlg,
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

INT_PTR CALLBACK wdbgScreenDialogProc(HWND hwndDlg,
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

INT_PTR CALLBACK wdbgEventDialogProc(HWND hwndDlg,
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

INT_PTR CALLBACK wdbgSoundDialogProc(HWND hwndDlg,
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

#else

HWND wdeb_hDialog = 0;
uint32_t WDEB_DISASM_LINES = 42;

enum wdeb_actions
{
  WDEB_NO_ACTION,
  WDEB_INIT_DIALOG,
  WDEB_EXIT
};

enum wdeb_actions wdeb_action;

void wdebInitInstructionColumns()
{
  uint32_t i;
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

char *wdbg_registernames[] = 
{"D0","D1","D2","D3","D4","D5","D6","D7",
"A0","A1","A2","A3","A4","A5","A6","A7",
"PC","USP","SSP","MSP","ISP","VBR","SR"};

void wdebInitRegisterColumns()
{
  uint32_t i;
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
  uint32_t i;
  LV_ITEM lv_item;
  HWND hList=GetDlgItem(wdeb_hDialog, IDC_LST_INSTRUCTIONS);
  uint32_t disasm_pc = cpuGetPC();
  char inst_address[256];
  char inst_data[256];
  char inst_name[256];
  char inst_operands[256];

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
  uint32_t i, j, col;
  LV_ITEM lv_item;
  HWND hList=GetDlgItem(wdeb_hDialog, IDC_LST_REGISTERS);
  uint32_t disasm_pc = cpuGetPC();
  char tmp[16];

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

  sprintf(tmp, "%.8X", cpuGetUspDirect());
  lv_item.pszText = tmp;
  lv_item.iItem = col++;
  lv_item.iSubItem = 1;
  ListView_SetItem(hList, &lv_item);

  sprintf(tmp, "%.8X", cpuGetSspDirect());
  lv_item.pszText = tmp;
  lv_item.iItem = col++;
  lv_item.iSubItem = 1;
  ListView_SetItem(hList, &lv_item);

  sprintf(tmp, "%.8X", cpuGetMspDirect());
  lv_item.pszText = tmp;
  lv_item.iItem = col++;
  lv_item.iSubItem = 1;
  ListView_SetItem(hList, &lv_item);

  sprintf(tmp, "%.8X", cpuGetSspDirect());
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

BOOLE wdeb_is_working = FALSE;

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
	    if (!wdeb_is_working)
	    {
	      wdeb_is_working = TRUE;
	      winDrvDebugStart(DBG_STEP, hwndDlg);
	      wdebUpdateCpuDisplay();
	      wdeb_is_working = FALSE;
	    }
	    break;
	  case IDC_BTN_STEP_OVER:
	    if (!wdeb_is_working)
	    {
	      wdeb_is_working = TRUE;
	      winDrvDebugStart(DBG_STEP_OVER, hwndDlg);
	      wdebUpdateCpuDisplay();
	      wdeb_is_working = FALSE;
	    }
	    break;
	  case IDC_BTN_RUN:
	    if (!wdeb_is_working)
	    {
	      wdeb_is_working = TRUE;
	      winDrvDebugStart(DBG_RUN, hwndDlg);
	      wdebUpdateCpuDisplay();
	      wdeb_is_working = FALSE;
	    }
	    break;
	  case IDC_BTN_MODRIP:
	    modripRIP();
	    break;
	  case IDC_BTN_DUMPCHIP:
	    modripChipDump();
            break;
	  case IDC_BTN_BREAK:
	    if (wdeb_is_working)
	    {
	      fellowRequestEmulationStop();
	    }
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
    {
      fellowHardReset();
    }
    wdeb_action = WDEB_NO_ACTION;
    wdebCreateDialog();
    wdebDoMessages();
    wdebCloseDialog();

    fellowEmulationStop();
  }
  else
  {
    MessageBox(NULL, "Specified KickImage does not exist", "Configuration Error", 0);
  }
}

#endif