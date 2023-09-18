#pragma once

/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* This file contains  specific functionality to register as RetroPlatform */
/* guest and interact with the host.                                       */
/*                                                                         */
/* Author: Torsten Enderling                                               */
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

#ifdef RETRO_PLATFORM

// the conditional define below enables/disables support for the API changes introduced in 7.1
#define FELLOW_SUPPORT_RP_API_VERSION_71
// #define FELLOW_DELAY_RP_KEYBOARD_INPUT

// DirectInput resources are used in interfaces
#include "dxver.h"
#include <dinput.h>

#include "gfxdrv.h"
#include "RetroPlatformGuestIPC.h"
#include "RetroPlatformIPC.h"
#include "defs.h"

#define RETRO_PLATFORM_HARDDRIVE_BLINK_MSECS 100
#define RETRO_PLATFORM_KEYSET_COUNT            6 ///< north, east, south, west, fire, autofire
#define RETRO_PLATFORM_MAX_PAL_LORES_WIDTH   376
#define RETRO_PLATFORM_MAX_PAL_LORES_HEIGHT  288
#define RETRO_PLATFORM_NUM_GAMEPORTS           2 ///< gameport 1 & 2
#define RETRO_PLATFORM_OFFSET_ADJUST_LEFT    368
#define RETRO_PLATFORM_OFFSET_ADJUST_TOP      52

class RetroPlatform 
{
public:
  void       EmulationStart();
  void       EmulationStop();
  void       EnterHeadlessMode();

  // getters
  uint32_t        GetClippingOffsetLeftAdjusted();
  uint32_t        GetClippingOffsetTopAdjusted();
  uint32_t        GetDisplayScale();
  bool       GetEmulationPaused();
  uint32_t        GetEscapeKey();
  ULONGLONG  GetEscapeKeyHeldSince();
  uint32_t        GetEscapeKeyHoldTime();
  ULONGLONG  GetEscapeKeySimulatedTargetTime();
  bool       GetHeadlessMode();
  HWND       GetParentWindowHandle();
  HWND       GetTopWindowHandle();
  bool       GetScanlines();
  
  uint32_t        GetScreenHeightAdjusted();
  uint32_t        GetScreenWidthAdjusted();

  uint32_t        GetSourceBufferWidth();
  uint32_t        GetSourceBufferHeight();

  ULONGLONG  GetTime();

  // IPC guest to host communication - asynchronous post
  bool       PostEscaped();
  bool       PostFloppyDriveLED(const uint32_t, const bool, const bool);
  bool       PostFloppyDriveSeek(const uint32_t, const uint32_t);
  bool       PostGameportActivity(const uint32_t, const uint32_t);
  bool       PostHardDriveLED(const uint32_t, const bool, const bool);
  
  // IPC guest to host communication - synchronous send
  bool       SendActivated(const bool, const LPARAM);
  bool       SendClose();
  bool       SendEnable(const bool);
  bool       SendFloppyDriveContent(const uint32_t, const char *, const bool);
  bool       SendHardDriveContent(const uint32_t, const char *, const bool);
  bool       SendFloppyDriveReadOnly(const uint32_t, const bool);
  bool       SendFloppyTurbo(const bool);
  bool       SendMouseCapture(const bool);

  // setters
  void       SetClippingOffsetLeft(const uint32_t);
  void       SetClippingOffsetTop(const uint32_t);
  void       SetEscapeKey(const uint32_t);
  ULONGLONG  SetEscapeKeyHeld(const bool);
  void       SetEscapeKeyHoldTime(const uint32_t);
  void       SetEscapeKeySimulatedTargetTime(const ULONGLONG);
  void       SetHeadlessMode(const bool);
  void       SetHostID(const char *);
  void       SetScreenHeight(uint32_t);
  void       SetScreenMode(const char *);
  void       SetScreenWidth(uint32_t);
  void       SetWindowInstance(HINSTANCE);

  void       RegisterRetroPlatformScreenMode(const bool bStartup);

  // public callback hooks
  LRESULT CALLBACK HostMessageFunction(UINT, WPARAM, LPARAM, LPCVOID, DWORD, LPARAM);
  BOOL FAR PASCAL  EnumerateJoystick(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef);

  void       Shutdown();
  void       Startup();

             RetroPlatform();
  virtual   ~RetroPlatform();
  
private:
  // private functions
  bool       CheckEmulationNecessities();
  bool       ConnectInputDeviceToPort(const uint32_t, const uint32_t, DWORD, const char *);
  void       DetermineScreenModeFromConfig(struct RPScreenMode *, cfg *);
  int        EnumerateJoysticks();

  uint32_t        GetClippingOffsetLeft();
  uint32_t        GetClippingOffsetTop();
  uint32_t        GetCPUSpeed();
  bool       GetHostVersion(uint32_t *, uint32_t *, uint32_t *);
  const char *GetMessageText(uint32_t);
  uint32_t        GetScreenHeight();
  uint32_t        GetScreenWidth();
  bool       GetScreenWindowed();

  // IPC guest to host communication - asynchronous post
  bool       PostMessageToHost(uint32_t, WPARAM, LPARAM, const RPGUESTINFO *);
  bool       PostPowerLEDIntensityPercent(const WPARAM);

  // IPC guest to host communication - synchronous send
  bool       SendMessageToHost(uint32_t, WPARAM, LPARAM, LPCVOID, DWORD, const RPGUESTINFO *, LRESULT *);

  // setters
  void       SetCustomKeyboardLayout(const uint32_t, const char *);
  void       SetDisplayScale(const uint32_t);
  void       SetEmulationPaused(const bool);
  void       SetEmulationState(const bool);
  void       SetEmulatorQuit(const bool);
  void       SetScanlines(const bool);
  void       SetScreenModeStruct(struct RPScreenMode *);
  void       SetScreenWindowed(const bool);

  // IPC guest to host communication - send (sync.)
  bool       SendEnabledFloppyDrives();
  bool       SendEnabledHardDrives();
  bool       SendFeatures();
  bool       SendGameports(const uint32_t);
  bool       SendInputDevice(const DWORD, const DWORD, const DWORD, const WCHAR *, const WCHAR *);
  bool       SendInputDevices();
  bool       SendScreenMode(HWND);

  // private variables
  char szHostID[CFG_FILENAME_LENGTH];  /// host ID that was passed over by the RetroPlatform player
  bool bRetroPlatformMode = false;    /// flag to indicate that emulator operates in RetroPlatform/"headless" mode

    // emulation/state settings
  bool bInitialized = false;
  bool bEmulationState = false;
  bool bEmulationPaused = false;
  bool bEmulatorQuit = false;
  bool bMouseCaptureRequestedByHost = false;

  uint32_t lMainVersion = 0, lRevision = 0, lBuild = 0;

  // screen settings
  int32_t lClippingOffsetLeftRP = RETRO_PLATFORM_OFFSET_ADJUST_LEFT;
  int32_t lClippingOffsetTopRP  = RETRO_PLATFORM_OFFSET_ADJUST_TOP;
  int32_t lScreenWidthRP        = 0, lScreenHeightRP      = 0;
  uint32_t lScreenMode = 0;
  bool bScreenWindowed = true;
  uint32_t lDisplayScale = 1;
  bool bScanlines = false;

  // escape key
  uint32_t lEscapeKey = 1;
  uint32_t lEscapeKeyHoldTime = 600;
  ULONGLONG lEscapeKeyHeldSince = 0;
  ULONGLONG lEscapeKeySimulatedTargetTime = 0;

  int iNumberOfJoysticksAttached = 0;

  RPGUESTINFO GuestInfo;

  HINSTANCE hWindowInstance = nullptr;
  HWND      hGuestWindow = nullptr;

  cfg *pConfig; ///< RetroPlatform copy of WinFellow configuration
};

extern RetroPlatform RP;

#endif
