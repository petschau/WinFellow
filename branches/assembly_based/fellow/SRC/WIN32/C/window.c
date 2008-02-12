/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Emulation window creation                                                 */
/* Author: Petter Schau (peschau@online.no)                                  */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

#include "portable.h"

#include <windowsx.h>
#include "defs.h"
#include "fellow.h"
#include "window.h"
#include "ini.h"
#include "gui_general.h"
#include "listtree.h"

/* Remove later */
#include "draw.h"
#include "gfxdrv.h"

ini *window_ini; /* window copy of initdata */

/*==========================================================================*/
/* Window hosting the Amiga display and some generic status information     */
/*==========================================================================*/

#define WINDOW_LISTENERS_MAX 16
#define WINDOW_TITLE_MAX 512

typedef struct
{
  HWND hwnd;		                            /* Handle to the window */
  HINSTANCE hinstance;		                       /* The app hinstance */
  HANDLE active_event;       /* Event that is set when the window is active */
  RECT rect_screen;	   /* The current client rect in screen coordinates */
  BOOLE on_desktop;    /* This is either a full-screen window, or a desktop */
	                                              /* cooperative window */
  BOOLE syskey_down;           /* Says whether the syskey is pressed or not */
  ULO width;	                            /* Size and depth of the window */
  ULO height;
  ULO depth;
  ini *ini;
  ULO create_listener_count;                      /* Listener callback data */
  ULO destroy_listener_count;
  ULO active_listener_count;
  ULO palette_listener_count;
  ULO close_listener_count;
  ULO displaychange_listener_count;
  window_create_listener_func create_listener_func[WINDOW_LISTENERS_MAX];
  window_destroy_listener_func destroy_listener_func[WINDOW_LISTENERS_MAX];
  window_active_listener_func active_listener_func[WINDOW_LISTENERS_MAX];
  window_palette_listener_func palette_listener_func[WINDOW_LISTENERS_MAX];
  window_close_listener_func close_listener_func[WINDOW_LISTENERS_MAX];
  window_displaychange_listener_func displaychange_listener_func[WINDOW_LISTENERS_MAX];
  PALETTEENTRY palette[256];    /* The current palette if in 256-color mode */
  STR classname[32];                             /* Autogenerated classname */
  STR title[WINDOW_TITLE_MAX];                          /* The window title */
} wininfo;

/*==========================================================================*/
/* Manages the list of windows, ineffective for large numbers of windows    */
/* but we don't do that.                                                    */
/*==========================================================================*/

felist *windowlist = NULL;

wininfo *windowListFindHwnd(HWND hwnd)
{
  felist *tmp;
  for (tmp = windowlist; tmp != NULL; tmp = listNext(tmp))
  {
    wininfo *wi = (wininfo *) listNode(tmp);
    if (wi->hwnd == hwnd) return wi;
  }
  return NULL;
}

void windowListAddWinInfo(wininfo *wi)
{
  if (windowlist == NULL) windowlist = listNew(wi);
  else listAddFirst(windowlist, listNew(wi));
}

void windowListRemoveWinInfo(wininfo *wi)
{
  felist *tmp;
  for (tmp = windowlist; tmp != NULL; tmp = listNext(tmp))
  {
    wininfo *tmp_wi = (wininfo *) listNode(tmp);
    if (tmp_wi == wi)
    {
      if (windowlist == tmp) windowlist = listNext(tmp);
      listFree(tmp);
      return;
    }
  }
}

/*==========================================================================*/
/* Initialize a window active event                                         */
/*==========================================================================*/

static BOOLE windowWIActiveEventInitialize(wininfo *wi)
{
  wi->active_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  return (wi->active_event != NULL);
}

static void windowWIActiveEventRelease(wininfo *wi)
{
  if (wi->active_event != NULL)
  {
    CloseHandle(wi->active_event);
    wi->active_event = NULL;
  }
}

void windowWIActiveEventSet(wininfo *wi)
{
  if (wi->active_event != NULL) SetEvent(wi->active_event);
}

void windowWIActiveEventReset(wininfo *wi)
{
  if (wi->active_event != NULL) ResetEvent(wi->active_event);
}

void windowWIActiveEventWait(wininfo *wi)
{
  if (wi->active_event != NULL)
    WaitForSingleObject(wi->active_event, INFINITE);
}

/*==========================================================================*/
/* Notify modules that wants to know the window status                      */
/*==========================================================================*/

static void windowWINotifyCreateListeners(wininfo *wi)
{
  ULO i;
  for (i = 0; i < wi->create_listener_count; ++i)
    wi->create_listener_func[i](wi->hwnd);
}

static void windowWINotifyDestroyListeners(wininfo *wi)
{
  ULO i;
  for (i = 0; i < wi->destroy_listener_count; ++i)
    wi->destroy_listener_func[i](wi->hwnd);
}

static void windowWINotifyActiveListeners(wininfo *wi, BOOLE active, BOOLE iconic, BOOLE syskey)
{
  ULO i;
  for (i = 0; i < wi->active_listener_count; ++i)
    wi->active_listener_func[i](wi->hwnd, active, iconic, syskey);
}

static void windowWINotifyPaletteListeners(wininfo *wi)
{
  ULO i;
  for (i = 0; i < wi->palette_listener_count; ++i)
    wi->palette_listener_func[i](wi->hwnd);
}

static void windowWINotifyCloseListeners(wininfo *wi)
{
  ULO i;
  for (i = 0; i < wi->close_listener_count; ++i)
    wi->close_listener_func[i](wi->hwnd);
}

static void windowWINotifyDisplayChangeListeners(wininfo *wi)
{
  ULO i;
  for (i = 0; i < wi->displaychange_listener_count; ++i)
    wi->displaychange_listener_func[i](wi->hwnd);
}

/*==========================================================================*/
/* Evaluate current status for various events                               */
/*==========================================================================*/

static void windowWIEvaluateCreateStatus(wininfo *wi)
{
  windowWINotifyCreateListeners(wi);
}

static void windowWIEvaluateDestroyStatus(wininfo *wi)
{
  windowWINotifyDestroyListeners(wi);
}

static void windowWIEvaluateActiveStatus(wininfo *wi)
{
  windowWINotifyActiveListeners(wi, (GetActiveWindow() == wi->hwnd), IsIconic(wi->hwnd), wi->syskey_down);
}

static void windowWIEvaluatePaletteStatus(wininfo *wi)
{
  windowWINotifyPaletteListeners(wi);
}

static void windowWIEvaluateCloseStatus(wininfo *wi)
{
  windowWINotifyCloseListeners(wi);
}

static void windowWIEvaluateDisplayChangeStatus(wininfo *wi)
{
  windowWINotifyDisplayChangeListeners(wi);
}

/*==========================================================================*/
/* Get information about the current client rectangle                       */
/*==========================================================================*/

static void windowWIFindClientRectScreen(wininfo *wi)
{
  RECT rect;
  GetClientRect(wi->hwnd, &rect);
  memcpy(&wi->rect_screen, &rect, sizeof(RECT));
  ClientToScreen(wi->hwnd, (LPPOINT) &wi->rect_screen);
  ClientToScreen(wi->hwnd, (LPPOINT) &wi->rect_screen + 1);
}

/*==========================================================================*/
/* Window procedure for the emulation window                                */
/* Distributes events to mouse and keyboard drivers as well                 */
/*==========================================================================*/

long FAR PASCAL windowWIProc(HWND hWnd,
			     UINT message, 
                             WPARAM wParam,
			     LPARAM lParam)
{
  RECT emulation_rect;
  wininfo *wi = NULL;

  if (message == WM_CREATE)
  {
    CREATESTRUCT *cs = (CREATESTRUCT *) lParam;
    wi = (wininfo *) cs->lpCreateParams;
  }
  else
    wi = windowListFindHwnd(hWnd);

  if (wi == NULL)
    return DefWindowProc(hWnd, message, wParam, lParam);

  switch (message) {
    /* WM_CREATE is sent inside the CreateWindow() function by win32 */
    case WM_CREATE:
      wi->hwnd = hWnd;
      windowWIEvaluateCreateStatus(wi);
      return 0; /* Continues creation of window */
    /* Alt+"another key" is pressed, or F10 (Menu) */
    /* DefWindowProc() sends WM_SYSCOMMAND if "another key" is tab or enter */
    /* Investigate more, what does this mean for us? */
    case WM_SYSKEYDOWN:
      wi->syskey_down = TRUE;
      windowWIEvaluateActiveStatus(wi);
      break;
    /* A key was released while alt was held down, or alt iself was released */
    /* Investigate more, what does this mean for us? */
    case WM_SYSKEYUP:
      wi->syskey_down = FALSE;
      windowWIEvaluateActiveStatus(wi);
      break;
    /* The window has been moved. */
    /* On desktop, remember our new position, DDraw then blits to new location */
    case WM_MOVE:
      if (wi->on_desktop) windowWIFindClientRectScreen(wi);
      break;
    /* We are being activated, or deactivated, return 0 when handled. */
    /* We blindly forward this to listeners */
    case WM_ACTIVATE:
      windowWIEvaluateActiveStatus(wi);
      return 0;
    /* A menu modal loop is entered, we don't use menu, but check it anyway */
    /* since we need to release acquired DI devices if this happens. */
    case WM_ENTERMENULOOP:
      windowWIEvaluateActiveStatus(wi);
      break;
    /* The window has entered a modal size/move loop, pass to DefWndProc() */
    /* which completes the loop, and reevaluate our active status first. */
    case WM_ENTERSIZEMOVE:
      windowWIEvaluateActiveStatus(wi);
      break;
    /* We have exited a modal loop in the menu or a move/size loop */
    /* We are probably active again now, so reevaluate our status */
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
      windowWIEvaluateActiveStatus(wi);
      return 0;
    /* Generated in response to a WM_SYSKEY message, some bad stuff can happen, such as
       the screensaver, in general we reevaluate our status and let DefWindowProc() do the job */
    case WM_SYSCOMMAND:
      windowWIEvaluateActiveStatus(wi);
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      {
        case SC_SCREENSAVE:
	  return 0; /* Veto screen-saver */
        default:
          return DefWindowProc(hWnd, message, wParam, lParam);
      }
      break;
    /* Sent to the two activating and deactivating windows when a "different" app than */
    /* the active one is about to be activated */
    case WM_ACTIVATEAPP:
      wi->syskey_down = FALSE;
      windowWIEvaluateActiveStatus(wi);
      return 0;
    /* Sent to the window after the window has been removed from the screen. */
    case WM_DESTROY:
      /* save emulation window position only if in windowed mode */
      if (wi->on_desktop)
      {
	GetWindowRect(hWnd, &emulation_rect);  
	iniSetEmulationWindowPosition(wi->ini, emulation_rect.left, emulation_rect.top);
      }
      windowWIEvaluateActiveStatus(wi);
      windowWIEvaluateDestroyStatus(wi);
      return 0;
    /* Sent to the window after the size has changed */
    case WM_SIZE:
      break;
    /* The window is about to be shown or hidden */
    case WM_SHOWWINDOW:
      break; 
    /* Sent when the display resolution has changed */
    case WM_DISPLAYCHANGE:
      windowWIEvaluateDisplayChangeStatus(wi);
      break;
    /* The window with the keyboard focus has realized its palette */
    /* If we can't have them all, we don't care */
    case WM_PALETTECHANGED:	/* Sometimes we did it */
      break;
    /* We are about to get the keyboard focus and is asked to realize our palette */
    case WM_QUERYNEWPALETTE:
      windowWIEvaluatePaletteStatus(wi);
      return TRUE; /* We must return TRUE! */
    /* This is a signal that the app or window should terminate */
    case WM_CLOSE:
      windowWIEvaluateCloseStatus(wi);
      fellowRequestEmulationStop();
      return 0; /* We handled this message */ 
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

/*==========================================================================*/
/* Create window classes for Amiga display                                  */
/* Called on emulator startup                                               */
/*==========================================================================*/

BOOLE windowWIRegisterClass(wininfo *wi)
{
  WNDCLASS wc1;
  
  sprintf(wi->classname, "Fellow%.8X", wi);
  memset(&wc1, 0, sizeof(wc1));
  wc1.style = CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNCLIENT;
  wc1.lpfnWndProc = windowWIProc;
  wc1.cbClsExtra = 0;
  wc1.cbWndExtra = 0;
  wc1.hInstance = wi->hinstance;
  wc1.hIcon = LoadIcon(wi->hinstance, MAKEINTRESOURCE(IDI_ICON_WINFELLOW));
  wc1.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc1.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
  wc1.lpszClassName = wi->classname;
  wc1.lpszMenuName = NULL;
  return (RegisterClass(&wc1) != 0);
}

void windowWIUnregisterClass(wininfo *wi)
{
  UnregisterClass(wi->classname, wi->hinstance);
}

/*==========================================================================*/
/* Show window hosting the amiga display                                    */
/* Called on every emulation startup                                        */
/*==========================================================================*/

void windowWIShow(wininfo *wi)
{
  if (!wi->on_desktop)
  {
    ShowWindow(wi->hwnd, SW_SHOWMAXIMIZED);
    UpdateWindow(wi->hwnd);
  }
  else
  {
    RECT rc1;
    
    SetRect(&rc1,
	    iniGetEmulationWindowXPos(wi->ini),
	    iniGetEmulationWindowYPos(wi->ini), 
	    wi->width + iniGetEmulationWindowXPos(wi->ini), 
            wi->height + iniGetEmulationWindowYPos(wi->ini));
    AdjustWindowRectEx(&rc1,
		       GetWindowStyle(wi->hwnd),
		       GetMenu(wi->hwnd) != NULL,
		       GetWindowExStyle(wi->hwnd));
    MoveWindow(wi->hwnd,
	       iniGetEmulationWindowXPos(wi->ini),
	       iniGetEmulationWindowYPos(wi->ini), 
	       rc1.right - rc1.left,
	       rc1.bottom - rc1.top,
	       FALSE);
    ShowWindow(wi->hwnd, SW_SHOWNORMAL);
    UpdateWindow(wi->hwnd);
    windowWIFindClientRectScreen(wi);
  }
}


/*==========================================================================*/
/* Hide window hosting the amiga display                                    */
/* Called on emulation stop                                                 */
/*==========================================================================*/

void windowWIHide(wininfo *wi)
{
  ShowWindow(wi->hwnd, SW_HIDE);
}

/*==========================================================================*/
/* Create a window to host the Amiga display                                */
/* Called on emulation start                                                */
/* Returns TRUE if succeeded                                                */
/*==========================================================================*/

BOOLE windowWICreate(wininfo *wi)
{
  if (wi->on_desktop)
  {
    wi->hwnd = CreateWindowEx(0,
			      wi->classname,
			      wi->title,
			      WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
			      CW_USEDEFAULT,
			      SW_SHOW,
			      wi->width,
			      wi->height,
			      NULL,
			      NULL,
			      wi->hinstance,
			      wi); /* Need to know the address of our data */
  }
  else
  {
    wi->hwnd = CreateWindowEx(WS_EX_TOPMOST,
			      wi->classname,
			      wi->title,
			      WS_POPUP,
			      0,
			      0,
			      GetSystemMetrics(SM_CXSCREEN),
			      GetSystemMetrics(SM_CYSCREEN),
			      NULL,
			      NULL,
			      wi->hinstance,
			      NULL);
  }
  return (wi->hwnd != NULL);
}

/*==========================================================================*/
/* Destroy window hosting the amiga display                                 */
/* Called on emulation stop                                                 */
/*==========================================================================*/

BOOLE windowWIDestroy(wininfo *wi)
{
  BOOLE res = TRUE;
  if (wi->hwnd != NULL)
  {
    res = DestroyWindow(wi->hwnd);
    wi->hwnd = NULL;
  }
  return res;
}

/*==========================================================================*/
/* Initialize and release a window info struct                              */
/* Called on emulator startup and shutdown                                  */
/*==========================================================================*/

wininfo *windowWINew(HINSTANCE hinstance, STR *title, ini *inidata)
{
  wininfo *wi = malloc(sizeof(wininfo));
  memset(wi, 0, sizeof(wininfo));
  wi->hinstance = hinstance;
  wi->ini = inidata;
  strncpy(wi->title, title, WINDOW_TITLE_MAX);
  wi->title[WINDOW_TITLE_MAX - 1] = '\0';
  if (windowWIRegisterClass(wi))
  {
    if (windowWIActiveEventInitialize(wi))
    {
      windowListAddWinInfo(wi);
      return wi;
    }
    else windowWIUnregisterClass(wi);
  }
  free(wi);
  return NULL;
}

void windowWIDelete(wininfo *wi)
{
  windowWIActiveEventRelease(wi);
  windowWIUnregisterClass(wi);
  windowListRemoveWinInfo(wi);
}

/*==========================================================================*/
/* Adds a listener for the creation state of a window                       */
/*==========================================================================*/

BOOLE windowWIAddCreateListener(wininfo *wi,
				window_create_listener_func listener_func)
{
  BOOLE res = (wi->create_listener_count < WINDOW_LISTENERS_MAX);
  if (res)
    wi->create_listener_func[wi->create_listener_count++] = listener_func;
  return res;
}

/*==========================================================================*/
/* Adds a listener for the destroy state of a window                        */
/*==========================================================================*/

BOOLE windowWIAddDestroyListener(wininfo *wi,
				 window_destroy_listener_func listener_func)
{
  BOOLE res = (wi->destroy_listener_count < WINDOW_LISTENERS_MAX);
  if (res)
    wi->destroy_listener_func[wi->destroy_listener_count++] = listener_func;
  return res;
}

/*==========================================================================*/
/* Adds a listener for the active state of a window                         */
/*==========================================================================*/

BOOLE windowWIAddActiveListener(wininfo *wi,
				window_active_listener_func listener_func)
{
  BOOLE res = (wi->active_listener_count < WINDOW_LISTENERS_MAX);
  if (res)
    wi->active_listener_func[wi->active_listener_count++] = listener_func;
  return res;
}

/*==========================================================================*/
/* Adds a palette listener for the window                                   */
/*==========================================================================*/

BOOLE windowWIAddPaletteListener(wininfo *wi,
			         window_palette_listener_func listener_func)
{
  BOOLE res = (wi->palette_listener_count < WINDOW_LISTENERS_MAX);
  if (res)
    wi->palette_listener_func[wi->palette_listener_count++] = listener_func;
  return res;
}

/*==========================================================================*/
/* Adds a close listener for the window                                     */
/*==========================================================================*/

BOOLE windowWIAddCloseListener(wininfo *wi,
			       window_close_listener_func listener_func)
{
  BOOLE res = (wi->close_listener_count < WINDOW_LISTENERS_MAX);
  if (res)
    wi->close_listener_func[wi->close_listener_count++] = listener_func;
  return res;
}

/*==========================================================================*/
/* Adds a move listener for the window                                      */
/*==========================================================================*/

BOOLE windowWIAddDisplayChangeListener(wininfo *wi,
			               window_displaychange_listener_func listener_func)
{
  BOOLE res = (wi->displaychange_listener_count < WINDOW_LISTENERS_MAX);
  if (res)
    wi->displaychange_listener_func[wi->displaychange_listener_count++] = listener_func;
  return res;
}

/*==========================================================================*/
/* Removes all listeners                                                    */
/*==========================================================================*/

void windowWIRemoveListeners(wininfo *wi)
{
  wi->active_listener_count = 0;
  wi->close_listener_count = 0;
  wi->create_listener_count = 0;
  wi->destroy_listener_count = 0;
  wi->palette_listener_count = 0;
  wi->displaychange_listener_count = 0;
}

/*==========================================================================*/
/* We handle this ourselves                                                 */
/*==========================================================================*/

void windowCloseListener(HWND hwnd)
{
  fellowRequestEmulationStop();
}


/* PUBLIC INTERFACE */

wininfo *window_info = NULL;

/*==========================================================================*/
/* Returns the current client rect in screen coordinates                    */
/*==========================================================================*/

RECT *windowGetClientRectScreen(void)
{
  return &(window_info->rect_screen);
}

/*==========================================================================*/
/* Returns the current HWND                                                 */
/*==========================================================================*/

HWND windowHwnd(void)
{
  return window_info->hwnd;
}

/*==========================================================================*/
/* Show the window                                                          */
/*==========================================================================*/

void windowShow(void)
{
  windowWIShow(window_info);
}

/*==========================================================================*/
/* Hide the window                                                          */
/*==========================================================================*/

void windowHide(void)
{
  windowWIHide(window_info);
}

/*==========================================================================*/
/* Adds a listener for the creation state of a window                       */
/*==========================================================================*/

BOOLE windowAddCreateListener(window_create_listener_func listener_func)
{
  return windowWIAddCreateListener(window_info, listener_func);
}

/*==========================================================================*/
/* Adds a listener for the destroy state of a window                        */
/*==========================================================================*/

BOOLE windowAddDestroyListener(window_destroy_listener_func listener_func)
{
  return windowWIAddDestroyListener(window_info, listener_func);
}

/*==========================================================================*/
/* Adds a listener for the active state of a window                         */
/*==========================================================================*/

BOOLE windowAddActiveListener(window_active_listener_func listener_func)
{
  return windowWIAddActiveListener(window_info, listener_func);
}

/*==========================================================================*/
/* Adds a listener for the palette of a window                              */
/*==========================================================================*/

BOOLE windowAddPaletteListener(window_palette_listener_func listener_func)
{
  return windowWIAddPaletteListener(window_info, listener_func);
}

/*==========================================================================*/
/* Adds a listener for the closing of a window                              */
/*==========================================================================*/

BOOLE windowAddCloseListener(window_close_listener_func listener_func)
{
  return windowWIAddCloseListener(window_info, listener_func);
}

/*==========================================================================*/
/* Adds a listener for displaychange                                        */
/*==========================================================================*/

BOOLE windowAddDisplayChangeListener(window_displaychange_listener_func listener_func)
{
  return windowWIAddDisplayChangeListener(window_info, listener_func);
}

/*==========================================================================*/
/* Remove all listeners                                                     */
/*==========================================================================*/

void windowRemoveListeners(void)
{
  windowWIRemoveListeners(window_info);
}

/*==========================================================================*/
/* Waits for the window to become active                                    */
/*==========================================================================*/

void windowActiveWait(void)
{
  windowWIActiveEventWait(window_info);
}

/*==========================================================================*/
/* Initializes the window module for emulation                              */
/* Called on emulation startup                                              */
/*==========================================================================*/

BOOLE windowEmulationStart(BOOLE on_desktop, ULO width, ULO height, ULO depth)
{
  window_info->ini = iniManagerGetCurrentInitdata(&ini_manager);
  window_info->on_desktop = on_desktop;
  window_info->width = width;
  window_info->height = height;
  window_info->depth = depth;
  windowAddCloseListener(windowCloseListener);
  return windowWICreate(window_info);
}

/*==========================================================================*/
/* Releases the window module after emulation                               */
/* Called on emulation stop                                                 */
/*==========================================================================*/

void windowEmulationStop(void)
{
  windowWIDestroy(window_info);
}

/*==========================================================================*/
/* Initializes the window module                                            */
/* Called on emulator startup                                               */
/*==========================================================================*/

void windowStartup(HINSTANCE hinstance, STR *title)
{
  window_info = windowWINew(hinstance,
			    title,
			    iniManagerGetCurrentInitdata(&ini_manager));
}

/*==========================================================================*/
/* Shuts down the window module                                             */
/* Called on emulator startup                                               */
/*==========================================================================*/

void windowShutdown(void)
{
  windowWIDelete(window_info);
  window_info = NULL;
}
