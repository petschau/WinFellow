/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Windows GUI code for debugger                                              */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "defs.h"

#ifdef WGUI

#include <windef.h>
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
#include "cpudis.h"
#include "kbd.h"
#include "fmem.h"


HWND wdbg_hDialog;                           /* Handle of the main dialog box */


/*============================================================================*/
/* Updates the CPU state in the main window                                   */
/*============================================================================*/

STR *wdbgGetDataRegistersStr(STR *s) {
  sprintf(s,
	  "D0: %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X :D7",
	  d[0],
	  d[1],
	  d[2],
	  d[3],
	  d[4],
	  d[5],
	  d[6],
	  d[7]);
  return s;
}

STR *wdbgGetAddressRegistersStr(STR *s) {
  sprintf(s,
	  "A0: %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X :A7",
	  a[0],
	  a[1],
	  a[2],
	  a[3],
	  a[4],
	  a[5],
	  a[6],
	  a[7]);
  return s;
}

STR *wdbgGetSpecialRegistersStr(STR *s) {
  sprintf(s,
	  "USP:%.8X SSP:%.8X SR:%.4X FRAME: %d y: %d x: %d",
	  (sr & 0x2000) ? usp : a[7],
	  (sr & 0x2000) ? a[7] : ssp,
	  sr,
	  draw_frame_count,
	  graph_raster_y,
	  graph_raster_x);
  return s;
}

void wdbgExtendStringTo80Columns(STR *s) {
  ULO i;
  BOOLE zero_found = FALSE;

  for (i = 0; i < 79; i++) {
    if (!zero_found) zero_found = (s[i] == '\0');
    if (zero_found) s[i] = ' ';
  }
  s[80] = '\0';
}

ULO wdbgLineOut(HDC hDC, STR *s, ULO x, ULO y) {
  struct tagSIZE text_size;
  SetBkMode(hDC, OPAQUE);  
  wdbgExtendStringTo80Columns(s);
  TextOut(hDC, x, y, s, strlen(s));
  GetTextExtentPoint32(hDC, s, strlen(s), (LPSIZE) &text_size);
  return y + text_size.cy;
}

STR *wdbgGetDisassemblyLineStr(STR *s, ULO *disasm_pc) {
  *disasm_pc = disOpcode(*disasm_pc, s);  
  return s;
}


#define WDBG_CPU_REGISTERS_X 24
#define WDBG_CPU_REGISTERS_Y 26
#define WDBG_DISASSEMBLY_X 16
#define WDBG_DISASSEMBLY_Y 96
#define WDBG_DISASSEMBLY_LINES 17
#define WDBG_DISASSEMBLY_INDENT 16

void wdbgUpdateCPUState(HWND hwndDlg) {
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
/* DialogProc for our main Dialog                                             */
/*============================================================================*/

BOOL CALLBACK wdbgMainDialogProc(HWND hwndDlg,
				 UINT uMsg,
				 WPARAM wParam,
				 LPARAM lParam) {
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
          f12pressed = TRUE;
	  InvalidateRect(hwndDlg, NULL, FALSE); 
          break;
	case IDCANCEL:
	  EndDialog(hwndDlg, LOWORD(wParam));
	  return TRUE;
        default:
          break;
      }
      break;
    case WM_DESTROY:
      EndDialog(hwndDlg, LOWORD(wParam));
      return TRUE;
  }
  return FALSE;
}
 

/*============================================================================*/
/* Runs the debugger                                                          */
/*============================================================================*/

void wdbgDebugSessionRun(HWND parent) {
  BOOLE quit_emulator = FALSE;
  BOOLE debugger_start = FALSE;

  /* The configuration has been activated, but we must prepare the modules */
  /* for emulation ourselves */

  fellowEmulationStart();
  if (fellowGetPreStartReset()) fellowHardReset();
  DialogBox(win_drv_hInstance,
	    MAKEINTRESOURCE(IDD_DEBUG_CPU),
	    parent,
	    wdbgMainDialogProc);
  fellowEmulationStop();
}


#endif /* WGUI */
