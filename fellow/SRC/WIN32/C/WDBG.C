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

#include "fellow/api/defs.h"
#include "fellow/cpu/CpuModule.h"

#include <WinDef.h>
#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <PrSht.h>

#include "gui_general.h"
#include "gui_debugger.h"

#include "fellow/application/Fellow.h"
#include "WINDRV.H"
#include "fellow/chipset/Sound.h"
#include "fellow/application/ListTree.h"
#include "fellow/application/GamePort.h"
#include "fellow/configuration/Configuration.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/cpu/CpuModule_Disassembler.h"
#include "fellow/chipset/Kbd.h"
#include "fellow/memory/Memory.h"
#include "fellow/chipset/Cia.h"
#include "fellow/chipset/Floppy.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/chipset/CopperRegisters.h"
#include "fellow/chipset/Graphics.h"
#include "fellow/chipset/Sound.h"
#include "fellow/chipset/Sprite.h"
#include "fellow/chipset/SpriteRegisters.h"
#include "fellow/application/modrip.h"
#include "fellow/chipset/Blitter.h"
#include "fellow/application/WGui.h"
#include "commoncontrol_wrap.h"
#include "GfxDrvCommon.h"
#include "fellow/chipset/draw_interlace_control.h"
#include "fellow/debug/log/DebugLogHandler.h"

/*===============================*/
/* Handle of the main dialog box */
/*===============================*/

HWND wdbg_hDialog;

//#define FELLOW_USE_LEGACY_DEBUGGER
#ifdef FELLOW_USE_LEGACY_DEBUGGER
// the legacy debugger has more features, but is text-based and has been replaced
// by a more modern version; the code is retained for educational reasons

/*=======================================================*/
/* external references not exported by the include files */
/*=======================================================*/

/* floppy.c */
extern ULO dsklen, dsksync, dskpt;

/* sprite.c */
extern ULO sprpt[8];
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

/* cia.c */

typedef struct cia_state_
{
  ULO ta;
  ULO tb;
  ULO ta_rem;
  ULO tb_rem;
  ULO talatch;
  ULO tblatch;
  LON taleft;
  LON tbleft;
  ULO evalarm;
  ULO evlatch;
  ULO evlatching;
  ULO evwritelatch;
  ULO evwritelatching;
  ULO evalarmlatch;
  ULO evalarmlatching;
  ULO ev;
  UBY icrreq;
  UBY icrmsk;
  UBY cra;
  UBY crb;
  UBY pra;
  UBY prb;
  UBY ddra;
  UBY ddrb;
  UBY sp;
} cia_state;

extern cia_state cia[2];

/* blit.c */

typedef struct blitter_state_
{
  ULO bltcon;
  ULO bltafwm;
  ULO bltalwm;
  ULO bltapt;
  ULO bltbpt;
  ULO bltcpt;
  ULO bltdpt;
  ULO bltamod;
  ULO bltbmod;
  ULO bltcmod;
  ULO bltdmod;
  ULO bltadat;
  ULO bltbdat;
  ULO bltbdat_original;
  ULO bltcdat;
  ULO bltzero;
  ULO height;
  ULO width;
  ULO a_shift_asc;
  ULO a_shift_desc;
  ULO b_shift_asc;
  ULO b_shift_desc;
  BOOLE started;
  BOOLE dma_pending;
  ULO cycle_length;
  ULO cycle_free;
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

static ULO memory_padd = WDBG_DISASSEMBLY_LINES * 32;
static ULO memory_adress = 0;
static BOOLE memory_ascii = FALSE;

static float g_DPIScaleX = 1, g_DPIScaleY = 1;

/*============*/
/* Prototypes */
/*============*/

INT_PTR CALLBACK wdbgCPUDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgMemoryDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgCIADialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgFloppyDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgBlitterDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgCopperDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgSpriteDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgScreenDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgEventDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK wdbgSoundDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*==============================================================*/
/* the various sheets of the main dialog and their dialog procs */
/*==============================================================*/

#define DEBUG_PROP_SHEETS 10

UINT wdbg_propsheetRID[DEBUG_PROP_SHEETS] = {
    IDD_DEBUG_CPU, IDD_DEBUG_MEMORY, IDD_DEBUG_CIA, IDD_DEBUG_FLOPPY, IDD_DEBUG_BLITTER, IDD_DEBUG_COPPER, IDD_DEBUG_SPRITES, IDD_DEBUG_SCREEN, IDD_DEBUG_EVENTS, IDD_DEBUG_SOUND};

typedef INT_PTR(CALLBACK *wdbgDlgProc)(HWND, UINT, WPARAM, LPARAM);

wdbgDlgProc wdbg_propsheetDialogProc[DEBUG_PROP_SHEETS] = {
    wdbgCPUDialogProc,
    wdbgMemoryDialogProc,
    wdbgCIADialogProc,
    wdbgFloppyDialogProc,
    wdbgBlitterDialogProc,
    wdbgCopperDialogProc,
    wdbgSpriteDialogProc,
    wdbgScreenDialogProc,
    wdbgEventDialogProc,
    wdbgSoundDialogProc};

/*===============================*/
/* calculate DIP scaling factors */
/*===============================*/

void wdbgInitializeDPIScale(HWND hwnd)
{
  HDC hdc = GetDC(hwnd);
  g_DPIScaleX = (float)GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
  g_DPIScaleY = (float)GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
  ReleaseDC(hwnd, hdc);
}

/*===========================*/
/* debugger main dialog proc */
/*===========================*/

INT_PTR wdbgSessionMainDialog()
{
  PROPSHEETPAGE propertysheets[DEBUG_PROP_SHEETS];
  PROPSHEETHEADER propertysheetheader;

  for (int i = 0; i < DEBUG_PROP_SHEETS; i++)
  {
    propertysheets[i].dwSize = sizeof(PROPSHEETPAGE);
    propertysheets[i].dwFlags = PSP_DEFAULT;
    propertysheets[i].hInstance = win_drv_hInstance;
    propertysheets[i].pszTemplate = MAKEINTRESOURCE(wdbg_propsheetRID[i]);
    propertysheets[i].hIcon = nullptr;
    propertysheets[i].pszTitle = nullptr;
    propertysheets[i].pfnDlgProc = wdbg_propsheetDialogProc[i];
    propertysheets[i].lParam = 0;
    propertysheets[i].pfnCallback = nullptr;
    propertysheets[i].pcRefParent = nullptr;
  }
  propertysheetheader.dwSize = sizeof(PROPSHEETHEADER);
  propertysheetheader.dwFlags = PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_NOAPPLYNOW;
  propertysheetheader.hwndParent = wdbg_hDialog;
  propertysheetheader.hInstance = win_drv_hInstance;
  propertysheetheader.hIcon = LoadIcon(win_drv_hInstance, MAKEINTRESOURCE(IDI_ICON_WINFELLOW));
  propertysheetheader.pszCaption = "WinFellow Debugger Session";
  propertysheetheader.nPages = DEBUG_PROP_SHEETS;
  propertysheetheader.nStartPage = 0;
  propertysheetheader.ppsp = propertysheets;
  propertysheetheader.pfnCallback = nullptr;

  wdbgInitializeDPIScale(wdbg_hDialog);

  return PropertySheet(&propertysheetheader);
}

/*============================================================================*/
/* helper functions                                                           */
/*============================================================================*/

STR *wdbgGetDataRegistersStr(STR *s)
{
  sprintf(s, "D0: %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X :D7", cpuGetDReg(0), cpuGetDReg(1), cpuGetDReg(2), cpuGetDReg(3), cpuGetDReg(4), cpuGetDReg(5), cpuGetDReg(6), cpuGetDReg(7));
  return s;
}

STR *wdbgGetAddressRegistersStr(STR *s)
{
  sprintf(s, "A0: %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X :A7", cpuGetAReg(0), cpuGetAReg(1), cpuGetAReg(2), cpuGetAReg(3), cpuGetAReg(4), cpuGetAReg(5), cpuGetAReg(6), cpuGetAReg(7));
  return s;
}

STR *wdbgGetSpecialRegistersStr(STR *s)
{
  sprintf(s, "USP:%.8X SSP:%.8X SR:%.4X FRAME: %u y: %u x: %u", cpuGetUspAutoMap(), cpuGetSspAutoMap(), cpuGetSR(), draw_frame_count, busGetRasterY(), busGetRasterX());
  return s;
}

void wdbgExtendStringTo80Columns(STR *s)
{
  BOOLE zero_found = FALSE;

  for (ULO i = 0; i < 79; i++)
  {
    if (!zero_found) zero_found = (s[i] == '\0');
    if (zero_found) s[i] = ' ';
  }
  s[80] = '\0';
}

ULO wdbgLineOut(HDC hDC, STR *s, ULO x, ULO y)
{
  struct tagSIZE text_size
  {
  };
  SetBkMode(hDC, OPAQUE);
  wdbgExtendStringTo80Columns(s);
  TextOut(hDC, x, y, s, (int)strlen(s));
  GetTextExtentPoint32(hDC, s, (int)strlen(s), (LPSIZE)&text_size);
  return y + text_size.cy;
}

STR *wdbgGetDisassemblyLineStr(STR *s, ULO *disasm_pc)
{
  STR inst_address[256] = "";
  STR inst_data[256] = "";
  STR inst_name[256] = "";
  STR inst_operands[256] = "";

  *disasm_pc = cpuDisOpcode(*disasm_pc, inst_address, inst_data, inst_name, inst_operands);
  sprintf(s, "%-9s %-18s %-11s %s", inst_address, inst_data, inst_name, inst_operands);
  return s;
}

/*============================================================================*/
/* Updates the CPU state                                                      */
/*============================================================================*/

void wdbgUpdateCPUState(HWND hwndDlg)
{
  STR s[WDBG_STRLEN];
  PAINTSTRUCT paint_struct;

  HDC hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != nullptr)
  {
    ULO y = (ULO)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    ULO x = (ULO)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    ULO disasm_pc;
    HFONT myfont = CreateFont(
        (ULO)(8 * g_DPIScaleY),
        (ULO)(8 * g_DPIScaleX),
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
    HBITMAP myarrow = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(s), x, y);
    wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(s), x, y);
    x = (ULO)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (ULO)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (ULO)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);
    disasm_pc = cpuGetPC();
    for (ULO i = 0; i < WDBG_DISASSEMBLY_LINES; i++)
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
  PAINTSTRUCT paint_struct;

  HDC hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != nullptr)
  {

    ULO y = (ULO)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    ULO x = (ULO)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont(
        (ULO)(8 * g_DPIScaleY),
        (ULO)(8 * g_DPIScaleX),
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

    HBITMAP myarrow = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(st), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(st), x, y);
    wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(st), x, y);
    x = (ULO)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (ULO)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (ULO)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    for (ULO i = 0; i < WDBG_DISASSEMBLY_LINES; i++)
    {
      if (memory_ascii)
      {
        sprintf(
            st,
            "%.6X %.8X%.8X %.8X%.8X ",
            (memory_adress + i * 16) & 0xffffff,
            memoryReadLong(memory_adress + i * 16 + 0),
            memoryReadLong(memory_adress + i * 16 + 4),
            memoryReadLong(memory_adress + i * 16 + 8),
            memoryReadLong(memory_adress + i * 16 + 12));

        for (ULO j = 0; j < 16; j++)
        {
          unsigned char k = memoryReadByte(memory_adress + i * 16 + j) & 0xff;
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
        sprintf(
            st,
            "%.6X %.8X%.8X %.8X%.8X %.8X%.8X %.8X%.8X",
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
  PAINTSTRUCT paint_struct;

  HDC hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != nullptr)
  {
    ULO y = (ULO)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    ULO x = (ULO)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont(
        (ULO)(8 * g_DPIScaleY),
        (ULO)(8 * g_DPIScaleX),
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

    HBITMAP myarrow = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(s), x, y);
    x = (ULO)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (ULO)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (ULO)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    for (ULO i = 0; i < 2; i++)
    {
      sprintf(s, "CIA %s Registers:", (i == 0) ? "A" : "B");
      y = wdbgLineOut(hDC, s, x, y);

      sprintf(s, "  CRA-%.2X CRB-%.2X IREQ-%.2X IMSK-%.2X SP-%.2X", cia[i].cra, cia[i].crb, cia[i].icrreq, cia[i].icrmsk, cia[i].sp);
      y = wdbgLineOut(hDC, s, x, y);

      sprintf(s, "  EV-%.8X ALARM-%.8X", cia[i].ev, cia[i].evalarm);
      y = wdbgLineOut(hDC, s, x, y);

      sprintf(s, "  TA-%.4X TAHELP-%.8X TB-%.4X TBHELP-%.8X", cia[i].ta, cia[i].taleft, cia[i].tb, cia[i].tbleft);
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
  STR s[WDBG_STRLEN];
  PAINTSTRUCT paint_struct;

  HDC hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != nullptr)
  {

    ULO y = (ULO)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    ULO x = (ULO)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont(
        (ULO)(8 * g_DPIScaleY),
        (ULO)(8 * g_DPIScaleX),
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

    HBITMAP myarrow = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(s), x, y);
    wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(s), x, y);
    x = (ULO)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (ULO)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (ULO)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    for (ULO i = 0; i < 4; i++)
    {
      sprintf(s, "DF%u: Track-%u Sel-%d Motor-%d Side-%d WP-%d", i, floppy[i].track, floppy[i].sel, floppy[i].motor, floppy[i].side, floppy[i].writeprotconfig);
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

    sprintf(s, "Wordsleft: %u  Wait: %u  Dst: %X", floppy_DMA.wordsleft, floppy_DMA.wait, floppy_DMA.dskpt);
    y = wdbgLineOut(hDC, s, x, y);

#ifdef ALIGN_CHECK
    sprintf(s, "Busloop: %X  eol: %X copemu: %X", checkadr, end_of_line, copemu);
    y = wdbgLineOut(hDC, s, x, y);
    sprintf(s, "68000strt:%X busstrt:%X copperstrt:%X memorystrt:%X", m68000start, busstart, copperstart, memorystart);
    y = wdbgLineOut(hDC, s, x, y);
    sprintf(s, "sndstrt:%X sbstrt:%X blitstrt:%X", soundstart, sblaststart, blitstart);
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
  PAINTSTRUCT paint_struct;

  HDC hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != nullptr)
  {

    ULO y = (ULO)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    ULO x = (ULO)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont(
        (ULO)(8 * g_DPIScaleY),
        (ULO)(8 * g_DPIScaleX),
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

    HBITMAP myarrow = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(s), x, y);
    wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(s), x, y);
    x = (ULO)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (ULO)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (ULO)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    sprintf(s, "bltcon:  %08X bltafwm: %08X bltalwm: %08X", blitter.bltcon, blitter.bltafwm, blitter.bltalwm);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "");
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "bltapt:  %08X bltbpt:  %08X bltcpt:  %08X bltdpt:  %08X ", blitter.bltapt, blitter.bltbpt, blitter.bltcpt, blitter.bltdpt);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "bltamod: %08X bltbmod: %08X bltcmod: %08X bltdmod: %08X ", blitter.bltamod, blitter.bltbmod, blitter.bltcmod, blitter.bltdmod);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "bltadat: %08X bltbdat: %08X bltcdat: %08X bltzero: %08X ", blitter.bltadat, blitter.bltbdat, blitter.bltcdat, blitter.bltzero);
    wdbgLineOut(hDC, s, x, y);

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
  PAINTSTRUCT paint_struct;

  HDC hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != nullptr)
  {

    ULO y = (ULO)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    ULO x = (ULO)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont(
        (ULO)(8 * g_DPIScaleY),
        (ULO)(8 * g_DPIScaleX),
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

    HBITMAP myarrow = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(s), x, y);
    wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(s), x, y);
    x = (ULO)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (ULO)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (ULO)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    ULO list1 = (copper_registers.cop1lc & 0xfffffe);
    ULO list2 = (copper_registers.cop2lc & 0xfffffe);

    ULO atpc = (copper_registers.copper_pc & 0xfffffe); /* Make sure debug doesn't trap odd ex */

    sprintf(s, "Cop1lc-%.6X Cop2lc-%.6X Copcon-%u Copper PC - %.6X", copper_registers.cop1lc, copper_registers.cop2lc, copper_registers.copcon, copper_registers.copper_pc);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next cycle - %u  Y - %u  X - %u", copperEvent.cycle, (copperEvent.cycle != -1) ? (copperEvent.cycle / 228) : 0, (copperEvent.cycle != -1) ? (copperEvent.cycle % 228) : 0);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "List 1:        List 2:        At PC:");
    y = wdbgLineOut(hDC, s, x, y);

    for (ULO i = 0; i < 16; i++)
    {
      sprintf(
          s,
          "$%.4X, $%.4X   $%.4X, $%.4X   $%.4X, $%.4X",
          memoryReadWord(list1),
          memoryReadWord(list1 + 2),
          memoryReadWord(list2),
          memoryReadWord(list2 + 2),
          memoryReadWord(atpc),
          memoryReadWord(atpc + 2));
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
  PAINTSTRUCT paint_struct;

  HDC hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != nullptr)
  {

    ULO y = (ULO)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    ULO x = (ULO)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont(
        (ULO)(8 * g_DPIScaleY),
        (ULO)(8 * g_DPIScaleX),
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

    HBITMAP myarrow = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(s), x, y);
    x = (ULO)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (ULO)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (ULO)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    sprintf(s, "Spr0pt-%.6X Spr1pt-%.6X Spr2pt-%.6X Spr3pt - %.6X", sprite_registers.sprpt[0], sprite_registers.sprpt[1], sprite_registers.sprpt[2], sprite_registers.sprpt[3]);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Spr4pt-%.6X Spr5pt-%.6X Spr6pt-%.6X Spr7pt - %.6X", sprite_registers.sprpt[4], sprite_registers.sprpt[5], sprite_registers.sprpt[6], sprite_registers.sprpt[7]);
    wdbgLineOut(hDC, s, x, y);

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
  PAINTSTRUCT paint_struct;

  HDC hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != nullptr)
  {

    ULO y = (ULO)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    ULO x = (ULO)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont(
        (ULO)(8 * g_DPIScaleY),
        (ULO)(8 * g_DPIScaleX),
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

    HBITMAP myarrow = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(s), x, y);
    wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(s), x, y);
    x = (ULO)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (ULO)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (ULO)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    sprintf(
        s,
        "DIWSTRT - %.4X DIWSTOP - %.4X DDFSTRT - %.4X DDFSTOP - %.4X LOF     - %.4X",
        bitplane_registers.diwstrt,
        bitplane_registers.diwstop,
        bitplane_registers.ddfstrt,
        bitplane_registers.ddfstop,
        bitplane_registers.lof);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(
        s,
        "BPLCON0 - %.4X BPLCON1 - %.4X BPLCON2 - %.4X BPL1MOD - %.4X BPL2MOD - %.4X",
        bitplane_registers.bplcon0,
        bitplane_registers.bplcon1,
        bitplane_registers.bplcon2,
        bitplane_registers.bpl1mod,
        bitplane_registers.bpl2mod);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(
        s,
        "BPL1PT -%.6X BPL2PT -%.6X BPL3PT -%.6X BPL4PT -%.6X BPL5PT -%.6X",
        bitplane_registers.bplpt[0],
        bitplane_registers.bplpt[1],
        bitplane_registers.bplpt[2],
        bitplane_registers.bplpt[3],
        bitplane_registers.bplpt[4]);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "BPL6PT -%.6X DMACON  - %.4X", bitplane_registers.bplpt[5], dmacon);
    y = wdbgLineOut(hDC, s, x, y);
    y++;

    const draw_rect &clip = drawGetChipsetClipOutput();
    sprintf(s, "Host window clip envelope (Hor) (Ver): (%u, %u) (%u, %u)", clip.left, clip.right, clip.top, clip.bottom);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Even/odd scroll (lores/hires): (%u, %u) (%u, %u)", evenscroll, evenhiscroll, oddscroll, oddhiscroll);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Visible bitplane data envelope (Hor) (Ver): (%u, %u) (%u, %u)", diwxleft, diwxright, diwytop, diwybottom);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "DDF (First output cylinder, length in words): (%u, %u)", graph_DDF_start, graph_DDF_word_count);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "DIW (First visible pixel, last visible pixel + 1): (%u, %u)", graph_DIW_first_visible, graph_DIW_last_visible);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Raster beam position (x, y): (%u, %u)", busGetRasterX(), busGetRasterY());
    wdbgLineOut(hDC, s, x, y);

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
  PAINTSTRUCT paint_struct;

  HDC hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != nullptr)
  {

    ULO y = (ULO)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    ULO x = (ULO)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont(
        (ULO)(8 * g_DPIScaleY),
        (ULO)(8 * g_DPIScaleX),
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

    HBITMAP myarrow = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(s), x, y);
    wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(s), x, y);
    x = (ULO)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (ULO)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (ULO)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    sprintf(s, "Next CPU      - %u", cpuEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next Copper   - %u", copperEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next EOL      - %u", eolEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next Blitter  - %u", blitterEvent.cycle);
    y = wdbgLineOut(hDC, s, x, y);

    sprintf(s, "Next EOF      - %u", eofEvent.cycle);
    wdbgLineOut(hDC, s, x, y);

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
  PAINTSTRUCT paint_struct;

  HDC hDC = BeginPaint(hwndDlg, &paint_struct);
  if (hDC != nullptr)
  {

    ULO y = (ULO)(WDBG_CPU_REGISTERS_Y * g_DPIScaleY);
    ULO x = (ULO)(WDBG_CPU_REGISTERS_X * g_DPIScaleX);
    HFONT myfont = CreateFont(
        (ULO)(8 * g_DPIScaleY),
        (ULO)(8 * g_DPIScaleX),
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

    HBITMAP myarrow = LoadBitmap(win_drv_hInstance, MAKEINTRESOURCE(IDB_DEBUG_ARROW));
    HDC hDC_image = CreateCompatibleDC(hDC);
    SelectObject(hDC_image, myarrow);
    SelectObject(hDC, myfont);
    SetBkMode(hDC, TRANSPARENT);
    SetBkMode(hDC_image, TRANSPARENT);
    y = wdbgLineOut(hDC, wdbgGetDataRegistersStr(s), x, y);
    y = wdbgLineOut(hDC, wdbgGetAddressRegistersStr(s), x, y);
    wdbgLineOut(hDC, wdbgGetSpecialRegistersStr(s), x, y);
    x = (ULO)(WDBG_DISASSEMBLY_X * g_DPIScaleX);
    y = (ULO)(WDBG_DISASSEMBLY_Y * g_DPIScaleY);
    BitBlt(hDC, x, y + 2, 14, 14, hDC_image, 0, 0, SRCCOPY);
    x += (ULO)(WDBG_DISASSEMBLY_INDENT * g_DPIScaleX);

    sprintf(s, "A0: %p A1: %p A2: %p A3: %p A5: %p", soundState0, soundState1, soundState2, soundState3, soundState5);
    y = wdbgLineOut(hDC, s, x, y);

    for (ULO i = 0; i < 4; i++)
    {
      sprintf(
          s,
          "Ch%u State: %2d Lenw: %5u Len: %5u per: %5u Pcnt: %5X Vol: %5u",
          i,
          (audstate[i] == soundState0)   ? 0
          : (audstate[i] == soundState1) ? 1
          : (audstate[i] == soundState2) ? 2
          : (audstate[i] == soundState3) ? 3
          : (audstate[i] == soundState5) ? 5
                                         : -1,
          audlenw[i],
          audlen[i],
          audper[i],
          audpercounter[i],
          audvol[i]);
      y = wdbgLineOut(hDC, s, x, y);
    }

    sprintf(s, "dmacon: %X", dmacon);
    wdbgLineOut(hDC, s, x, y);

    DeleteDC(hDC_image);
    DeleteObject(myarrow);
    DeleteObject(myfont);
    EndPaint(hwndDlg, &paint_struct);
  }
}

/*============================================================================*/
/* DialogProc for our CPU dialog                                              */
/*============================================================================*/

INT_PTR CALLBACK wdbgCPUDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG: return TRUE;
    case WM_PAINT: wdbgUpdateCPUState(hwndDlg); break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_DEBUG_CPU_STEP: winDrvDebugStart(DBG_STEP, 0); break;
        case IDC_DEBUG_CPU_STEP_OVER: winDrvDebugStart(DBG_STEP_OVER, 0); break;
        case IDC_DEBUG_CPU_RUN: winDrvDebugStart(DBG_RUN, 0); break;
        case IDC_DEBUG_CPU_BREAK:
          fellowRequestEmulationStop();
          InvalidateRect(hwndDlg, nullptr, FALSE);
          break;
        default: break;
      }
      break;
    default: break;
  }
  return FALSE;
}

/*============================================================================*/
/* DialogProc for our memory dialog                                           */
/*============================================================================*/

INT_PTR CALLBACK wdbgMemoryDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG: return TRUE;
    case WM_PAINT: wdbgUpdateMemoryState(hwndDlg); break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_DEBUG_MEMORY_UP:
          memory_adress = (memory_adress - 32) & 0xffffff;
          InvalidateRect(hwndDlg, nullptr, FALSE);
          SetFocus(hwndDlg);
          break;
        case IDC_DEBUG_MEMORY_DOWN:
          memory_adress = (memory_adress + 32) & 0xffffff;
          InvalidateRect(hwndDlg, nullptr, FALSE);
          SetFocus(hwndDlg);
          break;
        case IDC_DEBUG_MEMORY_PGUP:
          memory_adress = (memory_adress - memory_padd) & 0xffffff;
          InvalidateRect(hwndDlg, nullptr, FALSE);
          SetFocus(hwndDlg);
          break;
        case IDC_DEBUG_MEMORY_PGDN:
          memory_adress = (memory_adress + memory_padd) & 0xffffff;
          InvalidateRect(hwndDlg, nullptr, FALSE);
          SetFocus(hwndDlg);
          break;
        case IDC_DEBUG_MEMORY_ASCII:
          memory_ascii = TRUE;
          memory_padd = WDBG_DISASSEMBLY_LINES * 16;
          InvalidateRect(hwndDlg, nullptr, FALSE);
          SetFocus(hwndDlg);
          break;
        case IDC_DEBUG_MEMORY_HEX:
          memory_ascii = FALSE;
          memory_padd = WDBG_DISASSEMBLY_LINES * 32;
          InvalidateRect(hwndDlg, nullptr, FALSE);
          SetFocus(hwndDlg);
          break;
        default: break;
      }
      break;
    default: break;
  }
  return FALSE;
}

/*============================================================================*/
/* DialogProc for our CIA dialog                                              */
/*============================================================================*/

INT_PTR CALLBACK wdbgCIADialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG: return TRUE;
    case WM_PAINT: wdbgUpdateCIAState(hwndDlg); break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_DEBUG_MEMORY_UP: break;
        default: break;
      }
      break;
    default: break;
  }
  return FALSE;
}

/*============================================================================*/
/* DialogProc for our floppy dialog                                           */
/*============================================================================*/

INT_PTR CALLBACK wdbgFloppyDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG: return TRUE;
    case WM_PAINT: wdbgUpdateFloppyState(hwndDlg); break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_DEBUG_MEMORY_UP: break;
        default: break;
      }
      break;
    default: break;
  }
  return FALSE;
}

/*============================================================================*/
/* DialogProc for our blitter dialog                                          */
/*============================================================================*/

INT_PTR CALLBACK wdbgBlitterDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG: Button_SetCheck(GetDlgItem(hwndDlg, IDC_DEBUG_LOGBLT), blitterGetOperationLog()); return TRUE;
    case WM_PAINT: wdbgUpdateBlitterState(hwndDlg); break;
    case WM_DESTROY: blitterSetOperationLog(Button_GetCheck(GetDlgItem(hwndDlg, IDC_DEBUG_LOGBLT))); break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_DEBUG_MEMORY_UP: break;
        default: break;
      }
      break;
    default: break;
  }
  return FALSE;
}

/*============================================================================*/
/* DialogProc for our copper dialog                                           */
/*============================================================================*/

INT_PTR CALLBACK wdbgCopperDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG: return TRUE;
    case WM_PAINT: wdbgUpdateCopperState(hwndDlg); break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_DEBUG_MEMORY_UP: break;
        default: break;
      }
      break;
    default: break;
  }
  return FALSE;
}

/*============================================================================*/
/* DialogProc for our sprite dialog                                           */
/*============================================================================*/

INT_PTR CALLBACK wdbgSpriteDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG: return TRUE;
    case WM_PAINT: wdbgUpdateSpriteState(hwndDlg); break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_DEBUG_MEMORY_UP: break;
        default: break;
      }
      break;
    default: break;
  }
  return FALSE;
}

/*============================================================================*/
/* DialogProc for our screen dialog                                           */
/*============================================================================*/

INT_PTR CALLBACK wdbgScreenDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG: return TRUE;
    case WM_PAINT: wdbgUpdateScreenState(hwndDlg); break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_DEBUG_MEMORY_UP: break;
        default: break;
      }
      break;
    default: break;
  }
  return FALSE;
}

/*============================================================================*/
/* DialogProc for our event dialog                                            */
/*============================================================================*/

INT_PTR CALLBACK wdbgEventDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG: return TRUE;
    case WM_PAINT: wdbgUpdateEventState(hwndDlg); break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_DEBUG_MEMORY_UP: break;
        default: break;
      }
      break;
    default: break;
  }
  return FALSE;
}

/*============================================================================*/
/* DialogProc for our sound dialog                                            */
/*============================================================================*/

INT_PTR CALLBACK wdbgSoundDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG: return TRUE;
    case WM_PAINT: wdbgUpdateSoundState(hwndDlg); break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_DEBUG_MODRIP: modripRIP(); break;
        case IDC_DEBUG_DUMPCHIP: modripChipDump(); break;
        default: break;
      }
      break;
    default: break;
  }
  return FALSE;
}

/*============================================================================*/
/* Runs the debugger                                                          */
/*============================================================================*/

void wdbgDebugSessionRun(HWND parent)
{
  /* The configuration has been activated, but we must prepare the modules */
  /* for emulation ourselves */

  if (wguiCheckEmulationNecessities())
  {
    wdbg_hDialog = parent;
    fellowEmulationStart();
    if (fellowGetPreStartReset()) fellowHardReset();
    wdbgSessionMainDialog();
    fellowEmulationStop();
  }
  else
    MessageBox(parent, "Specified KickImage does not exist", "Configuration Error", 0);
}

#else

HWND wdeb_hDialog = nullptr;
int WDEB_DISASM_LINES = 42;

enum wdeb_actions
{
  WDEB_NO_ACTION,
  WDEB_INIT_DIALOG,
  WDEB_EXIT
};

enum wdeb_actions wdeb_action;

//---------------------------------------------------------------------------
// Utils

void wdebSetRedraw(HWND hwnd, BOOL enableRedraw)
{
  SendMessage(hwnd, WM_SETREDRAW, WPARAM(enableRedraw), 0);
}

void wdebInsertListColumn(HWND hwndDlgItem, char *name, int width, int index)
{
  LV_COLUMN lv_col = {0};
  lv_col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv_col.pszText = name;
  lv_col.cx = width;
  lv_col.iSubItem = index;
  ListView_InsertColumn(hwndDlgItem, index, &lv_col);
}

void wdebInitializeListItemStruct(LV_ITEM *lv_item, char *text, int index, int subIndex)
{
  lv_item->mask = LVIF_TEXT;
  lv_item->pszText = text;
  lv_item->iItem = index;
  lv_item->iSubItem = subIndex;
}

void wdebInsertListItem(HWND hwndDlgItem, char *text, int index, int subIndex)
{
  LV_ITEM lv_item = {0};
  wdebInitializeListItemStruct(&lv_item, text, index, subIndex);
  ListView_InsertItem(hwndDlgItem, &lv_item);
}

void wdebSetListItem(HWND hwndDlgItem, char *text, int index, int subIndex)
{
  LV_ITEM lv_item = {0};
  wdebInitializeListItemStruct(&lv_item, text, index, subIndex);
  ListView_SetItem(hwndDlgItem, &lv_item);
}

void wdebInsertNameValueListItem(HWND hList, char *name, char *value, int index)
{
  wdebInsertListItem(hList, name, index, 0);
  wdebSetListItem(hList, value, index, 1);
}

bool wdeb_is_working = false;

void wdebEnableDisableRunButtons()
{
  BOOL working = (wdeb_is_working) ? TRUE : FALSE;
  ccwButtonEnableConditional(wdeb_hDialog, IDC_BTN_RUN, !working);
  ccwButtonEnableConditional(wdeb_hDialog, IDC_BTN_STEP1, !working);
  ccwButtonEnableConditional(wdeb_hDialog, IDC_BTN_STEP_OVER, !working);
  ccwButtonEnableConditional(wdeb_hDialog, IDC_BTN_STEP1_LINE, !working);
  ccwButtonEnableConditional(wdeb_hDialog, IDC_BTN_STEP1_FRAME, !working);
  ccwButtonEnableConditional(wdeb_hDialog, IDC_BTN_BREAK, working);
}

//---------------------------------------------------------------------------
// Disassembler

void wdebInitInstructionColumns()
{
  HWND hList = GetDlgItem(wdeb_hDialog, IDC_LST_INSTRUCTIONS);

  wdebSetRedraw(hList, FALSE);

  wdebInsertListColumn(hList, "Address", 100, 0);
  wdebInsertListColumn(hList, "Opcode data", 190, 1);
  wdebInsertListColumn(hList, "Instruction", 90, 2);
  wdebInsertListColumn(hList, "Operands", 128, 3);
  ListView_SetColumnWidth(hList, 3, LVSCW_AUTOSIZE_USEHEADER);

  for (int i = 0; i < WDEB_DISASM_LINES; ++i)
  {
    wdebInsertListItem(hList, "", i, 0);
    wdebSetListItem(hList, "", i, 1);
    wdebSetListItem(hList, "", i, 2);
    wdebSetListItem(hList, "", i, 3);
  }

  wdebSetRedraw(hList, TRUE);
}

void wdebUpdateInstructionColumns()
{
  if (wdeb_is_working)
  {
    return;
  }

  HWND hList = GetDlgItem(wdeb_hDialog, IDC_LST_INSTRUCTIONS);
  ULO disasm_pc = cpuGetPC();
  STR inst_address[256];
  STR inst_data[256];
  STR inst_name[256];
  STR inst_operands[256];

  wdebSetRedraw(hList, FALSE);

  for (int i = 0; i < WDEB_DISASM_LINES; i++)
  {
    inst_address[0] = '\0';
    inst_data[0] = '\0';
    inst_name[0] = '\0';
    inst_operands[0] = '\0';
    disasm_pc = cpuDisOpcode(disasm_pc, inst_address, inst_data, inst_name, inst_operands);

    wdebSetListItem(hList, inst_address, i, 0);
    wdebSetListItem(hList, inst_data, i, 1);
    wdebSetListItem(hList, inst_name, i, 2);
    wdebSetListItem(hList, inst_operands, i, 3);
  }

  wdebSetRedraw(hList, TRUE);
}

//---------------------------------------------------------------------------
// Register table

STR *wdbg_registernames[] = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "PC", "USP", "SSP", "MSP", "ISP", "VBR", "SR"};

void wdebInitRegisterColumns()
{
  HWND hList = GetDlgItem(wdeb_hDialog, IDC_LST_REGISTERS);

  wdebSetRedraw(hList, FALSE);

  wdebInsertListColumn(hList, "Register", 64, 0);
  wdebInsertListColumn(hList, "Value", 128, 1);
  ListView_SetColumnWidth(hList, 1, LVSCW_AUTOSIZE_USEHEADER);

  for (int i = 0; i < 23; ++i)
  {
    wdebInsertNameValueListItem(hList, wdbg_registernames[i], "", i);
  }

  wdebSetRedraw(hList, TRUE);
}

void wdebUpdateRegisterColumns()
{
  if (wdeb_is_working)
  {
    return;
  }

  HWND hList = GetDlgItem(wdeb_hDialog, IDC_LST_REGISTERS);
  STR tmp[16];
  ULO col = 0;

  wdebSetRedraw(hList, FALSE);

  for (unsigned int j = 0; j < 2; j++)
  {
    for (unsigned int i = 0; i < 8; i++)
    {
      sprintf(tmp, "%.8X", cpuGetReg(j, i));
      wdebSetListItem(hList, tmp, col++, 1);
    }
  }

  sprintf(tmp, "%.8X", cpuGetPC());
  wdebSetListItem(hList, tmp, col++, 1);

  sprintf(tmp, "%.8X", cpuGetUspDirect());
  wdebSetListItem(hList, tmp, col++, 1);

  sprintf(tmp, "%.8X", cpuGetSspDirect());
  wdebSetListItem(hList, tmp, col++, 1);

  sprintf(tmp, "%.8X", cpuGetMspDirect());
  wdebSetListItem(hList, tmp, col++, 1);

  sprintf(tmp, "%.8X", cpuGetSspDirect());
  wdebSetListItem(hList, tmp, col++, 1);

  sprintf(tmp, "%.8X", cpuGetVbr());
  wdebSetListItem(hList, tmp, col++, 1);

  sprintf(tmp, "%.4X", cpuGetSR());
  wdebSetListItem(hList, tmp, col, 1);

  wdebSetRedraw(hList, TRUE);
}

//---------------------------------------------------------------------------
// Generic info table

void wdebInitInfoColumns()
{
  HWND hList = GetDlgItem(wdeb_hDialog, IDC_LST_INFO);

  wdebSetRedraw(hList, FALSE);

  wdebInsertListColumn(hList, "Name", 80, 0);
  wdebInsertListColumn(hList, "Value", 128, 1);

  ULO index = 0;
  wdebInsertNameValueListItem(hList, "Frame", "", index++);
  wdebInsertNameValueListItem(hList, "Line", "", index++);
  wdebInsertNameValueListItem(hList, "Cycle", "", index++);
  wdebInsertNameValueListItem(hList, "Interlace", "", index);

  ListView_SetColumnWidth(hList, 1, LVSCW_AUTOSIZE_USEHEADER);

  wdebSetRedraw(hList, TRUE);
}

void wdebUpdateInfoColumns()
{
  if (wdeb_is_working)
  {
    return;
  }

  HWND hList = GetDlgItem(wdeb_hDialog, IDC_LST_INFO);
  char tmpStr[256];

  wdebSetRedraw(hList, FALSE);

  int index = 0;

  sprintf(tmpStr, "%I64u", scheduler.GetRasterFrameCount());
  wdebSetListItem(hList, tmpStr, index++, 1);

  sprintf(tmpStr, "%u", scheduler.GetRasterY());
  wdebSetListItem(hList, tmpStr, index++, 1);

  ULO remainder = (ULO)(((float)cpuIntegrationGetTimeUsedRemainder()) * 100 / 4096.0f);
  sprintf(tmpStr, "%u.%.2u", scheduler.GetRasterX(), remainder);
  wdebSetListItem(hList, tmpStr, index++, 1);

  wdebSetListItem(hList, drawGetUseInterlacedRendering() ? "Yes" : "No", index, 1);

  wdebSetRedraw(hList, TRUE);
}

//---------------------------------------------------------------------------
// Log display

void wdebInitLogColumns()
{
  HWND hList = GetDlgItem(wdeb_hDialog, IDC_DEBUG_LOG);

  wdebSetRedraw(hList, FALSE);

  wdebInsertListColumn(hList, "Line", 64, 0);
  wdebInsertListColumn(hList, "Cycle", 64, 1);
  wdebInsertListColumn(hList, "Source", 128, 2);
  wdebInsertListColumn(hList, "Description", 128, 3);
  ListView_SetColumnWidth(hList, 3, LVSCW_AUTOSIZE_USEHEADER);

  wdebSetRedraw(hList, TRUE);
}

void wdebUpdateLogColumns()
{
  // if (wdeb_is_working)
  //{
  //   return;
  // }

  // HWND hList = GetDlgItem(wdeb_hDialog, IDC_DEBUG_LOG);
  // char tmpStr[256];

  // bool bitplaneDMAFilter = ccwButtonGetCheckBool(wdeb_hDialog, IDC_FILTER_BITPLANE_DMA);
  // bool copperDMAFilter = ccwButtonGetCheckBool(wdeb_hDialog, IDC_FILTER_COPPER_DMA);
  // bool diwyFilter = ccwButtonGetCheckBool(wdeb_hDialog, IDC_FILTER_DIWY);
  // bool shifterFilter = ccwButtonGetCheckBool(wdeb_hDialog, IDC_FILTER_SHIFTER);

  // wdebSetRedraw(hList, FALSE);

  // ListView_DeleteAllItems(hList);

  // int index = 0;
  // for (auto* entry : DebugLog.DebugLog)
  //{
  //   if ((entry->Source == DebugLogSource::CHIP_BUS_BITPLANE_DMA && bitplaneDMAFilter)
  //     || (entry->Source == DebugLogSource::CHIP_BUS_COPPER_DMA && copperDMAFilter)
  //     || (entry->Source == DebugLogSource::BITPLANESHIFTER_ACTION && shifterFilter)
  //     || (entry->Source == DebugLogSource::DIWY_ACTION && diwyFilter))
  //   {
  //     sprintf(tmpStr, "%u", entry->Timestamp.Line);
  //     wdebInsertListItem(hList, tmpStr, index, 0);

  //    sprintf(tmpStr, "%u", entry->Timestamp.Pixel);
  //    wdebSetListItem(hList, tmpStr, index, 1);

  //    wdebSetListItem(hList, entry->GetSource(), index, 2);

  //    entry->GetDescription(tmpStr);
  //    wdebSetListItem(hList, tmpStr, index, 3);
  //    index++;
  //  }
  //}

  // if (index > 0)
  //{
  //   ListView_EnsureVisible(hList, index - 1, FALSE);
  // }

  // wdebSetRedraw(hList, TRUE);
}

//---------------------------------------------------------------------------

void wdebPaintAll()
{
  wdebUpdateInstructionColumns();
  wdebUpdateRegisterColumns();
  wdebUpdateLogColumns();
  wdebUpdateInfoColumns();
}

void wdebInitAll()
{
  wdebInitInstructionColumns();
  wdebInitRegisterColumns();
  wdebInitInfoColumns();
  wdebInitLogColumns();
  wdebEnableDisableRunButtons();
}

void wdebRun(dbg_operations operation)
{
  if (!wdeb_is_working)
  {
    wdeb_is_working = true;
    wdebEnableDisableRunButtons();
    winDrvDebugStart(operation, 0);
    wdeb_is_working = false;
    wdebEnableDisableRunButtons();
    wdebPaintAll();
  }
}

void wdebBreak()
{
  if (wdeb_is_working)
  {
    fellowRequestEmulationStop();
  }
}

void wdebExit()
{
  if (wdeb_is_working)
  {
    wdebBreak();
  }
  wdeb_action = WDEB_EXIT;
}

INT_PTR CALLBACK wdebDebuggerDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG: wdeb_action = WDEB_INIT_DIALOG; return TRUE;
    case WM_PAINT: wdebPaintAll(); break;
    case WM_DESTROY: break;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
      {
        switch (LOWORD(wParam))
        {
          case IDC_BTN_STEP1: wdebRun(dbg_operations::DBG_STEP); break;
          case IDC_BTN_STEP_OVER: wdebRun(dbg_operations::DBG_STEP_OVER); break;
          case IDC_BTN_STEP1_LINE: wdebRun(dbg_operations::DBG_STEP_LINE); break;
          case IDC_BTN_STEP1_FRAME: wdebRun(dbg_operations::DBG_STEP_FRAME); break;
          case IDC_BTN_RUN: wdebRun(dbg_operations::DBG_RUN); break;
          case IDC_BTN_BREAK: wdebBreak(); break;
          case IDC_FILTER_BITPLANE_DMA:
          case IDC_FILTER_COPPER_DMA:
          case IDC_FILTER_SHIFTER:
          case IDC_FILTER_DIWY: wdebUpdateLogColumns(); break;
          case IDC_BTN_MODRIP: modripRIP(); break;
          case IDC_BTN_DUMPCHIP: modripChipDump(); break;
          case IDOK:
          case IDCANCEL: wdebExit(); break;
          default: break;
        }
      }
      break;
    default: break;
  }

  return FALSE;
}

void wdebCreateDialog()
{
  wdeb_hDialog = CreateDialog(win_drv_hInstance, MAKEINTRESOURCE(IDD_DEBUGGER), NULL, wdebDebuggerDialogProc);
  ShowWindow(wdeb_hDialog, win_drv_nCmdShow);
}

void wdebCloseDialog()
{
  DestroyWindow(wdeb_hDialog);
  wdeb_hDialog = nullptr;
}

void wdebDoMessages()
{
  bool end_loop = false;
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
        wdebInitAll();
        wdebPaintAll();
        break;
      case WDEB_EXIT: end_loop = true; break;
      default: break;
    }

    wdeb_action = WDEB_NO_ACTION;
  }
}

void wdebDebug()
{
  /* The configuration has been activated, but we must prepare the modules */
  /* for emulation ourselves */

  if (wguiCheckEmulationNecessities())
  {
    bool previousPauseEmulationOnLostFocus = gfxDrvCommon->GetPauseEmulationWhenWindowLosesFocus();
    gfxDrvCommon->SetPauseEmulationWhenWindowLosesFocus(false);

    if (!fellowEmulationStart())
    {
      MessageBox(nullptr, "Emulation debug session failed to start", "Startup Error", 0);
      fellowEmulationStop();
      gfxDrvCommon->SetPauseEmulationWhenWindowLosesFocus(previousPauseEmulationOnLostFocus);
      return;
    }

    if (fellowGetPreStartReset())
    {
      fellowHardReset();
    }

    wdeb_action = WDEB_NO_ACTION;
    wdebCreateDialog();
    wdebDoMessages();
    wdebCloseDialog();

    fellowEmulationStop();
    gfxDrvCommon->SetPauseEmulationWhenWindowLosesFocus(previousPauseEmulationOnLostFocus);
  }
  else
  {
    MessageBox(nullptr, "Specified KickImage does not exist", "Configuration Error", 0);
  }
}

#endif