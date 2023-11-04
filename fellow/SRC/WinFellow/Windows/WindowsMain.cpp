/*=========================================================================*/
/* Fellow                                                                  */
/* This file is for whatever peculiarities we need to support on Windows   */
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

#include <windows.h>

#include "gui_general.h"
#include "Defs.h"
#include "versioninfo.h"
#include "WindowsUI.h"
#include "WindowsDebugger.h"
#include "BusScheduler.h"
#include "WindowsMain.h"
#include "FellowMain.h"
#include "MouseDriver.h"
#include "Keyboard.h"
#include "JoystickDriver.h"
#include "KeyboardDriver.h"
#include "GfxDrvCommon.h"
#include "VirtualHost/Core.h"

#ifdef RETRO_PLATFORM
#include "retroplatform.h"
#endif

// user mode crash dumps shall be captured
#include <Dbghelp.h>

extern int __cdecl main(int, const char **);

/*===========================================================================*/
/* Records some startup data                                                 */
/*===========================================================================*/

HINSTANCE win_drv_hInstance;
int win_drv_nCmdShow;

/* thread name info struct */

typedef struct tagTHREADNAME_INFO
{
  DWORD dwType;     // must be 0x1000
  LPCSTR szName;    // pointer to name (in user addr space)
  DWORD dwThreadID; // thread ID (-1=caller thread)
  DWORD dwFlags;    // reserved for future use, must be zero
} THREADNAME_INFO;

/*===========================================================================*/
/* Start Fellow and main message loop                                        */
/*===========================================================================*/

HANDLE win_drv_emulation_ended;

/* Thread entry points */

DWORD WINAPI winDrvFellowRunStart(LPVOID in)
{
  winDrvSetThreadName(-1, "fellowRun()");
  fellowRun();
  SetEvent(win_drv_emulation_ended);
  return 0;
}

DWORD WINAPI winDrvFellowStepOneStart(LPVOID in)
{
  winDrvSetThreadName(-1, "fellowStepOne()");
  fellowStepOne();
  SetEvent(win_drv_emulation_ended);
  return 0;
}

DWORD WINAPI winDrvFellowStepOverStart(LPVOID in)
{
  winDrvSetThreadName(-1, "fellowStepOver()");
  fellowStepOver();
  SetEvent(win_drv_emulation_ended);
  return 0;
}

DWORD WINAPI winDrvFellowRunDebugStart(LPVOID in)
{
  winDrvSetThreadName(-1, "fellowRunDebug()");
  fellowRunDebug((uint32_t)in);
  SetEvent(win_drv_emulation_ended);
  return 0;
}

enum MultiEventTypes
{
  met_emulation_ended = 0,
  met_mouse_data = 1,
  met_kbd_data = 2,
  met_messages = 3
};

extern BOOLE mouse_drv_initialization_failed;
extern HANDLE mouse_drv_DIevent;
extern bool kbd_drv_initialization_failed;
extern HANDLE kbd_drv_DIevent;

uint32_t winDrvInitializeMultiEventArray(HANDLE *multi_events, enum MultiEventTypes *object_mapping)
{
  uint32_t event_count = 0;

  multi_events[event_count] = win_drv_emulation_ended;
  object_mapping[event_count++] = met_emulation_ended;
  if (!mouse_drv_initialization_failed)
  {
    multi_events[event_count] = mouse_drv_DIevent;
    object_mapping[event_count++] = met_mouse_data;
  }
  if (!kbd_drv_initialization_failed)
  {
    multi_events[event_count] = kbd_drv_DIevent;
    object_mapping[event_count++] = met_kbd_data;
  }
  object_mapping[event_count] = met_messages;
  return event_count;
}

/* added setting of thread names for easier debugging */

void winDrvSetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = szThreadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags = 0;

  __try
  {
    RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR *)&info);
  }
  __except (EXCEPTION_CONTINUE_EXECUTION)
  {
  }
}

void winDrvEmulate(LPTHREAD_START_ROUTINE startfunc, void *param)
{
  DWORD dwThreadId;
  MSG myMsg;
  HANDLE multi_events[3];
  enum MultiEventTypes object_mapping[4];

  win_drv_emulation_ended = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  _core.Log->AddLog("fellowEmulationStart() finished\n");
  HANDLE FellowThread = CreateThread(
      nullptr,      // Security attr
      0,            // Stack Size
      startfunc,    // Thread procedure
      param,        // Thread parameter
      0,            // Creation flags
      &dwThreadId); // ThreadId
  SetTimer(gfxDrvCommon->GetHWND(), 1, 10, nullptr);
  uint32_t event_count = winDrvInitializeMultiEventArray(multi_events, object_mapping);
  BOOLE keep_on_waiting = TRUE;
  while (keep_on_waiting)
  {
    DWORD dwEvt = MsgWaitForMultipleObjects(event_count, multi_events, FALSE, INFINITE, QS_ALLINPUT);
    if ((dwEvt >= WAIT_OBJECT_0) && (dwEvt <= (WAIT_OBJECT_0 + event_count)))
    {
      switch (object_mapping[dwEvt - WAIT_OBJECT_0])
      {
        case met_emulation_ended:
          /* Emulation session is ending */
          _core.Log->AddLog("met_emulation_ended\n");
          keep_on_waiting = FALSE;
          break;
        case met_mouse_data:
          /* Deal with mouse input */
          mouseDrvMovementHandler();
          break;
        case met_kbd_data:
          /* Deal with kbd input */
          kbdDrvKeypressHandler();
          break;
        case met_messages:
          /* Deal with windows messages */
          while (PeekMessage(&myMsg, nullptr, 0, 0, PM_REMOVE))
          {
            TranslateMessage(&myMsg);
            DispatchMessage(&myMsg);
          }
      }
    }
  }
  CloseHandle(FellowThread);
  CloseHandle(win_drv_emulation_ended);
}

void winDrvEmulationStart()
{
  if (fellowEmulationStart())
  {
    winDrvEmulate(winDrvFellowRunStart, nullptr);
  }
  else
  {
    fellowShowRequester(FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_ERROR, "Emulation session failed to start up");
  }
  fellowEmulationStop();
}

/*===========================================================================*/
/* Controls multi-threaded execution of debugger.                            */
/*===========================================================================*/

DWORD WINAPI winDrvFellowStepOne(LPVOID in)
{
  winDrvEmulate(winDrvFellowStepOneStart, nullptr);
  SetEvent(win_drv_emulation_ended);
  return 0;
}

DWORD WINAPI winDrvFellowStepOver(LPVOID in)
{
  winDrvEmulate(winDrvFellowStepOverStart, nullptr);
  SetEvent(win_drv_emulation_ended);
  return 0;
}

DWORD WINAPI winDrvFellowRunDebug(LPVOID in)
{
  winDrvEmulate(winDrvFellowRunDebugStart, in);
  SetEvent(win_drv_emulation_ended);
  return 0;
}

BOOLE winDrvDebugStart(dbg_operations operation, HWND hwndDlg)
{

  switch (operation)
  {
    case dbg_operations::DBG_STEP: winDrvFellowStepOne((LPVOID)1); break;
    case dbg_operations::DBG_STEP_OVER: winDrvFellowStepOver((LPVOID)1); break;
    case dbg_operations::DBG_RUN: winDrvFellowRunDebug((LPVOID) nullptr); break;
    default: return FALSE;
  }
  return TRUE;
}

/*===========================================================================*/
/* Timer controls execution of this function                                 */
/*===========================================================================*/

void winDrvHandleInputDevices()
{
  joyDrvMovementHandler();
}

/*===========================================================================*/
/* Sets the version and path registry keys                                   */
/*===========================================================================*/

void winDrvSetKey(const char *path, const char *name, const char *value)
{
  HKEY hkey;
  DWORD disposition;
  LONG result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, path, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hkey, &disposition);
  if ((result == ERROR_SUCCESS) && ((disposition == REG_CREATED_NEW_KEY) || (disposition == REG_OPENED_EXISTING_KEY)))
  {
    RegSetValueEx(hkey, name, 0, REG_SZ, (BYTE *)value, (DWORD)strlen(value));
    RegCloseKey(hkey);
  }
}

void winDrvSetRegistryKeys(const char **argv)
{
  char p[1024];
  p[0] = '\0';
  winDrvSetKey("Software\\WinFellow", "version", FELLOWNUMERICVERSION);
  _fullpath(p, argv[0], 1024);
  char *locc = strrchr(p, '\\');
  if (locc == nullptr)
  {
    p[0] = '\0';
  }
  else
  {
    BOOLE isreadonly = FALSE;
    *locc = '\0';
    if (p[1] == ':' && p[2] == '\\')
    {
      char p2[8];
      p2[0] = p[0];
      p2[1] = ':';
      p2[2] = '\\';
      p2[3] = '\0';
      isreadonly = (GetDriveType(p2) == DRIVE_CDROM);
    }
    if (!isreadonly)
    {
      winDrvSetKey("Software\\WinFellow", "path", p);
    }
  }
}

/*===========================================================================*/
/* Command line conversion routines                                          */
/*===========================================================================*/

/* Returns the first character in the next argument                          */

char *winDrvCmdLineGetNextFirst(char *lpCmdLine)
{
  while (*lpCmdLine == ' ' && *lpCmdLine != '\0')
  {
    lpCmdLine++;
  }
  return (*lpCmdLine == '\0') ? nullptr : lpCmdLine;
}

/* Returns the first character after the next argument                       */

char *winDrvCmdLineGetNextEnd(char *lpCmdLine)
{
  int InString = FALSE;

  while (((*lpCmdLine != ' ') && (*lpCmdLine != '\0')) || (InString && (*lpCmdLine != '\0')))
  {
    if (*lpCmdLine == '\"')
    {
      InString = !InString;
    }
    lpCmdLine++;
  }
  return lpCmdLine;
}

/* Returns an argv vector and takes argc as a pointer parameter              */
/* Must free memory argv on exit                                             */

const char **winDrvCmdLineMakeArgv(char *lpCmdLine, int *argc)
{
  int elements = 0;

  char *tmp = winDrvCmdLineGetNextFirst(lpCmdLine);
  if (tmp != nullptr)
  {
    while ((tmp = winDrvCmdLineGetNextFirst(tmp)) != nullptr)
    {
      tmp = winDrvCmdLineGetNextEnd(tmp);
      elements++;
    }
  }
  const char **argv = (const char **)malloc(sizeof(const char **) * (elements + 2));
  argv[0] = "winfellow.exe";
  char *argend = lpCmdLine;
  for (int i = 1; i <= elements; i++)
  {
    char *argstart = winDrvCmdLineGetNextFirst(argend);
    argend = winDrvCmdLineGetNextEnd(argstart);
    if (*argstart == '\"')
    {
      argstart++;
    }
    if (*(argend - 1) == '\"')
    {
      argend--;
    }
    *argend++ = '\0';
    argv[i] = argstart;
  }
  argv[elements + 1] = nullptr;
  *argc = elements + 1;
  return argv;
}

/*============================================================================*/
/* exception handling to generate minidumps                                   */
/*============================================================================*/

typedef BOOL(__stdcall *tMDWD)(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    OPTIONAL IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    OPTIONAL IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL);

void winDrvWriteMinidump(EXCEPTION_POINTERS *e)
{
  char name[MAX_PATH], filename[MAX_PATH];
  HINSTANCE hDbgHelp = LoadLibraryA("dbghelp.dll");
  SYSTEMTIME t;

  if (hDbgHelp == nullptr) return;

  tMDWD pMiniDumpWriteDump = (tMDWD)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");

  if (pMiniDumpWriteDump == nullptr) return;

  GetSystemTime(&t);

  wsprintfA(filename, "WinFellow_%s_%4d%02d%02d_%02d%02d%02d.dmp", FELLOWNUMERICVERSION, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

  _core.Fileops->GetGenericFileName(name, "WinFellow", filename);

  _core.Log->AddLog("Unhandled exception detected, write minidump to %s...\n", name);

  HANDLE hFile = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (hFile == INVALID_HANDLE_VALUE) return;

  MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
  exceptionInfo.ThreadId = GetCurrentThreadId();
  exceptionInfo.ExceptionPointers = e;
  exceptionInfo.ClientPointers = FALSE;

  pMiniDumpWriteDump(
      GetCurrentProcess(),
      GetCurrentProcessId(),
      hFile,
      MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
      e ? &exceptionInfo : nullptr,
      nullptr,
      nullptr);

  CloseHandle(hFile);

  return;
}

LONG CALLBACK winDrvUnhandledExceptionHandler(EXCEPTION_POINTERS *e)
{
  winDrvWriteMinidump(e);
  return EXCEPTION_CONTINUE_SEARCH;
}

// #endif

int WINAPI WinMain(
    HINSTANCE hInstance,     // handle to current instance
    HINSTANCE hPrevInstance, // handle to previous instance
    LPSTR lpCmdLine,         // pointer to command line
    int nCmdShow)            // show state of window
{
  SetUnhandledExceptionFilter(winDrvUnhandledExceptionHandler);

#ifdef _FELLOW_DEBUG_CRT_MALLOC
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  char *cmdline = (char *)malloc(strlen(lpCmdLine) + 1);
  int argc;

  winDrvSetThreadName(-1, "WinMain()");
  win_drv_nCmdShow = nCmdShow;
  win_drv_hInstance = hInstance;
  strcpy(cmdline, lpCmdLine);
  const char **argv = winDrvCmdLineMakeArgv(cmdline, &argc);
  winDrvSetRegistryKeys(argv);

  int result = main(argc, argv);

  free(cmdline);
  free(argv);

  return result;
}

#ifdef _FELLOW_DEBUG_CRT_MALLOC

/*============================================================================*/
/* detect memory leaks in debug builds; called after main                     */
/*============================================================================*/

int winDrvDetectMemoryLeaks()
{
  char stOutputFileName[CFG_FILENAME_LENGTH];
  char strLogFileName[CFG_FILENAME_LENGTH];
  SYSTEMTIME t;

  GetSystemTime(&t);

  wsprintfA(strLogFileName, "WinFellowCrtMallocReport_%s_%4d%02d%02d_%02d%02d%02d.log", FELLOWNUMERICVERSION, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

  _core.Fileops->GetGenericFileName(stOutputFileName, "WinFellow", strLogFileName);

  HANDLE hLogFile = CreateFile(stOutputFileName, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_WARN, hLogFile);

  if (!_CrtDumpMemoryLeaks())
  {
    CloseHandle(hLogFile);
    remove(stOutputFileName);
  }
  else
    CloseHandle(hLogFile);

  return 0;
}

/*============================================================================*/
/* ensure winDrvDetectMemoryLeaks is called after main (CRT Link-Time Tables) */
/*============================================================================*/

typedef int cb();

#pragma data_seg(".CRT$XPU")
static cb *autoexit[] = {winDrvDetectMemoryLeaks};

#pragma data_seg() /* reset data-segment */

#endif