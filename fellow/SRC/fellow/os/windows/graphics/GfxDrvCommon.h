#pragma once

#include <windows.h>

#include "fellow/configuration/Configuration.h"
#include "fellow/application/HostRenderConfiguration.h"
#include "fellow/application/DisplayMode.h"
#include "fellow/api/Drivers.h"

class GfxDrvCommon
{
private:
  HANDLE _run_event{}; // Event indicating running or paused status
  HWND _hwnd{};        // The emulation output window
  volatile bool _syskey_down{};
  volatile bool _win_active{};
  volatile bool _win_active_original{};
  volatile bool _win_minimized_original{};
  bool _pause_emulation_when_window_loses_focus{};
  DisplayMode _current_draw_mode;
  IniValues *_iniValues{};

  DimensionsUInt _hostBufferSize;
  int _frametime_target{};
  int _previous_flip_time{};
  volatile int _time{};
  volatile int _wait_for_time{};
  HANDLE _delay_flip_event{};

  bool _displayChange{};
  bool _isInitialized{};

  const char *_fellowWindowClassName = "FellowWindowClass";
  const char *_fellowMenuName = "Fellow";

  static LRESULT CALLBACK EmulationWindowProcedureStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

  void MaybeDelayFlip();
  void DelayFlipWait(int milliseconds);
  void RememberFlipTime();
  int GetTimeSinceLastFlip();
  void InitializeDelayFlipTimerCallback();

  bool InitializeRunEvent();
  void ReleaseRunEvent();

  bool InitializeDelayFlipEvent();
  void ReleaseDelayFlipEvent();
  
  void SetDrawMode(const DisplayMode &dm);

public:
  LRESULT EmulationWindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

  const DimensionsUInt &GetHostBufferSize() const;

  bool GetDisplayChange() const;
  bool IsHostBufferWindowed() const;
  void SizeChanged(unsigned int width, unsigned int height);

  void DelayFlipTimerCallback(ULO timeMilliseconds);

  void RunEventSet();
  void RunEventReset();
  void RunEventWait();
  void EvaluateRunEventStatus();
  void NotifyDirectInputDevicesAboutActiveState(bool active);

  bool InitializeWindowClass();
  void ReleaseWindowClass();
  void DisplayWindow();
  void HideWindow();
  bool InitializeWindow(DisplayScale displayScale);
  void ReleaseWindow();

  HWND GetHWND();
  const DisplayMode &GetDrawMode() const;

  void SetPauseEmulationWhenWindowLosesFocus(bool pause);
  bool GetPauseEmulationWhenWindowLosesFocus();

  void Flip();
  bool EmulationStart(const HostRenderConfiguration &hostRenderConfiguration, const DisplayMode &displayMode);
  void EmulationStartPost();
  void EmulationStop();

  bool IsInitialized() const;
  void Initialize();
  void Release();
};
