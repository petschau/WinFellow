#pragma once

/*===========================================================================*/
/* Windows data, misc.                                                       */
/*===========================================================================*/

#include "wdbg.h"

extern HINSTANCE win_drv_hInstance;
extern int win_drv_nCmdShow;

extern void winDrvHandleInputDevices(void);
extern void winDrvEmulationStart(void);
extern BOOLE winDrvDebugStart(dbg_operations operation, HWND hwndDlg);
extern void winDrvSetThreadName(DWORD dwThreadID, LPCSTR szThreadName);
