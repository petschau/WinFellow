/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* This file contains  specific functionality to register as RetroPlatform */
/* guest and interact with the host.                                       */
/*                                                                         */
/* Author: Torsten Enderling (carfesh@gmx.net)                             */
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

#ifndef RETROPLATFORM_H
#define RETROPLATFORM_H

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
  void       EmulationStart(void);
  void       EmulationStop(void);
  void       EnterHeadlessMode(void);

  // getters
  ULO        GetClippingOffsetLeftAdjusted(void);
  ULO        GetClippingOffsetTopAdjusted(void);
  ULO        GetDisplayScale(void);
  bool       GetEmulationPaused(void);
  ULO        GetEscapeKey(void);
  ULONGLONG  GetEscapeKeyHeldSince(void);
  ULO        GetEscapeKeyHoldTime(void);
  ULONGLONG  GetEscapeKeySimulatedTargetTime(void);
  bool       GetHeadlessMode(void);
  HWND       GetParentWindowHandle(void);
  bool       GetScanlines(void);
  
  ULO        GetScreenHeightAdjusted(void);
  ULO        GetScreenWidthAdjusted(void);

  ULO        GetSourceBufferWidth(void);
  ULO        GetSourceBufferHeight(void);

  ULONGLONG  GetTime(void);

  // IPC guest to host communication - asynchronous post
  bool       PostEscaped(void);
  bool       PostFloppyDriveLED(const ULO, const bool, const bool);
  bool       PostFloppyDriveSeek(const ULO, const ULO);
  bool       PostGameportActivity(const ULO, const ULO);
  bool       PostHardDriveLED(const ULO, const bool, const bool);
  
  // IPC guest to host communication - synchronous send
  bool       SendActivated(const bool, const LPARAM);
  bool       SendClose(void);
  bool       SendEnable(const bool);
  bool       SendFloppyDriveContent(const ULO, const STR *, const bool);
  bool       SendHardDriveContent(const ULO, const STR *, const bool);
  bool       SendFloppyDriveReadOnly(const ULO, const bool);
  bool       SendFloppyTurbo(const bool);
  bool       SendMouseCapture(const bool);

  // setters
  void       SetClippingOffsetLeft(const ULO);
  void       SetClippingOffsetTop(const ULO);
  void       SetEscapeKey(const ULO);
  ULONGLONG  SetEscapeKeyHeld(const bool);
  void       SetEscapeKeyHoldTime(const ULO);
  void       SetEscapeKeySimulatedTargetTime(const ULONGLONG);
  void       SetHeadlessMode(const bool);
  void       SetHostID(const char *);
  void       SetScreenHeight(ULO);
  void       SetScreenMode(const char *);
  void       SetScreenWidth(ULO);
  void       SetWindowInstance(HINSTANCE);

  void       RegisterRetroPlatformScreenMode(const bool bStartup);

  // public callback hooks
  LRESULT CALLBACK HostMessageFunction(UINT, WPARAM, LPARAM, LPCVOID, DWORD, LPARAM);
  BOOL FAR PASCAL  EnumerateJoystick(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef);

  void       Shutdown(void);
  void       Startup(void);

             RetroPlatform();
  virtual   ~RetroPlatform();
  
private:
  // private functions
  bool       CheckEmulationNecessities(void);
  bool       ConnectInputDeviceToPort(const ULO, const ULO, DWORD, const STR *);
  void       DetermineScreenModeFromConfig(struct RPScreenMode *, cfg *);
  int        EnumerateJoysticks(void);

  ULO        GetClippingOffsetLeft(void);
  ULO        GetClippingOffsetTop(void);
  ULO        GetCPUSpeed(void);
  bool       GetHostVersion(ULO *, ULO *, ULO *);
  const STR *GetMessageText(ULO);
  ULO        GetScreenHeight(void);
  ULO        GetScreenWidth(void);
  bool       GetScreenWindowed(void);

  // IPC guest to host communication - asynchronous post
  bool       PostMessageToHost(ULO, WPARAM, LPARAM, const RPGUESTINFO *);
  bool       PostPowerLEDIntensityPercent(const WPARAM);

  // IPC guest to host communication - synchronous send
  bool       SendMessageToHost(ULO, WPARAM, LPARAM, LPCVOID, DWORD, const RPGUESTINFO *, LRESULT *);

  // setters
  void       SetCustomKeyboardLayout(const ULO, const STR *);
  void       SetDisplayScale(const ULO);
  void       SetEmulationPaused(const bool);
  void       SetEmulationState(const bool);
  void       SetEmulatorQuit(const bool);
  void       SetScanlines(const bool);
  void       SetScreenModeStruct(struct RPScreenMode *);
  void       SetScreenWindowed(const bool);

  // IPC guest to host communication - send (sync.)
  bool       SendEnabledFloppyDrives(void);
  bool       SendEnabledHardDrives(void);
  bool       SendFeatures(void);
  bool       SendGameports(const ULO);
  bool       SendInputDevice(const DWORD, const DWORD, const DWORD, const WCHAR *, const WCHAR *);
  bool       SendInputDevices(void);
  bool       SendScreenMode(HWND);

  // private variables
  STR szHostID[CFG_FILENAME_LENGTH];  /// host ID that was passed over by the RetroPlatform player
  bool bRetroPlatformMode = false;    /// flag to indicate that emulator operates in RetroPlatform/"headless" mode

    // emulation/state settings
  bool bInitialized = false;
  bool bEmulationState = false;
  bool bEmulationPaused = false;
  bool bEmulatorQuit = false;
  bool bMouseCaptureRequestedByHost = false;

  ULO lMainVersion = 0, lRevision = 0, lBuild = 0;

  // screen settings
  LON lClippingOffsetLeftRP = 0, lClippingOffsetTopRP = 0;
  LON lScreenWidthRP        = 0, lScreenHeightRP      = 0;
  ULO lScreenMode = 0;
  bool bScreenWindowed = true;
  ULO lDisplayScale = 1;
  bool bScanlines = false;

  // escape key
  ULO lEscapeKey = 1;
  ULO lEscapeKeyHoldTime = 600;
  ULONGLONG lEscapeKeyHeldSince = 0;
  ULONGLONG lEscapeKeySimulatedTargetTime = 0;

  int iNumberOfJoysticksAttached = 0;

  RPGUESTINFO GuestInfo;

  HINSTANCE hWindowInstance = NULL;
  HWND      hGuestWindow = NULL;

  cfg *pConfig; ///< RetroPlatform copy of WinFellow configuration
};

extern RetroPlatform RP;

#endif

#endif