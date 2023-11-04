#pragma once

#include <windows.h>

#include "Defs.h"
#include "Renderer.h"
#include "IniFile.h"

class GfxDrvCommon
{
private:
  HANDLE _run_event; /* Event indicating running or paused status */
  HWND _hwnd;        /* The emulation output window */
  volatile bool _syskey_down;
  volatile bool _win_active;
  volatile bool _win_active_original;
  volatile bool _win_minimized_original;
  bool _pause_emulation_when_window_loses_focus;
  draw_mode *_current_draw_mode;
  ini *_ini;
  unsigned int _output_width;
  unsigned int _output_height;
  bool _output_windowed;

  int _frametime_target;
  int _previous_flip_time;
  volatile int _time;
  volatile int _wait_for_time;
  HANDLE _delay_flip_event;

  void MaybeDelayFlip();
  void DelayFlipWait(int milliseconds);
  void RememberFlipTime();
  int GetTimeSinceLastFlip();
  void InitializeDelayFlipTimerCallback();
  void InitializeDelayFlipEvent();
  void ReleaseDelayFlipEvent();

public:
  bool _displaychange;

#ifdef RETRO_PLATFORM
  cfg *rp_startup_config;
#endif

  unsigned int GetOutputWidth();
  unsigned int GetOutputHeight();
  bool GetOutputWindowed();
  void SizeChanged(unsigned int width, unsigned int height);

  void DelayFlipTimerCallback(uint32_t timeMilliseconds);

  bool InitializeRunEvent();
  void ReleaseRunEvent();
  void RunEventSet();
  void RunEventReset();
  void RunEventWait();
  void EvaluateRunEventStatus();
  void NotifyDirectInputDevicesAboutActiveState(bool active);

  bool InitializeWindowClass();
  void ReleaseWindowClass();
  void DisplayWindow();
  void HideWindow();
  bool InitializeWindow();
  void ReleaseWindow();
  LRESULT EmulationWindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

  HWND GetHWND();
  void SetDrawMode(draw_mode *dm, bool windowed);
  draw_mode *GetDrawMode();

  void SetPauseEmulationWhenWindowLosesFocus(bool pause);

  void Flip();
  bool EmulationStart();
  void EmulationStartPost();
  void EmulationStop();
  bool Startup();
  void Shutdown();

  GfxDrvCommon();
  virtual ~GfxDrvCommon();
};

extern GfxDrvCommon *gfxDrvCommon;
