/*===========================================================================*/
/* This file is for whatever peculiarities we need to support on Windows     */
/*===========================================================================*/

#include <windows.h>
#include "gui_general.h"
#include "defs.h"
#include "wgui.h"
#include "wdbg.h"
#include "bus.h"
#include "windrv.h"
#include "fellow.h"
#include "mousedrv.h"
#include "kbd.h"
#include "joydrv.h"
#include "kbddrv.h"

extern int main(int, char **);


/*===========================================================================*/
/* Records some startup data                                                 */
/*===========================================================================*/

HINSTANCE win_drv_hInstance;
int win_drv_nCmdShow;


/*===========================================================================*/
/* Start Fellow and main message loop                                        */
/*===========================================================================*/

HANDLE win_drv_emulation_ended;

/* Thread entry points */

DWORD WINAPI winDrvFellowRunStart(void* in)
{
  fellowRun();
  SetEvent(win_drv_emulation_ended);
  return 0;
}

DWORD WINAPI winDrvFellowStepOneStart(void* in)
{
  fellowStepOne();
  SetEvent(win_drv_emulation_ended);
  return 0;
}

DWORD WINAPI winDrvFellowStepOverStart(void* in)
{
  fellowStepOver();
  SetEvent(win_drv_emulation_ended);
  return 0;
}

DWORD WINAPI winDrvFellowRunDebugStart(void* in)
{
  fellowRunDebug((ULO) in);
  SetEvent(win_drv_emulation_ended);
  return 0;
}


enum MultiEventTypes {
  met_emulation_ended = 0, 
  met_mouse_data = 1, 
  met_kbd_data = 2, 
  met_messages = 3
};

extern BOOLE mouse_drv_initialization_failed;
extern HANDLE mouse_drv_DIevent;
extern BOOLE kbd_drv_initialization_failed;
extern HANDLE kbd_drv_DIevent;

ULO winDrvInitializeMultiEventArray(HANDLE *multi_events,
				    enum MultiEventTypes *object_mapping) {
  ULO event_count = 0;

  multi_events[event_count] = win_drv_emulation_ended;
  object_mapping[event_count++] = met_emulation_ended;
  if (!mouse_drv_initialization_failed) {
    multi_events[event_count] = mouse_drv_DIevent;
    object_mapping[event_count++] = met_mouse_data;
  }
  if (!kbd_drv_initialization_failed) {
    multi_events[event_count] = kbd_drv_DIevent;
    object_mapping[event_count++] = met_kbd_data;
  }
  object_mapping[event_count] = met_messages;
  return event_count;
}

#ifdef _MSC_VER
#ifndef _DEBUG
#pragma optimize("g", off)
#endif
#endif

void winDrvEmulate(void *startfunc, void *param)
{
  DWORD dwThreadId;
  MSG myMsg;
  HANDLE FellowThread;
  DWORD dwEvt;
  HANDLE multi_events[3];
  ULO event_count;
  enum MultiEventTypes object_mapping[3];
  BOOLE keep_on_waiting;
  
  win_drv_emulation_ended = CreateEvent(NULL, FALSE, FALSE, NULL);
  fellowAddLog("fellowEmulationStart() finished\n");
  FellowThread = CreateThread(NULL,     // Security attr
			       0,                   // Stack Size
			       startfunc,   // Thread procedure
			       param,	            // Thread parameter
			       0,                   // Creation flags
			       &dwThreadId);        // ThreadId
  SetTimer(gfx_drv_hwnd, 1, 10, NULL);
  event_count = winDrvInitializeMultiEventArray(multi_events, object_mapping);
  keep_on_waiting = TRUE;
  while (keep_on_waiting) {
    dwEvt = MsgWaitForMultipleObjects(event_count,
				      multi_events,
				      FALSE,
				      INFINITE,
				      QS_ALLINPUT);
    if ((dwEvt >= WAIT_OBJECT_0) && (dwEvt <= (WAIT_OBJECT_0 + event_count))) {
      switch (object_mapping[dwEvt - WAIT_OBJECT_0]) {
        case met_emulation_ended:
	  /* Emulation session is ending */
          fellowAddLog("met_emulation_ended\n");
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
          while (PeekMessage(&myMsg,
		             NULL,
		             0,
		             0,
		             PM_REMOVE)) {
            TranslateMessage(&myMsg);
            DispatchMessage(&myMsg);
	  }
      }
    }
  }
  CloseHandle(FellowThread);
  CloseHandle(win_drv_emulation_ended);
}

#ifdef _MSC_VER
#ifndef _DEBUG
#pragma optimize("g", on)
#endif
#endif

void winDrvEmulationStart(void) {
  if (fellowEmulationStart()) winDrvEmulate(winDrvFellowRunStart, 0);
  else wguiRequester("Emulation session failed to start up", "", "");
  fellowEmulationStop();
}


/*===========================================================================*/
/* Controls multi-threaded execution of debugger.                            */
/*===========================================================================*/

DWORD WINAPI winDrvFellowStepOne(void* in)
{
  winDrvEmulate(winDrvFellowStepOneStart, NULL);
  SetEvent(win_drv_emulation_ended);
  return 0;
}


DWORD WINAPI winDrvFellowStepOver(void* in)
{
  winDrvEmulate(winDrvFellowStepOverStart, NULL);
  SetEvent(win_drv_emulation_ended);
  return 0;
}

DWORD WINAPI winDrvFellowRunDebug(void* in)
{
  winDrvEmulate(winDrvFellowRunDebugStart, in);
  SetEvent(win_drv_emulation_ended);
  return 0;
}


BOOLE winDrvDebugStart(dbg_operations operation, HWND hwndDlg) {

  switch (operation) {
    case DBG_STEP:      winDrvFellowStepOne((void*) 1); break;
    case DBG_STEP_OVER: winDrvFellowStepOver((void*) 1); break;
    case DBG_RUN:       winDrvFellowRunDebug((void*) 0); break;
    default:            return FALSE;
  }
  return TRUE;
}


/*===========================================================================*/
/* Timer controls execution of this function                                 */
/*===========================================================================*/

void winDrvHandleInputDevices(void) {
  joyDrvMovementHandler();
//  kbdDrvKeypressHandler();
}

/*===========================================================================*/
/* Sets the version and path registry keys                                   */
/*===========================================================================*/

void winDrvSetKey(char *path, char *name, char *value) {
  HKEY hkey;
  DWORD disposition;
  LONG result = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			       path,
			       0,
			       0,
			       REG_OPTION_NON_VOLATILE,
			       KEY_ALL_ACCESS,
			       NULL,
			       &hkey,
			       &disposition);
  if ((result == ERROR_SUCCESS) &&
      ((disposition == REG_CREATED_NEW_KEY) ||
       (disposition == REG_OPENED_EXISTING_KEY)))
  {
    RegSetValueEx(hkey,
		  name,
		  0,
		  REG_SZ,
		  value,
		  strlen(value));
    RegCloseKey(hkey);
  }
}

void winDrvSetRegistryKeys(char **argv) {
  char p[1024];
  char *locc;
  p[0] = '\0';
  winDrvSetKey("Software\\WinFellow", "version", "0.4.3.2");
  _fullpath(p, argv[0], 1024);
  locc = strrchr(p, '\\');
  if (locc == NULL) p[0] = '\0';
  else {
    BOOLE isreadonly = FALSE;
    *locc = '\0';
    if (p[1] == ':' && p[2] == '\\')
    {
      UINT drivetype;
      char p2[8];
      p2[0] = p[0];
      p2[1] = ':';
      p2[2] = '\\';
      p2[3] = '\0';
      isreadonly = (GetDriveType(p2) == DRIVE_CDROM);
    }
    if (!isreadonly)
      winDrvSetKey("Software\\WinFellow", "path", p);
  }
}

/*===========================================================================*/
/* Command line conversion routines                                          */
/*===========================================================================*/

/* Returns the first character in the next argument                          */

char *winDrvCmdLineGetNextFirst(char *lpCmdLine) {
  while (*lpCmdLine == ' ' && *lpCmdLine != '\0')
    lpCmdLine++;
  return (*lpCmdLine == '\0') ? NULL : lpCmdLine;
}


/* Returns the first character after the next argument                       */

char *winDrvCmdLineGetNextEnd(char *lpCmdLine) {
  int InString = FALSE;
  
  while (((*lpCmdLine != ' ') && (*lpCmdLine != '\0')) ||
    (InString && (*lpCmdLine != '\0'))) {
    if (*lpCmdLine == '\"')
      InString = !InString;
    lpCmdLine++;
  }
  return lpCmdLine;
}

/* Returns an argv vector and takes argc as a pointer parameter              */
/* Must free memory argv on exit                                             */

char **winDrvCmdLineMakeArgv(char *lpCmdLine, int *argc) {
  int elements = 0, i;
  char *tmp;
  char **argv;
  char *argstart, *argend;
  
  tmp = winDrvCmdLineGetNextFirst(lpCmdLine);
  if (tmp != 0) {
    while ((tmp = winDrvCmdLineGetNextFirst(tmp)) != NULL) {
      tmp = winDrvCmdLineGetNextEnd(tmp);
      elements++;
    }
  }
  argv = (char **) malloc(4*(elements + 2));
  argv[0] = "winfellow.exe";
  argend = lpCmdLine;
  for (i = 1; i <= elements; i++) {
    argstart = winDrvCmdLineGetNextFirst(argend);
    argend = winDrvCmdLineGetNextEnd(argstart);
    if (*argstart == '\"')
      argstart++;
    if (*(argend - 1) == '\"')
      argend--;
    *argend++ = '\0';
    argv[i] = argstart;
  }
  argv[elements + 1] = NULL;
  *argc = elements + 1;
  return argv;
}

int WINAPI WinMain(HINSTANCE hInstance,	    // handle to current instance 
		   HINSTANCE hPrevInstance, // handle to previous instance 
		   LPSTR lpCmdLine,	    // pointer to command line 
		   int nCmdShow) {	    // show state of window 
  char *cmdline = (char *) malloc(strlen(lpCmdLine) + 1);
  char **argv;
  int argc;
  int result;
  
  win_drv_nCmdShow = nCmdShow;
  win_drv_hInstance = hInstance;
  strcpy(cmdline, lpCmdLine);
  argv = winDrvCmdLineMakeArgv(cmdline, &argc);
  winDrvSetRegistryKeys(argv);
  result = main(argc, argv);
  free(cmdline);
  free(argv);
  return result;
}

