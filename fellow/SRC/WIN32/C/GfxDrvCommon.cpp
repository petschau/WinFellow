#include "PORTABLE.H"
#include "DEFS.H"
#include "FELLOW.H"
#include "WINDRV.H"
#include "Ini.h"
#include "GfxDrvCommon.h"
#include "gui_general.h"
#include "GFXDRV.H"

#include "SOUND.H"
#include "SOUNDDRV.H"
#include "MOUSEDRV.H"
#include "JOYDRV.H"
#include "KBDDRV.H"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#include "config.h"
#endif

#include "windowsx.h"

bool GfxDrvCommon::RunEventInitialize()
{
  _run_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  return (_run_event != NULL);
}

void GfxDrvCommon::RunEventRelease()
{
  if (_run_event != NULL)
  {
    CloseHandle(_run_event);
    _run_event = NULL;
  }
}

void GfxDrvCommon::RunEventSet()
{
  SetEvent(_run_event);
}

void GfxDrvCommon::RunEventReset()
{
  ResetEvent(_run_event);
}

void GfxDrvCommon::RunEventWait()
{
  WaitForSingleObject(_run_event, INFINITE);
}

void GfxDrvCommon::EvaluateRunEventStatus()
{
  _win_active = (_win_active_original &&
    !_win_minimized_original &&
    !_syskey_down);

#ifdef RETRO_PLATFORM
  if (!RetroPlatformGetMode()) {
#endif
    if (_win_active)
    {
      RunEventSet();
    }
    else
    {
      RunEventReset();
    }
#ifdef RETRO_PLATFORM
  }
#endif
}

void GfxDrvCommon::NotifyDirectInputDevicesAboutActiveState(bool active)
{
  mouseDrvStateHasChanged(active);
  joyDrvStateHasChanged(active);
  kbdDrvStateHasChanged(active);
}

#define WM_DIACQUIRE  (WM_USER + 0)

LRESULT FAR PASCAL EmulationWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  return gfxDrvCommon->EmulationWindowProcedure(hWnd, message, wParam, lParam);
}

/***********************************************************************/
/**
* Window procedure for the emulation window.
*
* Distributes events to mouse and keyboard drivers as well.
***************************************************************************/

LRESULT GfxDrvCommon::EmulationWindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  RECT emulationRect;

#ifdef RETRO_PLATFORM
  static BOOLE bIgnoreLeftMouseButton = FALSE;
#endif

#ifdef GFXDRV_DEBUG_WINDOW_MESSAGES
  switch (message)
  {
  case WM_SETFONT:
  case WM_ACTIVATE:           fellowAddLog("EmulationWindowProc got message %s\n", "WM_ACTIVATE");          break;
  case WM_ACTIVATEAPP:        fellowAddLog("EmulationWindowProc got message %s\n", "WM_ACTIVATEAPP");       break;
  case WM_CHAR:               fellowAddLog("EmulationWindowProc got message %s\n", "WM_CHAR");              break;
  case WM_CREATE:             fellowAddLog("EmulationWindowProc got message %s\n", "WM_CREATE");            break;
  case WM_CLOSE:              fellowAddLog("EmulationWindowProc got message %s\n", "WM_CLOSE");             break;
  case WM_DESTROY:            fellowAddLog("EmulationWindowProc got message %s\n", "WM_DESTROY");           break;
  case WM_ENABLE:             fellowAddLog("EmulationWindowProc got message %s\n", "WM_ENABLE");            break;
  case WM_ERASEBKGND:         fellowAddLog("EmulationWindowProc got message %s\n", "WM_ERASEBKGND");        break;
  case WM_GETICON:            fellowAddLog("EmulationWindowProc got message %s\n", "WM_GETICON");           break;
  case WM_GETMINMAXINFO:      fellowAddLog("EmulationWindowProc got message %s\n", "WM_GETMINMAXINFO");     break;
  case WM_IME_NOTIFY:         fellowAddLog("EmulationWindowProc got message %s\n", "WM_IME_NOTIFY");        break;
  case WM_IME_SETCONTEXT:     fellowAddLog("EmulationWindowProc got message %s\n", "WM_IME_SETCONTEXT");    break;
  case WM_KEYDOWN:            fellowAddLog("EmulationWindowProc got message %s\n", "WM_KEYDOWN");           break;
  case WM_KEYUP:              fellowAddLog("EmulationWindowProc got message %s\n", "WM_KEYUP");             break;
  case WM_KILLFOCUS:          fellowAddLog("EmulationWindowProc got message %s\n", "WM_KILLFOCUS");         break;
  case WM_LBUTTONDBLCLK:      fellowAddLog("EmulationWindowProc got message %s\n", "WM_LBUTTONDBLCLK");     break;
  case WM_LBUTTONDOWN:        fellowAddLog("EmulationWindowProc got message %s\n", "WM_LBUTTONDOWN");       break;
  case WM_LBUTTONUP:          fellowAddLog("EmulationWindowProc got message %s\n", "WM_LBUTTONUP");         break;
  case WM_MOVE:               fellowAddLog("EmulationWindowProc got message %s\n", "WM_MOVE");              break;
  case WM_MOUSEACTIVATE:      fellowAddLog("EmulationWindowProc got message %s\n", "WM_MOUSEACTIVATE");     break;
  case WM_MOUSEMOVE:        /*fellowAddLog("EmulationWindowProc got message %s\n", "WM_MOUSEMOVE");*/       break;
  case WM_NCPAINT:            fellowAddLog("EmulationWindowProc got message %s\n", "WM_NCPAINT");           break;
  case WM_NCACTIVATE:         fellowAddLog("EmulationWindowProc got message %s\n", "WM_NCACTIVATE");        break;
  case WM_NCCALCSIZE:         fellowAddLog("EmulationWindowProc got message %s\n", "WM_NCCALCSIZE");        break;
  case WM_NCCREATE:           fellowAddLog("EmulationWindowProc got message %s\n", "WM_NCCREATE");          break;
  case WM_NCDESTROY:          fellowAddLog("EmulationWindowProc got message %s\n", "WM_NCDESTROY");         break;
  case WM_PAINT:              fellowAddLog("EmulationWindowProc got message %s\n", "WM_PAINT");             break;
  case WM_SETCURSOR:          fellowAddLog("EmulationWindowProc got message %s\n", "WM_SETCURSOR");         break;
  case WM_SETFOCUS:           fellowAddLog("EmulationWindowProc got message %s\n", "WM_SETFOCUS");          break;
  case WM_SIZE:               fellowAddLog("EmulationWindowProc got message %s\n", "WM_SIZE");              break;
  case WM_SHOWWINDOW:         fellowAddLog("EmulationWindowProc got message %s\n", "WM_SHOWWINDOW");        break;
  case WM_SYNCPAINT:          fellowAddLog("EmulationWindowProc got message %s\n", "WM_SYNCPAINT");         break;
  case WM_SYSCOMMAND:         fellowAddLog("EmulationWindowProc got message %s\n", "WM_SYSCOMMAND");        break;
  case WM_SYSKEYDOWN:         fellowAddLog("EmulationWindowProc got message %s\n", "WM_SYSKEYDOWN");        break;
  case WM_SYSKEYUP:           fellowAddLog("EmulationWindowProc got message %s\n", "WM_SYSKEYUP");          break;
  case WM_TIMER:            /*fellowAddLog("EmulationWindowProc got message %s\n", "WM_TIMER");*/           break;
  case WM_WINDOWPOSCHANGING:  fellowAddLog("EmulationWindowProc got message %s\n", "WM_WINDOWPOSCHANGING"); break;
  case WM_WINDOWPOSCHANGED:   fellowAddLog("EmulationWindowProc got message %s\n", "WM_WINDOWPOSCHANGED");  break;
  default:                    fellowAddLog("EmulationWindowProc got message %Xh\n", message);
  }
#endif

  switch (message)
  {
  case WM_ERASEBKGND:
  case WM_NCPAINT:
  case WM_PAINT:
    graph_buffer_lost = TRUE;
    break;
  case WM_TIMER:
    if (wParam == 1)
    {
      winDrvHandleInputDevices();
      soundDrvPollBufferPosition();
      return 0;
    }
    break;
  case WM_SYSKEYDOWN:
  {
    int vkey = (int)wParam;
    _syskey_down = (vkey != VK_F10);
  }
  break;
  case WM_SYSKEYUP:
    _syskey_down = false;
    EvaluateRunEventStatus();
    switch (wParam)
    {
    case VK_RETURN:  /* Full screen vs windowed */
      break;
    }
    break;
  case WM_MOVE:
  case WM_SIZE:
    if (_current_draw_mode->windowed)
    {
      gfxDrvSizeChanged();
    }
    break;
  case WM_ACTIVATE:
    /* WM_ACTIVATE tells us whether our window is active or not */
    /* It is monitored so that we can know whether we should claim */
    /* the DirectInput devices */

    _win_active_original = (((LOWORD(wParam)) == WA_ACTIVE) ||
      ((LOWORD(wParam)) == WA_CLICKACTIVE));
    _win_minimized_original = ((HIWORD(wParam)) != 0);
    NotifyDirectInputDevicesAboutActiveState(_win_active_original);
#ifdef RETRO_PLATFORM
    if (RetroPlatformGetMode())
      RetroPlatformSendMouseCapture(TRUE);
#endif
    EvaluateRunEventStatus();
    return 0; /* We processed this message */
  case WM_ENTERMENULOOP:
  case WM_ENTERSIZEMOVE:
    _win_active_original = false;
    NotifyDirectInputDevicesAboutActiveState(_win_active_original);
    return 0;
  case WM_EXITMENULOOP:
  case WM_EXITSIZEMOVE:
    _win_active_original = (GetActiveWindow() == hWnd && !IsIconic(hWnd));
    PostMessage(hWnd, WM_DIACQUIRE, 0, 0L);
    return 0;
  case WM_SYSCOMMAND:
    if (IsWindow(hWnd))
    {
      NotifyDirectInputDevicesAboutActiveState(_win_active_original);
    }
    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
    case SC_SCREENSAVE:
      return 0;
    case SC_KEYMENU:
      return 0;
#ifdef RETRO_PLATFORM
    case SC_CLOSE:
      if (RetroPlatformGetMode())
        RetroPlatformSendClose();
      return 0;
#endif
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
  case WM_ACTIVATEAPP:
    if (wParam)
    {
      /* Being activated */
    }
    else
    {
      /* Being de-activated */
      _syskey_down = false;
    }

#ifdef RETRO_PLATFORM
    if (RetroPlatformGetMode())
      RetroPlatformSendActivate(wParam, lParam);
#endif

    return 0;
    break;
  case WM_DESTROY:
    // save emulation window position only if in windowed mode
    if (_current_draw_mode->windowed)
    {
      GetWindowRect(hWnd, &emulationRect);
      iniSetEmulationWindowPosition(_ini, emulationRect.left, emulationRect.top);
    }
    NotifyDirectInputDevicesAboutActiveState(false);
    return 0;
    break;
  case WM_SHOWWINDOW:
    break;
  case WM_DISPLAYCHANGE:
    if (_current_draw_mode->windowed)
    {
      _displaychange = (wParam != _current_draw_mode->bits) && (_current_draw_mode->windowed == TRUE);
      fellow_request_emulation_stop = TRUE;
    }
    break;
  case WM_DIACQUIRE:        /* Re-evaluate the active status of DI-devices */
    NotifyDirectInputDevicesAboutActiveState(_win_active_original);
    return 0;
    break;
  case WM_CLOSE:
    fellowRequestEmulationStop();
    return 0; /* We handled this message */

#ifdef RETRO_PLATFORM
  case WM_LBUTTONUP:
    if (RetroPlatformGetMode())
    {
      if (mouseDrvGetFocus())
      {
        NotifyDirectInputDevicesAboutActiveState(_win_active_original);
        RetroPlatformSendMouseCapture(TRUE);
      }
      else
      {
        mouseDrvSetFocus(TRUE, FALSE);
      }
      return 0;
    }
  case WM_ENABLE:
    if (RetroPlatformGetMode())
    {
      RetroPlatformSendEnable(wParam ? 1 : 0);
      return 0;
    }
#endif

  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}


/*==========================================================================*/
/* Create window classes for Amiga display                                  */
/* Called on emulator startup                                               */
/*==========================================================================*/

bool GfxDrvCommon::InitializeWindowClass()
{
  WNDCLASSEX wc1;

  memset(&wc1, 0, sizeof(wc1));
  wc1.cbSize = sizeof(wc1);
  wc1.style = CS_HREDRAW | CS_VREDRAW;
  wc1.lpfnWndProc = EmulationWindowProc;
  wc1.cbClsExtra = 0;
  wc1.cbWndExtra = 0;
  wc1.hInstance = win_drv_hInstance;
#ifdef RETRO_PLATFORM
  if (RetroPlatformGetMode())
    RetroPlatformSetWindowInstance(win_drv_hInstance);
#endif
  wc1.hIcon = LoadIcon(win_drv_hInstance, MAKEINTRESOURCE(IDI_ICON_WINFELLOW));
  wc1.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc1.lpszClassName = "FellowWindowClass";
  wc1.lpszMenuName = "Fellow";
  return (RegisterClassEx(&wc1) != 0);
}

void GfxDrvCommon::ReleaseWindowClass()
{
  UnregisterClass("FellowWindowClass", win_drv_hInstance);
}

/***********************************************************************/
/**
* Show window hosting the amiga display.
*
* Called on every emulation startup. In RetroPlatform mode, the player will
* take care of showing the emulator's window.
***************************************************************************/

void GfxDrvCommon::DisplayWindow()
{
  fellowAddLog("gfxdrv: gfxDrvWindowShow()\n");
  if (!_current_draw_mode->windowed)
  {
    ShowWindow(_hwnd, SW_SHOWMAXIMIZED);
    UpdateWindow(_hwnd);
  }
  else
  {
    ULO x = iniGetEmulationWindowXPos(_ini);
    ULO y = iniGetEmulationWindowYPos(_ini);
    RECT rc1;
    SetRect(&rc1, x, y, x + _current_draw_mode->width, y + _current_draw_mode->height);
    AdjustWindowRectEx(&rc1, GetWindowStyle(_hwnd), GetMenu(_hwnd) != NULL, GetWindowExStyle(_hwnd));
    MoveWindow(_hwnd, x, y, rc1.right - rc1.left, rc1.bottom - rc1.top, FALSE);
    ShowWindow(_hwnd, SW_SHOWNORMAL);
    UpdateWindow(_hwnd);

    gfxDrvSizeChanged();
  }
}


/*==========================================================================*/
/* Hide window hosting the amiga display                                    */
/* Called on emulation stop                                                 */
/*==========================================================================*/

void GfxDrvCommon::HideWindow()
{
  if (!_current_draw_mode->windowed)
  {
    ShowWindow(_hwnd, SW_SHOWMINIMIZED);
  }
}


HWND GfxDrvCommon::GetHWND()
{
  return _hwnd;
}

/*==========================================================================*/
/* Create a window to host the Amiga display                                */
/* Called on emulation start                                                */
/* Returns TRUE if succeeded                                                */
/*==========================================================================*/

bool GfxDrvCommon::InitializeWindow()
{
  char *versionstring = fellowGetVersionString();

  if (_current_draw_mode->windowed)
  {
    DWORD dwStyle = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
    DWORD dwExStyle = 0;
    HWND hParent = NULL;
    ULO width = _current_draw_mode->width;
    ULO height = _current_draw_mode->height;

#ifdef RETRO_PLATFORM
    if (RetroPlatformGetMode())
    {
      dwStyle = WS_POPUP;
      dwExStyle = WS_EX_TOOLWINDOW;
      hParent = RetroPlatformGetParentWindowHandle();

      width = cfgGetScreenWidth(rp_startup_config);
      height = cfgGetScreenHeight(rp_startup_config);

      fellowAddLog("RetroPlatform: override window dimensions to %ux%u, offset %u,%u...\n",
        width, height,
        RetroPlatformGetClippingOffsetLeftAdjusted(),
        RetroPlatformGetClippingOffsetTopAdjusted());
    }
#endif

    _hwnd = CreateWindowEx(dwExStyle,
      "FellowWindowClass",
      versionstring,
      dwStyle,
      0, // CW_USEDEFAULT,
      0, // SW_SHOW,
      width,
      height,
      hParent,
      NULL,
      win_drv_hInstance,
      NULL);
  }
  else
  {
    _hwnd = CreateWindowEx(WS_EX_TOPMOST,
      "FellowWindowClass",
      versionstring,
      WS_POPUP,
      0,
      0,
      GetSystemMetrics(SM_CXSCREEN),
      GetSystemMetrics(SM_CYSCREEN),
      NULL,
      NULL,
      win_drv_hInstance,
      NULL);
  }
  fellowAddLog("gfxdrv: Window created\n");
  free(versionstring);

  return (_hwnd != NULL);
}


/*==========================================================================*/
/* Destroy window hosting the amiga display                                 */
/* Called on emulation stop                                                 */
/*==========================================================================*/

void GfxDrvCommon::ReleaseWindow()
{
  if (_hwnd != NULL)
  {
    DestroyWindow(_hwnd);
    _hwnd = NULL;
  }
}

draw_mode* GfxDrvCommon::GetDrawMode()
{
  return _current_draw_mode;
}

void GfxDrvCommon::SetDrawMode(draw_mode* mode)
{
  _current_draw_mode = mode;
}

bool GfxDrvCommon::EmulationStart()
{
  RunEventReset();                    /* At this point, app is paused */

  _win_active = false;
  _win_active_original = false;
  _win_minimized_original = false;
  _syskey_down = false;
  _displaychange = false;

  if (!InitializeWindow())
  {
    fellowAddLog("gfxdrv: GfxDrvCommon::EmulationStart(): Failed to create window\n");
    return false;
  }

#ifdef RETRO_PLATFORM
  // unpause emulation if in Retroplatform mode
  if (RetroPlatformGetMode() && !RetroPlatformGetEmulationPaused())
    RunEventSet();
#endif

  return true;
}

void GfxDrvCommon::EmulationStartPost()
{
  if (_hwnd != NULL)
  {
#ifdef RETRO_PLATFORM
    if (!RetroPlatformGetMode())
#endif
      DisplayWindow();
  }
}

void GfxDrvCommon::EmulationStop()
{
  ReleaseWindow();
}

bool GfxDrvCommon::Startup()
{
  _ini = iniManagerGetCurrentInitdata(&ini_manager);
  bool initialized = RunEventInitialize();
  if (initialized)
  {
    initialized = InitializeWindowClass();
  }

  return initialized;
}

void GfxDrvCommon::Shutdown()
{
  ReleaseWindow();
  ReleaseWindowClass();
  RunEventRelease();
}

GfxDrvCommon::GfxDrvCommon()
  : _run_event(0), _hwnd(0), _ini(0)
{
}

GfxDrvCommon::~GfxDrvCommon()
{
  Shutdown();
}

