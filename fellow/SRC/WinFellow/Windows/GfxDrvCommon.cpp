#include "Defs.h"
#include "FELLOW.H"
#include "WINDRV.H"
#include "Ini.h"
#include "GfxDrvCommon.h"
#include "gui_general.h"
#include "GFXDRV.H"

#include "MOUSEDRV.H"
#include "JOYDRV.H"
#include "KBDDRV.H"
#include "TIMER.H"

#include "VirtualHost/Core.h"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#include "Configuration.h"
#endif

#include "windowsx.h"

void GfxDrvCommonDelayFlipTimerCallback(uint32_t timeMilliseconds)
{
  gfxDrvCommon->DelayFlipTimerCallback(timeMilliseconds);
}

void GfxDrvCommon::DelayFlipTimerCallback(uint32_t timeMilliseconds)
{
  _time = timeMilliseconds;

  if (_wait_for_time > 0)
  {
    if (--_wait_for_time == 0)
    {
      SetEvent(_delay_flip_event);
    }
  }
}

void GfxDrvCommon::InitializeDelayFlipEvent()
{
  _delay_flip_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void GfxDrvCommon::ReleaseDelayFlipEvent()
{
  if (_delay_flip_event != nullptr)
  {
    CloseHandle(_delay_flip_event);
    _delay_flip_event = nullptr;
  }
}

void GfxDrvCommon::InitializeDelayFlipTimerCallback()
{
  _previous_flip_time = 0;
  _time = 0;
  _wait_for_time = 0;
  SetEvent(_delay_flip_event);
  timerAddCallback(GfxDrvCommonDelayFlipTimerCallback);
}

int GfxDrvCommon::GetTimeSinceLastFlip()
{
  return static_cast<int>(_time - _previous_flip_time);
}

void GfxDrvCommon::RememberFlipTime()
{
  _previous_flip_time = _time;
}

void GfxDrvCommon::DelayFlipWait(int milliseconds)
{
  ResetEvent(_delay_flip_event);
  _wait_for_time = milliseconds;
  WaitForSingleObject(_delay_flip_event, INFINITE);
}

void GfxDrvCommon::MaybeDelayFlip()
{
  int elapsed_time = GetTimeSinceLastFlip();

  if (elapsed_time < _frametime_target)
  {
    DelayFlipWait(_frametime_target - elapsed_time);
  }
  RememberFlipTime();
}

unsigned int GfxDrvCommon::GetOutputWidth()
{
  return _output_width;
}

unsigned int GfxDrvCommon::GetOutputHeight()
{
  return _output_height;
}

bool GfxDrvCommon::GetOutputWindowed()
{
  return _output_windowed;
}

void GfxDrvCommon::SizeChanged(unsigned int width, unsigned int height)
{
  _output_width = (width > 0) ? width : 1;
  _output_height = (height > 0) ? height : 1;
}

bool GfxDrvCommon::InitializeRunEvent()
{
  _run_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  return (_run_event != nullptr);
}

void GfxDrvCommon::ReleaseRunEvent()
{
  if (_run_event != nullptr)
  {
    CloseHandle(_run_event);
    _run_event = nullptr;
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
  _win_active = (_win_active_original && !_win_minimized_original && !_syskey_down);

#ifdef RETRO_PLATFORM
  if (!RP.GetHeadlessMode())
  {
#endif
    if (_win_active)
    {
      RunEventSet();
    }
    else
    {
      if (_pause_emulation_when_window_loses_focus) RunEventReset();
    }

    gfxDrvNotifyActiveStatus(_win_active);
#ifdef RETRO_PLATFORM
  }
#endif
}

void GfxDrvCommon::NotifyDirectInputDevicesAboutActiveState(bool active)
{
  mouseDrvStateHasChanged(active);

#ifdef RETRO_PLATFORM
#ifdef FELLOW_SUPPORT_RP_API_VERSION_71
  if (!RP.GetHeadlessMode())
  {
#endif
#endif
    joyDrvStateHasChanged(active);
    kbdDrvStateHasChanged(active);
#ifdef RETRO_PLATFORM
#ifdef FELLOW_SUPPORT_RP_API_VERSION_71
  }
#endif
#endif
}

#define WM_DIACQUIRE (WM_USER + 0)

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

#ifdef GFXDRV_DEBUG_WINDOW_MESSAGES
  switch (message)
  {
    case WM_SETFONT:
    case WM_ACTIVATE: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_ACTIVATE"); break;
    case WM_ACTIVATEAPP: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_ACTIVATEAPP"); break;
    case WM_CHAR: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_CHAR"); break;
    case WM_CREATE: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_CREATE"); break;
    case WM_CLOSE: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_CLOSE"); break;
    case WM_DESTROY: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_DESTROY"); break;
    case WM_ENABLE: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_ENABLE"); break;
    case WM_ERASEBKGND: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_ERASEBKGND"); break;
    case WM_GETICON: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_GETICON"); break;
    case WM_GETMINMAXINFO: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_GETMINMAXINFO"); break;
    case WM_IME_NOTIFY: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_IME_NOTIFY"); break;
    case WM_IME_SETCONTEXT: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_IME_SETCONTEXT"); break;
    case WM_KEYDOWN: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_KEYDOWN"); break;
    case WM_KEYUP: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_KEYUP"); break;
    case WM_KILLFOCUS: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_KILLFOCUS"); break;
    case WM_LBUTTONDBLCLK: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_LBUTTONDBLCLK"); break;
    case WM_LBUTTONDOWN: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_LBUTTONDOWN"); break;
    case WM_LBUTTONUP: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_LBUTTONUP"); break;
    case WM_MOVE: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_MOVE"); break;
    case WM_MOUSEACTIVATE: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_MOUSEACTIVATE"); break;
    case WM_MOUSEMOVE: /*_core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_MOUSEMOVE");*/ break;
    case WM_NCPAINT: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_NCPAINT"); break;
    case WM_NCACTIVATE: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_NCACTIVATE"); break;
    case WM_NCCALCSIZE: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_NCCALCSIZE"); break;
    case WM_NCCREATE: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_NCCREATE"); break;
    case WM_NCDESTROY: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_NCDESTROY"); break;
    case WM_PAINT: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_PAINT"); break;
    case WM_SETCURSOR: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_SETCURSOR"); break;
    case WM_SETFOCUS: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_SETFOCUS"); break;
    case WM_SIZE: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_SIZE"); break;
    case WM_SHOWWINDOW: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_SHOWWINDOW"); break;
    case WM_SYNCPAINT: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_SYNCPAINT"); break;
    case WM_SYSCOMMAND: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_SYSCOMMAND"); break;
    case WM_SYSKEYDOWN: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_SYSKEYDOWN"); break;
    case WM_SYSKEYUP: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_SYSKEYUP"); break;
    case WM_TIMER: /*_core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_TIMER");*/ break;
    case WM_WINDOWPOSCHANGING: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_WINDOWPOSCHANGING"); break;
    case WM_WINDOWPOSCHANGED: _core.Log->AddLog("EmulationWindowProc got message %s\n", "WM_WINDOWPOSCHANGED"); break;
    default: _core.Log->AddLog("EmulationWindowProc got message %Xh\n", message);
  }
#endif

  switch (message)
  {
    case WM_ERASEBKGND:
    case WM_NCPAINT:
    case WM_PAINT: graph_buffer_lost = TRUE; break;
    case WM_TIMER:
      if (wParam == 1)
      {
        winDrvHandleInputDevices();
        _core.Drivers.SoundDriver->PollBufferPosition();
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
        case VK_RETURN: /* Full screen vs windowed */ break;
      }
      break;
    case WM_MOVE:
#ifdef _DEBUG
      _core.Log->AddLog("WM_MOVE message with coordinates x: %u; y: %u\n", LOWORD(lParam), HIWORD(lParam));
#endif
      gfxDrvPositionChanged();
      return 0;
    case WM_SIZE: gfxDrvSizeChanged(LOWORD(lParam), HIWORD(lParam)); break;
    case WM_ACTIVATE:
      /* WM_ACTIVATE tells us whether our window is active or not */
      /* It is monitored so that we can know whether we should claim */
      /* the DirectInput devices */

      _win_active_original = (((LOWORD(wParam)) == WA_ACTIVE) || ((LOWORD(wParam)) == WA_CLICKACTIVE));
      _win_minimized_original = ((HIWORD(wParam)) != 0);
      NotifyDirectInputDevicesAboutActiveState(_win_active_original);
#ifdef RETRO_PLATFORM
      if (RP.GetHeadlessMode() && _win_active_original) RP.SendMouseCapture(true);
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
        case SC_SCREENSAVE: return 0;
        case SC_KEYMENU: return 0;
#ifdef RETRO_PLATFORM
        case SC_CLOSE:
          if (RP.GetHeadlessMode())
          {
            RP.SendClose();
            return 0;
          }
          // else fall through to DefWindowProc() to get WM_CLOSE etc.
#endif
        default: return DefWindowProc(hWnd, message, wParam, lParam);
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
      if (RP.GetHeadlessMode()) RP.SendActivated(wParam ? true : false, lParam);
#endif

      return 0;
      break;
    case WM_DESTROY:
      // save emulation window position only if in windowed mode
      if (GetOutputWindowed())
      {
        GetWindowRect(hWnd, &emulationRect);
        iniSetEmulationWindowPosition(_ini, emulationRect.left, emulationRect.top);
      }
      NotifyDirectInputDevicesAboutActiveState(false);
      return 0;
      break;
    case WM_SHOWWINDOW: break;
    case WM_DISPLAYCHANGE:
      if (GetOutputWindowed())
      {
        _displaychange = (wParam != _current_draw_mode->bits);
        fellow_request_emulation_stop = TRUE;
      }
      break;
    case WM_DIACQUIRE: /* Re-evaluate the active status of DI-devices */
      NotifyDirectInputDevicesAboutActiveState(_win_active_original);
      return 0;
      break;
    case WM_CLOSE: fellowRequestEmulationStop(); return 0; /* We handled this message */

#ifdef RETRO_PLATFORM
    case WM_LBUTTONUP:
      if (RP.GetHeadlessMode())
      {
        if (mouseDrvGetFocus())
        {
          NotifyDirectInputDevicesAboutActiveState(_win_active_original);
          RP.SendMouseCapture(true);
        }
        else
        {
          mouseDrvStateHasChanged(TRUE);
          mouseDrvSetFocus(TRUE, FALSE);
        }
        return 0;
      }
    case WM_ENABLE:
      if (RP.GetHeadlessMode())
      {
        RP.SendEnable(wParam ? 1 : 0);
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
  WNDCLASSEX wc1 = {};

  wc1.cbSize = sizeof(wc1);
  wc1.style = CS_HREDRAW | CS_VREDRAW;
  wc1.lpfnWndProc = EmulationWindowProc;
  wc1.cbClsExtra = 0;
  wc1.cbWndExtra = 0;
  wc1.hInstance = win_drv_hInstance;
#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) RP.SetWindowInstance(win_drv_hInstance);
#endif
  wc1.hIcon = LoadIcon(win_drv_hInstance, MAKEINTRESOURCE(IDI_ICON_WINFELLOW));
  wc1.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc1.lpszClassName = "FellowWindowClass";
  wc1.lpszMenuName = "Fellow";
  wc1.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
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
  _core.Log->AddLog("GfxDrvCommon::DisplayWindow()\n");
  if (!GetOutputWindowed())
  {
    // Later: Make dx11 run the normal size code. DX11 startup is always windowed.
    ShowWindow(_hwnd, SW_SHOWNORMAL);
    UpdateWindow(_hwnd);
  }
  else
  {
    uint32_t x = iniGetEmulationWindowXPos(_ini);
    uint32_t y = iniGetEmulationWindowYPos(_ini);
    RECT rc1;
    SetRect(&rc1, x, y, x + _current_draw_mode->width, y + _current_draw_mode->height);
    AdjustWindowRectEx(&rc1, GetWindowStyle(_hwnd), GetMenu(_hwnd) != nullptr, GetWindowExStyle(_hwnd));
    MoveWindow(_hwnd, x, y, rc1.right - rc1.left, rc1.bottom - rc1.top, FALSE);
    ShowWindow(_hwnd, SW_SHOWNORMAL);
    UpdateWindow(_hwnd);

    gfxDrvSizeChanged(_current_draw_mode->width, _current_draw_mode->height);
  }
}

/*==========================================================================*/
/* Hide window hosting the amiga display                                    */
/* Called on emulation stop                                                 */
/*==========================================================================*/

void GfxDrvCommon::HideWindow()
{
  if (!GetOutputWindowed())
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

  SizeChanged(_current_draw_mode->width, _current_draw_mode->height);
  uint32_t width = _current_draw_mode->width;
  uint32_t height = _current_draw_mode->height;

  if (GetOutputWindowed())
  {
    DWORD dwStyle = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
    if (drawGetDisplayScale() == DISPLAYSCALE_AUTO)
    {
      dwStyle |= WS_SIZEBOX | WS_MAXIMIZEBOX;
    }
    DWORD dwExStyle = 0;
    HWND hParent = nullptr;

#ifdef RETRO_PLATFORM
    if (RP.GetHeadlessMode())
    {
      dwStyle = WS_POPUP;
      dwExStyle = WS_EX_TOOLWINDOW;
      hParent = RP.GetParentWindowHandle();
    }
#endif

    _hwnd = CreateWindowEx(
        dwExStyle,
        "FellowWindowClass",
        versionstring,
        dwStyle,
        0, // CW_USEDEFAULT,
        0, // SW_SHOW,
        width,
        height,
        hParent,
        nullptr,
        win_drv_hInstance,
        nullptr);
  }
  else
  {
    _hwnd = CreateWindowEx(WS_EX_TOPMOST, "FellowWindowClass", versionstring, WS_POPUP, 0, 0, width, height, nullptr, nullptr, win_drv_hInstance, nullptr);
  }
  _core.Log->AddLog("GfxDrvCommon::InitializeWindow(): Window created\n");
  free(versionstring);

  return (_hwnd != nullptr);
}

/*==========================================================================*/
/* Destroy window hosting the amiga display                                 */
/* Called on emulation stop                                                 */
/*==========================================================================*/

void GfxDrvCommon::ReleaseWindow()
{
  if (_hwnd != nullptr)
  {
    DestroyWindow(_hwnd);
    _hwnd = nullptr;
  }
}

draw_mode *GfxDrvCommon::GetDrawMode()
{
  return _current_draw_mode;
}

void GfxDrvCommon::SetDrawMode(draw_mode *mode, bool windowed)
{
  _current_draw_mode = mode;
  _output_windowed = windowed;
}

void GfxDrvCommon::SetPauseEmulationWhenWindowLosesFocus(bool pause)
{
  _pause_emulation_when_window_loses_focus = pause;
}

void GfxDrvCommon::Flip()
{
  if (_core.Sound->GetEmulation() == CustomChipset::SOUND_PLAY)
  {
    MaybeDelayFlip();
  }
}

bool GfxDrvCommon::EmulationStart()
{
  RunEventReset(); /* At this point, app is paused */

  _win_active = false;
  _win_active_original = false;
  _win_minimized_original = false;
  _syskey_down = false;
  _displaychange = false;

  if (!InitializeWindow())
  {
    _core.Log->AddLog("GfxDrvCommon::EmulationStart(): Failed to create window\n");
    return false;
  }

  InitializeDelayFlipTimerCallback();

#ifdef RETRO_PLATFORM
  // unpause emulation if in Retroplatform mode
  if (RP.GetHeadlessMode() && !RP.GetEmulationPaused()) RunEventSet();
#endif

  return true;
}

void GfxDrvCommon::EmulationStartPost()
{
  if (_hwnd != nullptr)
  {
#ifdef RETRO_PLATFORM
    if (!RP.GetHeadlessMode())
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
  bool initialized = InitializeRunEvent();
  if (initialized)
  {
    initialized = InitializeWindowClass();
  }

  InitializeDelayFlipEvent();

  return initialized;
}

void GfxDrvCommon::Shutdown()
{
  ReleaseWindow();
  ReleaseWindowClass();
  ReleaseRunEvent();
  ReleaseDelayFlipEvent();
}

GfxDrvCommon::GfxDrvCommon()
  : _run_event(nullptr), _hwnd(nullptr), _ini(nullptr), _frametime_target(18), _previous_flip_time(0), _time(0), _wait_for_time(0), _delay_flip_event(nullptr)
{
}

GfxDrvCommon::~GfxDrvCommon()
{
  Shutdown();
}
