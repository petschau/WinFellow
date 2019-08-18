/*===========================================================================*/
/* Windows data, misc.                                                       */
/*===========================================================================*/

#pragma once

#include "WDBG.H"

extern HINSTANCE win_drv_hInstance;
extern int win_drv_nCmdShow;

extern void winDrvEmulationStart();

extern bool winDrvDebugStartClient();
extern bool winDrvDebugStart(dbg_operations operation, ULO breakpoint);
extern void winDrvDebugMessageLoop();
extern void winDrvDebugStopMessageLoop();

extern bool winDrvRunInNewThread(LPTHREAD_START_ROUTINE func, LPVOID in);
extern void winDrvSetThreadName(DWORD dwThreadID, LPCSTR szThreadName);

extern void winDrvHandleInputDevices();
