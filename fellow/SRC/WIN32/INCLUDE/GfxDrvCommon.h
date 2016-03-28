#ifndef GfxDrvCommon_H
#define GfxDrvCommon_H

#include <windows.h>

#include "DEFS.H"
#include "DRAW.H"
#include "Ini.h"

class GfxDrvCommon
{
private:
  HANDLE _run_event;      /* Event indicating running or paused status */
  HWND _hwnd;             /* The emulation output window */
  volatile bool _syskey_down;
  volatile bool _win_active;
  volatile bool _win_active_original;
  volatile bool _win_minimized_original;
  draw_mode *_current_draw_mode;
  ini* _ini;
  unsigned int _output_width;
  unsigned int _output_height;
  bool _output_windowed;

public:
  bool _displaychange;

#ifdef RETRO_PLATFORM
  cfg *rp_startup_config;
#endif

  unsigned int GetOutputWidth();
  unsigned int GetOutputHeight();
  bool GetOutputWindowed();
  void SizeChanged(unsigned int width, unsigned int height);

  bool RunEventInitialize();
  void RunEventRelease();
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
  void SetDrawMode(draw_mode* dm, bool windowed);
  draw_mode *GetDrawMode();

  bool EmulationStart();
  void EmulationStartPost();
  void EmulationStop();
  bool Startup();
  void Shutdown();

  GfxDrvCommon();
  virtual ~GfxDrvCommon();
};

extern GfxDrvCommon* gfxDrvCommon;

#endif
