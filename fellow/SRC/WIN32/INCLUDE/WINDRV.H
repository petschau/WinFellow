/*===========================================================================*/
/* Windows data, misc.                                                       */
/*===========================================================================*/

#ifndef WinDrv_H
#define WinDrv_H

#include "wdbg.h"

extern HINSTANCE win_drv_hInstance;
extern int win_drv_nCmdShow;

extern HWND gfx_drv_hwnd;
extern void winDrvHandleInputDevices(void);
extern void winDrvEmulationStart(void);
extern BOOLE winDrvDebugStart(dbg_operations operation, HWND hwndDlg);
extern void winDrvSetThreadName(DWORD dwThreadID, LPCSTR szThreadName);

#endif
