/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Cloanto RetroPlatform GUI integration                                   */
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

/** @file
 *  Cloanto RetroPlatform GUI integration.
 *
 * This module contains RetroPlatform specific functionality to register as
 * RetroPlatform guest and interact with the host (player).
 * It imitates the full Windows GUI module, implementing the same functionality,
 * but supported via the RetroPlatform player as main GUI.
 * WinFellow's own GUI is not shown, the emulator operates in a headless mode.
 * The configuration is received as a command line parameter, all control events
 * (start, shutdown, reset, ...) are sent via IPC.
 *
 * *Important Note:* The Cloanto modules make heavy use of unicode strings.
 * As WinFellow uses ANSI strings, conversion is usually required (using, for
 * example, wsctombs and mbstowcs).
 *
 * When looking at an RP9 package, the RetroPlatform WinFellow plug-in has a
 * list of criteria to decide if WinFellow is a compatible emulator that is
 * offered as choice. It verifies that
 * - a valid model-specific INI file exists for the configured Amiga model
 * - no extended ADF files are used (file size = 901.120 for all ADFs)
 *
 * The plug-in will block the start of WinFellow on a number of criteria:
 * - if filesystems are used
 * - hardfiles using a non-standard geometry, or RDB hardfiles
 *
 * The RetroPlatform WinFellow plug-in will start WinFellow with the following
 * command-line arguments:
 *
 * -rphost:
 * ID of the RetroPlatform host, used to initiate IPC communication.
 *
 * -datapath:
 * the path where WinFellow specific runtime information should be stored.
 *
 * -f:
 * The -f parameter provides an initial configuration, that is created in the
 * following order:
 * -# the WinFellow plug-in's shared.ini is applied
 * -# a model-specific INI is applied on top of that
 * -# the WinFellow plug-in overrides a number of things:
 *    - if "No interlace" is checked in the compatibility settings,
 * fellow.gfx_deinterlace is set to no, otherwise to yes
 *    - if "Turbo CPU" is checked, cpu_speed is set to 0
 *    - if the user has enabled "Always speed up drives where possible", "Turbo
 * floppy" is set to yes in the RP9, and  "Always use more accurate (slower)
 * emulation" in the Option dialog is NOT set, fellow.floppy_fast_dma is set to
 * yes, otherwise to no
 *    - gfx_width, gfx_height, gfx_offset_left and gfx_offset_top are added into
 * the configuration depending on the settings of the RP9; the numbers assume
 * the maximum pixel density (horizontal values super hires, vertical values
 * interlace), so depending on the mode displayed, conversion is required; the
 * clipping offsets need to be adjusted (384 to the top, 52 to the left)
 * -# the WinFellow plug-in's override.ini is applied on top of that, to apply
 * any settings that always must be active
 *
 * -rpescapekey:
 * the DirectInput keycode of the escape key
 *
 * -rpescapeholdtime:
 * the time in milliseconds after which the escape key should actually escape
 *
 * -rpscreenmode:
 * the initial screenmode the guest should activate at startup (e.g. 1X, 2X,
 * ...). It is the numerical equivalent of the RP_SCREENMODE_* flags (see
 * RetroPlatformIPC.h).
 */

#include "Defs.h"

#include "RetroPlatform.h"
#include "RetroPlatformGuestIPC.h"
#include "RetroPlatformIPC.h"

#include "CpuIntegration.h"
#include "GfxDrvCommon.h"
#include "GfxDrvDXGI.h"
#include "KeyboardDriver.h"
#include "VirtualHost/Core.h"
#include "Configuration.h"
#include "dxver.h" /// needed for DirectInput based joystick detection code
#include "FellowMain.h"
#include "Module/Hardfile/IHardfileHandler.h"
#include "FloppyDisk.h"
#include "GraphicsDriver.h"
#include "DirectDrawDriver.h"
#include "JoystickDriver.h"
#include "KeyboardDriver.h"
#include "MouseDriver.h"
#include "WindowsMain.h"
#include "Keycode.h"

RetroPlatform RP;
bool rawkeyspressed[MAX_KEYS] = {false};

/** event handler function for events that are sent to WinFellow from Amiga Forever
 * handles multiple incoming events like keyboard or joystick input events that are queued within the event message
 *
 * returns TRUE if successful, FALSE otherwise (for instance if an unrecogized event is encountered)
 */

BOOL RetroPlatformHandleIncomingGuestEvent(char *strCurrentEvent)
{
  if (strCurrentEvent == nullptr)
  {
    _core.Log->AddLog("RetroPlatformHandleIncomingGuestEvent(): WARNING: ignoring "
                      "NULL event string.\n");
    return FALSE;
  }

#ifdef _DEBUG
  _core.Log->AddLog(" RetroPlatformHandleIncomingGuestEvent(): handling current event '%s'\n", strCurrentEvent);
#endif

  BOOL blnMatch = FALSE;
  char *strRawKeyCode = nullptr;
  uint32_t lRawKeyCode = 0;

  // handle key_raw_up and key_raw_down events
  if (!strnicmp(strCurrentEvent, "key_raw_down ", 13))
  {
    if (strRawKeyCode = strchr(strCurrentEvent, ' '))
    {
      lRawKeyCode = (uint32_t)strtol(strRawKeyCode, nullptr, 0);
      kbdDrvKeypressRaw(lRawKeyCode, TRUE);
      rawkeyspressed[lRawKeyCode] = true;
    }
    blnMatch = TRUE;
  }

  if (rawkeyspressed[A_CTRL] && rawkeyspressed[A_LEFT_AMIGA] && rawkeyspressed[A_RIGHT_AMIGA])
  {
    _core.Log->AddLog("RetroPlatformHandleIncomingGuestEvent(): performing keyboard-initiated reset.");

    fellowSetPreStartReset(true);
    gfxDrvCommon->RunEventSet();
    fellowRequestEmulationStop();
    mouseDrvToggleFocus();
    memset(rawkeyspressed, 0, sizeof(bool) * MAX_KEYS);
  }

  if (!strnicmp(strCurrentEvent, "key_raw_up ", 11))
  {
    if (strRawKeyCode = strchr(strCurrentEvent, ' '))
    {
      lRawKeyCode = (uint32_t)strtol(strRawKeyCode, nullptr, 0);
      kbdDrvKeypressRaw(lRawKeyCode, FALSE);
      rawkeyspressed[lRawKeyCode] = false;
    }

    blnMatch = TRUE;
  }

  // if no matching event was found, the player should return 0
  if (blnMatch)
    return TRUE;
  else
    return FALSE;
}

BOOL RetroPlatformHandleIncomingGuestEventMessageParser(char *strEventMessage)
{
  char *strCurrentEvent = (char *)strEventMessage;

  for (;;)
  {
    char *strNextEvent = nullptr;
    char *blank1 = strchr(strCurrentEvent, ' ');
    if (!blank1) break;
    char *blank2 = strchr(blank1 + 1, ' ');
    if (blank2)
    {
      *blank2 = NULL;
      strNextEvent = blank2 + 1;
    }

    RetroPlatformHandleIncomingGuestEvent(strCurrentEvent);

    if (!strNextEvent) break;

    strCurrentEvent = strNextEvent;
  }

  free(strEventMessage);
  return TRUE;
}

BOOL RetroPlatformHandleIncomingGuestEventMessage(wchar_t *wcsEventMessage)
{
  char *strEventMessage = nullptr;
  size_t lEventMessageLength = 0, lReturnCode = 0;

  lEventMessageLength = wcstombs(nullptr, wcsEventMessage,
                                 0); // first call to wcstombs() determines how
  // long the output buffer needs to be
  strEventMessage = (char *)malloc(lEventMessageLength + 1);
  if (strEventMessage == nullptr) return FALSE;
  lReturnCode = wcstombs(strEventMessage, wcsEventMessage, lEventMessageLength + 1);
  if (lReturnCode == (size_t)-1)
  {
    _core.Log->AddLog(
        "RetroPlatformHandleIncomingGuestEventMessage(): ERROR converting "
        "incoming guest event message with length %u to multi-byte string, "
        "ignoring message. Return code received was %u.\n",
        lEventMessageLength,
        lReturnCode);
    free(strEventMessage);
    return FALSE;
  }

#ifdef _DEBUG
  _core.Log->AddLog(
      "RetroPlatformHandleIncomingGuestEventMessage(): received an "
      "incoming guest event message with length %u: ",
      lEventMessageLength);
  _core.Log->AddLog2(strEventMessage);
  _core.Log->AddLog2("\n");
#endif

  RetroPlatformHandleIncomingGuestEventMessageParser(strEventMessage);
  return TRUE;
}

BOOL RetroPlatformHandleIncomingDeviceActivity(WPARAM wParam, LPARAM lParam)
{
  uint32_t lGamePort = HIBYTE(wParam);
  uint32_t lDeviceCategory = LOBYTE(wParam);
  uint32_t lMask = lParam;

  _core.Log->AddLog(
      "RetroPlatformHandleIncomingDeviceActivity(): wParam=%04x, "
      "lParam=%08x, lGamePort=%u, lDeviceCategory=%u\n",
      wParam,
      lParam,
      lGamePort,
      lDeviceCategory);

  if (lDeviceCategory != RP_DEVICECATEGORY_INPUTPORT)
  {
    _core.Log->AddLog(" RetroPlatformHandleIncomingDeviceActivity(): unsupported "
                      "device category.n");
    return FALSE;
  }

  if (lGamePort > 1)
  {
    _core.Log->AddLog(" RetroPlatformHandleIncomingDeviceActivity(): invalid gameport %u.\n", lGamePort);
    return FALSE;
  }

  BOOL bRight = lMask & RP_JOYSTICK_RIGHT;
  BOOL bLeft = lMask & RP_JOYSTICK_LEFT;
  BOOL bDown = lMask & RP_JOYSTICK_DOWN;
  BOOL bUp = lMask & RP_JOYSTICK_UP;
  BOOL bButton1 = lMask & RP_JOYSTICK_BUTTON1;
  BOOL bButton2 = lMask & RP_JOYSTICK_BUTTON2;

  if (lGamePort == 0)
    gameportJoystickHandler(RP_JOYSTICK0, bLeft, bUp, bRight, bDown, bButton1, bButton2);
  else if (lGamePort == 1)
    gameportJoystickHandler(RP_JOYSTICK1, bLeft, bUp, bRight, bDown, bButton1, bButton2);

  return TRUE;
}

// hook into RetroPlatform class to perform IPC communication with host
LRESULT CALLBACK RetroPlatformHostMessageFunction(UINT uMessage, WPARAM wParam, LPARAM lParam, LPCVOID pData, DWORD dwDataSize, LPARAM lMsgFunctionParam)
{
  return RP.HostMessageFunction(uMessage, wParam, lParam, pData, dwDataSize, lMsgFunctionParam);
}

// hook into RetroPlatform class to enumerate joysticks
BOOL FAR PASCAL RetroPlatformEnumerateJoystick(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef)
{
  return RP.EnumerateJoystick(pdinst, pvRef);
}

/** host message function that is used as callback to receive IPC messages from
 * the host.
 */
LRESULT CALLBACK RetroPlatform::HostMessageFunction(UINT uMessage, WPARAM wParam, LPARAM lParam, LPCVOID pData, DWORD dwDataSize, LPARAM lMsgFunctionParam)
{
#ifdef _DEBUG
  _core.Log->AddLog(
      "RetroPlatform::HostMessageFunction(%s [%d], %08x, %08x, %08x, "
      "%d, %08x)\n",
      RP.GetMessageText(uMessage),
      uMessage - WM_APP,
      wParam,
      lParam,
      pData,
      dwDataSize,
      lMsgFunctionParam);
#endif

  switch (uMessage)
  {
    default:
      _core.Log->AddLog(
          "RetroPlatform::HostMessageFunction(): Unknown or unsupported "
          "command 0x%x\n",
          uMessage);
      break;
    case RP_IPC_TO_GUEST_EVENT: return RetroPlatformHandleIncomingGuestEventMessage((wchar_t *)pData);
    case RP_IPC_TO_GUEST_PING: return true;
    case RP_IPC_TO_GUEST_CLOSE:
      _core.Log->AddLog("RetroPlatform::HostMessageFunction(): received close event.\n");
      fellowRequestEmulationStop();
      gfxDrvCommon->RunEventSet();
      RP.SetEmulatorQuit(true);
      return true;
    case RP_IPC_TO_GUEST_RESET:
      if (wParam == RP_RESET_HARD) fellowSetPreStartReset(true);
      RP.SetEmulationPaused(false);
      gfxDrvCommon->RunEventSet();
      fellowRequestEmulationStop();
      return true;
    case RP_IPC_TO_GUEST_TURBO:
      if (wParam & RP_TURBO_CPU)
      {
        static uint32_t lOriginalSpeed = 0;

        if (lParam & RP_TURBO_CPU)
        {
          _core.Log->AddLog("RetroPlatform::HostMessageFunction(): enabling CPU turbo "
                            "mode...\n");
          lOriginalSpeed = RP.GetCPUSpeed();
          cpuIntegrationSetSpeed(0);
          cpuIntegrationCalculateMasterCycleMultiplier();
          _core.Cpu->SetCpuInstructionHandlerFromConfig();
          fellowRequestEmulationStop();
        }
        else
        {
          _core.Log->AddLog(
              "RetroPlatform::HostMessageFunction(): disabling CPU "
              "turbo mode, reverting back to speed level %u...\n",
              lOriginalSpeed);
          cpuIntegrationSetSpeed(lOriginalSpeed);
          cpuIntegrationCalculateMasterCycleMultiplier();
          _core.Cpu->SetCpuInstructionHandlerFromConfig();
          fellowRequestEmulationStop();
        }
      }
      if (wParam & RP_TURBO_FLOPPY) floppySetFastDMA(lParam & RP_TURBO_FLOPPY ? true : false);
      return true;
    case RP_IPC_TO_GUEST_PAUSE:
      if (wParam != 0)
      { // pause emulation
        gfxDrvCommon->RunEventReset();
        RP.SetEmulationPaused(true);
        RP.SetEmulationState(false);
        return 1;
      }
      else
      { // resume emulation
        gfxDrvCommon->RunEventSet();
        RP.SetEmulationPaused(false);
        RP.SetEmulationState(true);
        return 1;
      }
    case RP_IPC_TO_GUEST_VOLUME:
      _core.Sound->SetVolume(wParam);
      _core.Drivers.SoundDriver->SetCurrentSoundDeviceVolume(wParam);
      return true;
#ifndef FELLOW_SUPPORT_RP_API_VERSION_71
    case RP_IPC_TO_GUEST_ESCAPEKEY:
      RP.SetEscapeKey(wParam);
      RP.SetEscapeKeyHoldTime(lParam);
      return true;
#endif
    case RP_IPC_TO_GUEST_MOUSECAPTURE:
      _core.Log->AddLog("RetroPlatform::HostMessageFunction(): mousecapture: %d.\n", wParam & RP_MOUSECAPTURE_CAPTURED);
      mouseDrvStateHasChanged(true);
      mouseDrvSetFocus(wParam & RP_MOUSECAPTURE_CAPTURED ? true : false, true);
      return true;
    case RP_IPC_TO_GUEST_DEVICEACTIVITY: return RetroPlatformHandleIncomingDeviceActivity(wParam, lParam);
    case RP_IPC_TO_GUEST_DEVICECONTENT:
    {
      struct RPDeviceContent *dc = (struct RPDeviceContent *)pData;
      char name[CFG_FILENAME_LENGTH] = "";
      wcstombs(name, dc->szContent, CFG_FILENAME_LENGTH);
#ifdef _DEBUG
      _core.Log->AddLog(
          "RetroPlatform::HostMessageFunction(): RP_IPC_TO_GUEST_DEVICECONTENT "
          "Cat=%d Num=%d Flags=%08x '%s'\n",
          dc->btDeviceCategory,
          dc->btDeviceNumber,
          dc->dwFlags,
          name);
#endif
      int num = dc->btDeviceNumber;
      int ok = false;

      switch (dc->btDeviceCategory)
      {
        case RP_DEVICECATEGORY_FLOPPY:
          if (name == nullptr || name[0] == 0)
          {
            _core.Log->AddLog(
                "RetroPlatform::HostMessageFunction(): remove floppy disk "
                "from drive %d.\n",
                num);
            floppyImageRemove(num);
          }
          else
          {
            _core.Log->AddLog(
                "RetroPlatform::HostMessageFunction(): set floppy image "
                "for drive %d to %s.\n",
                num,
                name);
            floppySetDiskImage(num, name);
          }
          ok = true;
          break;
        case RP_DEVICECATEGORY_INPUTPORT: ok = RP.ConnectInputDeviceToPort(num, dc->dwInputDevice, dc->dwFlags, name); break;
        case RP_DEVICECATEGORY_CD: ok = false; break;
      }
      return ok;
    }
    case RP_IPC_TO_GUEST_SCREENCAPTURE:
    {
      struct RPScreenCapture *rpsc = (struct RPScreenCapture *)pData;
      char szScreenFiltered[CFG_FILENAME_LENGTH] = "", szScreenRaw[CFG_FILENAME_LENGTH] = "";

      wcstombs(szScreenFiltered, rpsc->szScreenFiltered, CFG_FILENAME_LENGTH);
      wcstombs(szScreenRaw, rpsc->szScreenRaw, CFG_FILENAME_LENGTH);

      if (szScreenFiltered[0] || szScreenRaw[0])
      {
        bool bResult = true;
        DWORD dResult = 0;

        _core.Log->AddLog(
            "RetroPlatform::HostMessageFunction(): screenshot request "
            "received; filtered '%s', raw '%s'\n",
            szScreenFiltered,
            szScreenRaw);

        if (szScreenFiltered[0])
          if (!gfxDrvSaveScreenshot(true, szScreenFiltered)) bResult = false;

        if (szScreenRaw[0])
          if (!gfxDrvSaveScreenshot(false, szScreenRaw)) bResult = false;

        if (bResult)
        {
          dResult |= RP_GUESTSCREENFLAGS_MODE_PAL | RP_GUESTSCREENFLAGS_HORIZONTAL_HIRES | RP_GUESTSCREENFLAGS_VERTICAL_INTERLACED;
          return dResult;
        }
        else
          return RP_SCREENCAPTURE_ERROR;
      }
      return RP_SCREENCAPTURE_ERROR;
    }
    case RP_IPC_TO_GUEST_SCREENMODE:
    {
      struct RPScreenMode *sm = (struct RPScreenMode *)pData;
      RP.SetScreenModeStruct(sm);
      return (LRESULT)INVALID_HANDLE_VALUE;
    }
    case RP_IPC_TO_GUEST_DEVICEREADWRITE:
    {
      DWORD ret = false;
      int device = LOBYTE(wParam);
      if (device == RP_DEVICECATEGORY_FLOPPY)
      {
        int num = HIBYTE(wParam);
        if (lParam == RP_DEVICE_READONLY || lParam == RP_DEVICE_READWRITE)
        {
          floppySetReadOnlyConfig(num, lParam == RP_DEVICE_READONLY ? true : false);
          ret = true;
        }
      }
      return ret ? (LPARAM)1 : 0;
    }
    case RP_IPC_TO_GUEST_FLUSH: return 1;
    case RP_IPC_TO_GUEST_GUESTAPIVERSION:
    {
      return MAKELONG(3, 4);
    }
  }
  return false;
}

/** Joystick enumeration function.
 */
BOOL FAR PASCAL RetroPlatform::EnumerateJoystick(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef)
{
  char strHostInputID[CFG_FILENAME_LENGTH];
  WCHAR szHostInputID[CFG_FILENAME_LENGTH];
  WCHAR szHostInputName[CFG_FILENAME_LENGTH];

  _core.Log->AddLog("**** Joystick %d **** '%s'\n", ++iNumberOfJoysticksAttached, pdinst->tszProductName);

  sprintf(strHostInputID, "GP_ANALOG%d", iNumberOfJoysticksAttached - 1);
  mbstowcs(szHostInputID, strHostInputID, CFG_FILENAME_LENGTH);
  mbstowcs(szHostInputName, pdinst->tszProductName, CFG_FILENAME_LENGTH);

  RP.SendInputDevice(RP_HOSTINPUT_JOYSTICK, RP_FEATURE_INPUTDEVICE_JOYSTICK | RP_FEATURE_INPUTDEVICE_GAMEPAD, 0, szHostInputID, szHostInputName);

  if (iNumberOfJoysticksAttached == RETRO_PLATFORM_NUM_GAMEPORTS)
    return DIENUM_STOP;
  else
    return DIENUM_CONTINUE;
}

/** Determine the number of joysticks connected to the system.
 */
int RetroPlatform::EnumerateJoysticks()
{
  IDirectInput8 *RP_lpDI = nullptr;

  _core.Log->AddLog("RetroPlatform::EnumerateJoysticks()\n");

  if (!RP_lpDI)
  {
    HRESULT hResult = CoCreateInstance(CLSID_DirectInput8, nullptr, CLSCTX_INPROC_SERVER, IID_IDirectInput8, (LPVOID *)&RP_lpDI);
    if (hResult != DI_OK)
    {
      _core.Log->AddLog(
          "RetroPlatform::EnumerateJoysticks(): CoCreateInstance() "
          "failed, errorcode %d\n",
          hResult);
      return 0;
    }

    hResult = IDirectInput8_Initialize(RP_lpDI, win_drv_hInstance, DIRECTINPUT_VERSION);
    if (hResult != DI_OK)
    {
      _core.Log->AddLog(
          "RetroPlatform::EnumerateJoysticks(): Initialize() failed, "
          "errorcode %d\n",
          hResult);
      return 0;
    }

    iNumberOfJoysticksAttached = 0;

    hResult = IDirectInput8_EnumDevices(RP_lpDI, DI8DEVCLASS_GAMECTRL, RetroPlatformEnumerateJoystick, RP_lpDI, DIEDFL_ATTACHEDONLY);
    if (hResult != DI_OK)
    {
      _core.Log->AddLog(
          "RetroPlatform::EnumerateJoysticks(): EnumDevices() failed, "
          "errorcode %d\n",
          hResult);
      return 0;
    }

    if (RP_lpDI != nullptr)
    {
      IDirectInput8_Release(RP_lpDI);
      RP_lpDI = nullptr;
    }
  }

  _core.Log->AddLog("RetroPlatform::EnumerateJoysticks(): detected %d joystick(s).\n", iNumberOfJoysticksAttached);

  return iNumberOfJoysticksAttached;
}

/** Set clipping offset that is applied to the left of the picture.
 */
void RetroPlatform::SetClippingOffsetLeft(const uint32_t lOffsetLeft)
{
  lClippingOffsetLeftRP = lOffsetLeft;

#ifdef _DEBUG
  _core.Log->AddLog("RetroPlatform::SetClippingOffsetLeft(%u)\n", lOffsetLeft);
#endif
}

/** Set clipping offset that is applied to the top of the picture
 */
void RetroPlatform::SetClippingOffsetTop(const uint32_t lOffsetTop)
{
  lClippingOffsetTopRP = lOffsetTop;

#ifdef _DEBUG
  _core.Log->AddLog("RetroPlatform::SetClippingOffsetTop(%u)\n", lOffsetTop);
#endif
}

/** configure keyboard layout to custom key mappings
 *
 * Gameport 0 is statically mapped to internal keyboard layout GP_JOYKEY0,
 * gameport 1 to GP_JOYKEY1 as we reconfigure them anyway
 */
void RetroPlatform::SetCustomKeyboardLayout(const uint32_t lGameport, const char *pszKeys)
{
  const char *CustomLayoutKeys[RETRO_PLATFORM_KEYSET_COUNT] = {"up", "right", "down", "left", "fire", "fire.autorepeat"};
  int l[RETRO_PLATFORM_KEYSET_COUNT], n;
  char *psz;
  size_t ln;

  _core.Log->AddLog(" Configuring keyboard layout %d to %s.\n", lGameport, pszKeys);

  // keys not (always) configured via custom layouts
  kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_FIRE1_ACTIVE : EVENT_JOY0_FIRE1_ACTIVE, 0);
  kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_AUTOFIRE0_ACTIVE : EVENT_JOY0_AUTOFIRE0_ACTIVE, 0);
  kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_AUTOFIRE1_ACTIVE : EVENT_JOY0_AUTOFIRE1_ACTIVE, 0);

  while (*pszKeys)
  {
    for (; *pszKeys == ' '; pszKeys++)
      ; // skip spaces

    for (n = 0; n < RETRO_PLATFORM_KEYSET_COUNT; n++)
    {
      ln = strlen(CustomLayoutKeys[n]);
      if (strnicmp(pszKeys, CustomLayoutKeys[n], ln) == 0 && *(pszKeys + ln) == '=') break;
    }
    if (n < RETRO_PLATFORM_KEYSET_COUNT)
    {
      pszKeys += ln + 1;
      l[n] = kbddrv_DIK_to_symbol[strtoul(pszKeys, &psz,
                                          0)]; // convert DIK_* DirectInput key codes to symbolic keycodes

      // perform the individual mappings
      if (strnicmp(CustomLayoutKeys[n], "up", strlen("up")) == 0)
        kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_UP_ACTIVE : EVENT_JOY0_UP_ACTIVE, l[n]);
      else if (strnicmp(CustomLayoutKeys[n], "down", strlen("down")) == 0)
        kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_DOWN_ACTIVE : EVENT_JOY0_DOWN_ACTIVE, l[n]);
      else if (strnicmp(CustomLayoutKeys[n], "left", strlen("left")) == 0)
        kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_LEFT_ACTIVE : EVENT_JOY0_LEFT_ACTIVE, l[n]);
      else if (strnicmp(CustomLayoutKeys[n], "right", strlen("right")) == 0)
        kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_RIGHT_ACTIVE : EVENT_JOY0_RIGHT_ACTIVE, l[n]);
      else if (strnicmp(CustomLayoutKeys[n], "fire", strlen("fire")) == 0)
        kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_FIRE0_ACTIVE : EVENT_JOY0_FIRE0_ACTIVE, l[n]);
      else if (strnicmp(CustomLayoutKeys[n], "fire.autorepeat", strlen("fire.autorepeat")) == 0)
        kbdDrvJoystickReplacementSet((lGameport == 1) ? EVENT_JOY1_AUTOFIRE0_ACTIVE : EVENT_JOY0_AUTOFIRE0_ACTIVE, l[n]);
    }
    for (; *pszKeys != ' ' && *pszKeys != 0; pszKeys++)
      ; // reach next key definition
  }

  for (n = 0; n < RETRO_PLATFORM_KEYSET_COUNT; n++)
    _core.Log->AddLog(" Direction %s mapped to key %s.\n", CustomLayoutKeys[n], kbdDrvKeyString(l[n]));
}

/** Attach input devices to gameports during runtime of the emulator.
 *
 * The device is selected in the RetroPlatform player and passed to the emulator
 * in form of an IPC message.
 */
bool RetroPlatform::ConnectInputDeviceToPort(const uint32_t lGameport, const uint32_t lDeviceType, DWORD dwFlags, const char *szName)
{
  if (lGameport < 0 || lGameport >= RETRO_PLATFORM_NUM_GAMEPORTS) return false;

  _core.Log->AddLog(
      "RetroPlatform::ConnectInputDeviceToPort(): port %d, device "
      "type %d, flags %d, name '%s'\n",
      lGameport,
      lDeviceType,
      dwFlags,
      szName);

  switch (lDeviceType)
  {
    case RP_INPUTDEVICE_EMPTY:
      _core.Log->AddLog(" Removing input device from gameport..\n");
      gameportSetInput(lGameport, GP_NONE);
      kbdDrvSetJoyKeyEnabled(lGameport, lGameport, FALSE);
      return true;
    case RP_INPUTDEVICE_MOUSE:
      _core.Log->AddLog(" Attaching mouse device to gameport..\n");
      gameportSetInput(lGameport, GP_MOUSE0);
      return true;
    case RP_INPUTDEVICE_JOYSTICK:
      if (strcmp(szName, "GP_ANALOG0") == 0)
      {
        _core.Log->AddLog(" Attaching joystick 1 to gameport..\n");
        gameportSetInput(lGameport, GP_ANALOG0);
      }
      else if (strcmp(szName, "GP_ANALOG1") == 0)
      {
        _core.Log->AddLog(" Attaching joystick 2 to gameport..\n");
        gameportSetInput(lGameport, GP_ANALOG1);
      }
      else if (_strnicmp(szName, "GP_JOYKEYCUSTOM", strlen("GP_JOYKEYCUSTOM")) == 0)
      { // custom layout
        RetroPlatform::SetCustomKeyboardLayout(lGameport, szName + strlen("GP_JOYKEYCUSTOM") + 1);
        gameportSetInput(lGameport, (lGameport == 1) ? GP_JOYKEY1 : GP_JOYKEY0);
        if (lGameport == 0)
        {
          kbdDrvSetJoyKeyEnabled(lGameport, 0, TRUE);
          kbdDrvSetJoyKeyEnabled(lGameport, 1, FALSE);
        }
        else if (lGameport == 1)
        {
          kbdDrvSetJoyKeyEnabled(lGameport, 0, FALSE);
          kbdDrvSetJoyKeyEnabled(lGameport, 1, TRUE);
        }
      }
#ifdef FELLOW_SUPPORT_RP_API_VERSION_71
      else if (_strnicmp(szName, "", 1) == 0)
      {
        _core.Log->AddLog(
            " RetroPlatform controlled joystick device connect to "
            "gameport %u, leaving control up to host.\n",
            lGameport);

        if (lGameport == 0)
          gameportSetInput(lGameport, RP_JOYSTICK0);
        else if (lGameport == 1)
          gameportSetInput(lGameport, RP_JOYSTICK1);

        return true;
      }
#endif
      else
      {
        _core.Log->AddLog(" WARNING: Unknown joystick input device name, ignoring..\n");
        return false;
      }
      return true;
    default: _core.Log->AddLog(" WARNING: Unsupported input device type detected.\n"); return false;
  }
}

/** Translate a RetroPlatform IPC message code into readable text.
 */
const char *RetroPlatform::GetMessageText(uint32_t iMsg)
{
  switch (iMsg)
  {
    case RP_IPC_TO_HOST_FEATURES: return TEXT("RP_IPC_TO_HOST_FEATURES");
    case RP_IPC_TO_HOST_CLOSED: return TEXT("RP_IPC_TO_HOST_CLOSED");
    case RP_IPC_TO_HOST_ACTIVATED: return TEXT("RP_IPC_TO_HOST_ACTIVATED");
    case RP_IPC_TO_HOST_DEACTIVATED: return TEXT("RP_IPC_TO_HOST_DEACTIVATED");
    case RP_IPC_TO_HOST_SCREENMODE: return TEXT("RP_IPC_TO_HOST_SCREENMODE");
    case RP_IPC_TO_HOST_POWERLED: return TEXT("RP_IPC_TO_HOST_POWERLED");
    case RP_IPC_TO_HOST_DEVICES: return TEXT("RP_IPC_TO_HOST_DEVICES");
    case RP_IPC_TO_HOST_DEVICEACTIVITY: return TEXT("RP_IPC_TO_HOST_DEVICEACTIVITY");
    case RP_IPC_TO_HOST_MOUSECAPTURE: return TEXT("RP_IPC_TO_HOST_MOUSECAPTURE");
    case RP_IPC_TO_HOST_HOSTAPIVERSION: return TEXT("RP_IPC_TO_HOST_HOSTAPIVERSION");
    case RP_IPC_TO_HOST_PAUSE: return TEXT("RP_IPC_TO_HOST_PAUSE");
    case RP_IPC_TO_HOST_DEVICECONTENT: return TEXT("RP_IPC_TO_HOST_DEVICECONTENT");
    case RP_IPC_TO_HOST_TURBO: return TEXT("RP_IPC_TO_HOST_TURBO");
    case RP_IPC_TO_HOST_PING: return TEXT("RP_IPC_TO_HOST_PING");
    case RP_IPC_TO_HOST_VOLUME: return TEXT("RP_IPC_TO_HOST_VOLUME");
#ifndef FELLOW_SUPPORT_RP_API_VERSION_71
    case RP_IPC_TO_HOST_ESCAPED: return TEXT("RP_IPC_TO_HOST_ESCAPED");
#endif
    case RP_IPC_TO_HOST_PARENT: return TEXT("RP_IPC_TO_HOST_PARENT");
    case RP_IPC_TO_HOST_DEVICESEEK: return TEXT("RP_IPC_TO_HOST_DEVICESEEK");
    case RP_IPC_TO_HOST_CLOSE: return TEXT("RP_IPC_TO_HOST_CLOSE");
    case RP_IPC_TO_HOST_DEVICEREADWRITE: return TEXT("RP_IPC_TO_HOST_DEVICEREADWRITE");
    case RP_IPC_TO_HOST_HOSTVERSION: return TEXT("RP_IPC_TO_HOST_HOSTVERSION");
    case RP_IPC_TO_HOST_INPUTDEVICE: return TEXT("RP_IPC_TO_HOST_INPUTDEVICE");
    case RP_IPC_TO_GUEST_CLOSE: return TEXT("RP_IPC_TO_GUEST_CLOSE");
    case RP_IPC_TO_GUEST_SCREENMODE: return TEXT("RP_IPC_TO_GUEST_SCREENMODE");
    case RP_IPC_TO_GUEST_SCREENCAPTURE: return TEXT("RP_IPC_TO_GUEST_SCREENCAPTURE");
    case RP_IPC_TO_GUEST_PAUSE: return TEXT("RP_IPC_TO_GUEST_PAUSE");
    case RP_IPC_TO_GUEST_DEVICEACTIVITY: return TEXT("RP_IPC_TO_GUEST_DEVICEACTIVITY");
    case RP_IPC_TO_GUEST_DEVICECONTENT: return TEXT("RP_IPC_TO_GUEST_DEVICECONTENT");
    case RP_IPC_TO_GUEST_RESET: return TEXT("RP_IPC_TO_GUEST_RESET");
    case RP_IPC_TO_GUEST_TURBO: return TEXT("RP_IPC_TO_GUEST_TURBO");
    case RP_IPC_TO_GUEST_PING: return TEXT("RP_IPC_TO_GUEST_PING");
    case RP_IPC_TO_GUEST_VOLUME: return TEXT("RP_IPC_TO_GUEST_VOLUME");
#ifndef FELLOW_SUPPORT_RP_API_VERSION_71
    case RP_IPC_TO_GUEST_ESCAPEKEY: return TEXT("RP_IPC_TO_GUEST_ESCAPEKEY");
#endif
    case RP_IPC_TO_GUEST_EVENT: return TEXT("RP_IPC_TO_GUEST_EVENT");
    case RP_IPC_TO_GUEST_MOUSECAPTURE: return TEXT("RP_IPC_TO_GUEST_MOUSECAPTURE");
    case RP_IPC_TO_GUEST_SAVESTATE: return TEXT("RP_IPC_TO_GUEST_SAVESTATE");
    case RP_IPC_TO_GUEST_LOADSTATE: return TEXT("RP_IPC_TO_GUEST_LOADSTATE");
    case RP_IPC_TO_GUEST_FLUSH: return TEXT("RP_IPC_TO_GUEST_FLUSH");
    case RP_IPC_TO_GUEST_DEVICEREADWRITE: return TEXT("RP_IPC_TO_GUEST_DEVICEREADWRITE");
    case RP_IPC_TO_GUEST_QUERYSCREENMODE: return TEXT("RP_IPC_TO_GUEST_QUERYSCREENMODE");
    case RP_IPC_TO_GUEST_GUESTAPIVERSION: return TEXT("RP_IPC_TO_GUEST_GUESTAPIVERSION");
    default: return TEXT("UNKNOWN");
  }
}

/** Determine a timestamp for the current time.
 */
ULONGLONG RetroPlatform::GetTime()
{
  SYSTEMTIME st;
  FILETIME ft;
  ULARGE_INTEGER li;

  GetSystemTime(&st);
  if (!SystemTimeToFileTime(&st, &ft)) return 0;
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;
  return li.QuadPart / 10000;
}

/** Send an IPC message to RetroPlatform host.
 * @return true is sucessfully sent, false otherwise.
 */
bool RetroPlatform::SendMessageToHost(uint32_t iMessage, WPARAM wParam, LPARAM lParam, LPCVOID pData, DWORD dwDataSize, const RPGUESTINFO *pGuestInfo, LRESULT *plResult)
{
  bool bResult = RPSendMessage(iMessage, wParam, lParam, pData, dwDataSize, pGuestInfo, plResult) ? true : false;

#ifdef _DEBUG
  if (bResult)
    _core.Log->AddLog(
        "RetroPlatform::SendMessageToHost(): sent message ([%s], "
        "%08x, %08x, %08x, %d)\n",
        RetroPlatform::GetMessageText(iMessage),
        iMessage - WM_APP,
        wParam,
        lParam,
        pData);
  else
    _core.Log->AddLog(
        "RetroPlatform::SendMessageToHost(): could not send message, "
        "error: %d\n",
        GetLastError());
#endif

  return bResult;
}

uint32_t RetroPlatform::GetClippingOffsetLeftAdjusted()
{
  uint32_t lClippingOffsetLeft = lClippingOffsetLeftRP;

  if (lClippingOffsetLeft >= RETRO_PLATFORM_OFFSET_ADJUST_LEFT) lClippingOffsetLeft = (lClippingOffsetLeft - RETRO_PLATFORM_OFFSET_ADJUST_LEFT);

  lClippingOffsetLeft /= 2;

#ifdef _DEBUGVERBOSE
  _core.Log->AddLog(
      "RetroPlatform::GetClippingOffsetLeftAdjusted(): left offset "
      "adjusted from %u to %u\n",
      lClippingOffsetLeftRP,
      lClippingOffsetLeft);
#endif

  return lClippingOffsetLeft;
}

uint32_t RetroPlatform::GetClippingOffsetTopAdjusted()
{
  uint32_t lClippingOffsetTop = lClippingOffsetTopRP;

  if (lClippingOffsetTop >= RETRO_PLATFORM_OFFSET_ADJUST_TOP) lClippingOffsetTop -= RETRO_PLATFORM_OFFSET_ADJUST_TOP;

#ifdef _DEBUGVERBOSE
  _core.Log->AddLog(
      "RetroPlatform::GetClippingOffsetTopAdjusted(): top offset "
      "adjusted from %u to %u\n",
      lClippingOffsetTopRP,
      lClippingOffsetTop);
#endif

  return lClippingOffsetTop;
}

uint32_t RetroPlatform::GetClippingOffsetLeft()
{
  return lClippingOffsetLeftRP;
}

uint32_t RetroPlatform::GetClippingOffsetTop()
{
  return lClippingOffsetTopRP;
}

uint32_t RetroPlatform::GetScreenHeightAdjusted()
{
  uint32_t lScreenHeight = lScreenHeightRP;

  lScreenHeight *= RetroPlatform::GetDisplayScale();

  return lScreenHeight;
}

uint32_t RetroPlatform::GetSourceBufferHeight()
{
  return lScreenHeightRP;
}

uint32_t RetroPlatform::GetCPUSpeed()
{
  return cfgGetCPUSpeed(pConfig);
}

uint32_t RetroPlatform::GetScreenWidthAdjusted()
{
  uint32_t lScreenWidth = 0;

  lScreenWidth = lScreenWidthRP / 2 * RetroPlatform::GetDisplayScale();

  return lScreenWidth;
}

uint32_t RetroPlatform::GetSourceBufferWidth()
{
  return lScreenWidthRP / 2;
}

uint32_t RetroPlatform::GetScreenWidth()
{
  return lScreenWidthRP;
}

bool RetroPlatform::GetScreenWindowed()
{
  return bScreenWindowed;
}

bool RetroPlatform::GetScanlines()
{
  return bScanlines;
}

uint32_t RetroPlatform::GetScreenHeight()
{
  return lScreenHeightRP;
}

/** Translate the screenmode configured in the configuration file and pass it
 * along to the RetroPlatform Player.
 */
void RetroPlatform::DetermineScreenModeFromConfig(struct RPScreenMode *RetroPlatformScreenMode, cfg *RetroPlatformConfig)
{
  DWORD dwScreenMode = 0;

  if (RP.GetDisplayScale() == 1) dwScreenMode |= RP_SCREENMODE_SCALE_1X;
  if (RP.GetDisplayScale() == 2) dwScreenMode |= RP_SCREENMODE_SCALE_2X;
  if (RP.GetDisplayScale() == 3) dwScreenMode |= RP_SCREENMODE_SCALE_3X;
  if (RP.GetDisplayScale() == 4) dwScreenMode |= RP_SCREENMODE_SCALE_4X;

  if (RP.GetScreenWindowed())
    dwScreenMode |= RP_SCREENMODE_DISPLAY_WINDOW;
  else
    dwScreenMode |= RP_SCREENMODE_DISPLAY_FULLSCREEN_1;

  RetroPlatformScreenMode->dwScreenMode = dwScreenMode;
  RetroPlatformScreenMode->hGuestWindow = hGuestWindow;
  RetroPlatformScreenMode->lTargetHeight = RP.GetScreenHeight();
  RetroPlatformScreenMode->lTargetWidth = RP.GetScreenWidth();
  RetroPlatformScreenMode->lClipLeft = RP.GetClippingOffsetLeft();
  RetroPlatformScreenMode->lClipTop = RP.GetClippingOffsetTop();
  RetroPlatformScreenMode->lClipWidth = RP.GetScreenWidth();
  RetroPlatformScreenMode->lClipHeight = RP.GetScreenHeight();
  RetroPlatformScreenMode->dwClipFlags = 0;
}

bool RetroPlatform::GetEmulationPaused()
{
  return bEmulationPaused;
}

uint32_t RetroPlatform::GetDisplayScale()
{
  return lDisplayScale;
}

/** Determine the RetroPlatform host version.
 *
 * @param[out] lpMainVersion main version number
 * @param[out] lpRevision revision number
 * @param[out] lpBuild build number
 * @return true is successful, false otherwise.
 */
bool RetroPlatform::GetHostVersion(uint32_t *lpMainVersion, uint32_t *lpRevision, uint32_t *lpBuild)
{
  LRESULT lResult = 0;

  if (!RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_HOSTVERSION, 0, 0, nullptr, 0, &GuestInfo, (LRESULT *)&lResult)) return false;

  *lpMainVersion = RP_HOSTVERSION_MAJOR(lResult);
  *lpRevision = RP_HOSTVERSION_MINOR(lResult);
  *lpBuild = RP_HOSTVERSION_BUILD(lResult);
  return true;
}

/** Verify if the emulator is operating in RetroPlatform mode.
 *
 * Checks the value of the bRetroPlatformMode flag. It is set to true, if a
 * RetroPlatform host ID has been passed along as a commandline parameter.
 * @return true if WinFellow was called from Cloanto RetroPlatform, false if
 * not.
 */
bool RetroPlatform::GetHeadlessMode()
{
  return bRetroPlatformMode;
}

/** Asynchronously post a message to the RetroPlatform host.
 *
 * A message is posted to the host asynchronously, i.e. without waiting for
 * results.
 */
bool RetroPlatform::PostMessageToHost(uint32_t iMessage, WPARAM wParam, LPARAM lParam, const RPGUESTINFO *pGuestInfo)
{
  bool bResult = RPPostMessage(iMessage, wParam, lParam, pGuestInfo) ? true : false;

#ifdef _DEBUG
#ifndef RETRO_PLATFORM_LOG_VERBOSE
  if (iMessage != RP_IPC_TO_HOST_DEVICESEEK && iMessage != RP_IPC_TO_HOST_DEVICEACTIVITY)
  {
#endif !RETRO_PLATFORM_LOG_VERBOSE

    if (bResult)
      _core.Log->AddLog(
          "RetroPlatform::PostMessageToHost() posted message ([%s], "
          "%08x, %08x, %08x)\n",
          RetroPlatform::GetMessageText(iMessage),
          iMessage - WM_APP,
          wParam,
          lParam);
    else
      _core.Log->AddLog(
          "RetroPlatform::PostMessageToHost() could not post message, "
          "error: %d\n",
          GetLastError());

#ifndef RETRO_PLATFORM_LOG_VERBOSE
  }
#endif
#endif // _DEBUG

  return bResult;
}

/** Post message to the player to signalize that the guest wants to escape the
 * mouse cursor.
 */
#ifndef FELLOW_SUPPORT_RP_API_VERSION_71

bool RetroPlatform::PostEscaped()
{
  return RetroPlatform::PostMessageToHost(RP_IPC_TO_HOST_ESCAPED, 0, 0, &GuestInfo);
}

#endif

/** Control status of the RetroPlatform hard drive LEDs.
 *
 * Sends LED status changes to the RetroPlatform host in the form of
 * RP_IPC_TO_HOST_DEVICEACTIVITY messages, so that hard drive read and write
 * activity can be displayed, and detected (undo functionality uses write
 * messages as fallback method to detect changed floppy images).
 * @param[in] lHardDriveNo   hard drive index (0-...)
 * @param[in] bActive        flag indicating disk access (active/inactive)
 * @param[in] bWriteActivity flag indicating type of access (write/read)
 * @return true if message sent successfully, false otherwise.
 * @callergraph
 */
bool RetroPlatform::PostHardDriveLED(const uint32_t lHardDriveNo, const bool bActive, const bool bWriteActivity)
{
  static int oldleds[FHFILE_MAX_DEVICES];
  static ULONGLONG lastsent[FHFILE_MAX_DEVICES];

  if (!bInitialized) return false;

  int state = bActive ? 1 : 0;
  state |= bWriteActivity ? 2 : 0;

  if (state == oldleds[lHardDriveNo])
    return true;
  else
    oldleds[lHardDriveNo] = state;

  if (bActive && (lastsent[lHardDriveNo] + RETRO_PLATFORM_HARDDRIVE_BLINK_MSECS < RetroPlatform::GetTime()) || (bActive && bWriteActivity))
  {
    RetroPlatform::PostMessageToHost(
        RP_IPC_TO_HOST_DEVICEACTIVITY,
        MAKEWORD(RP_DEVICECATEGORY_HD, lHardDriveNo),
        MAKELONG(RETRO_PLATFORM_HARDDRIVE_BLINK_MSECS, bWriteActivity ? RP_DEVICEACTIVITY_WRITE : RP_DEVICEACTIVITY_READ),
        &GuestInfo);
    lastsent[lHardDriveNo] = RetroPlatform::GetTime();
  }
  else
    return true;
  return true;
}

/** Control status of the RetroPlatform floppy drive LEDs.
 *
 * Sends LED status changes to the RetroPlatform host in the form of
 * RP_IPC_TO_HOST_DEVICEACTIVITY messages, so that floppy read and write
 * activity can be displayed, and detected (undo functionality uses write
 * messages as fallback method to detect changed floppy images).
 * @param[in] lFloppyDriveNo floppy drive index (0-3)
 * @param[in] bMotorActive   state of floppy drive motor (active/inactive)
 * @param[in] bWriteActivity type of access (write/read)
 * @return true if message sent successfully, false otherwise.
 * @callergraph
 */
bool RetroPlatform::PostFloppyDriveLED(const uint32_t lFloppyDriveNo, const bool bMotorActive, const bool bWriteActivity)
{
  if (lFloppyDriveNo > 3) return false;

  return RetroPlatform::PostMessageToHost(
      RP_IPC_TO_HOST_DEVICEACTIVITY,
      MAKEWORD(RP_DEVICECATEGORY_FLOPPY, lFloppyDriveNo),
      MAKELONG(bMotorActive ? -1 : 0, (bWriteActivity) ? RP_DEVICEACTIVITY_WRITE : RP_DEVICEACTIVITY_READ),
      &GuestInfo);
}

/** Send content of floppy drive to RetroPlatform host.
 * The read-only state is determined and sent here, however at this point
 * it is usually wrong, as floppySetDiskImage only reflects the ability to write
 * to the file in the writeprot flag.
 * The actual state within the config is configured in a separate call within
 * cfgManagerConfigurationActivate - therefore an update message is sent later.
 * @param[in] lFloppyDriveNo floppy drive index (0-3)
 * @param[in] szImageName ANSI string containing the floppy image name
 * @param[in] bWriteProtected flag indicating the read-only state of the drive
 * @return true if message sent successfully, false otherwise.
 * @sa RetroPlatformSendFloppyDriveReadOnly
 * @callergraph
 */
bool RetroPlatform::SendFloppyDriveContent(const uint32_t lFloppyDriveNo, const char *szImageName, const bool bWriteProtected)
{
  bool bResult;
  struct RPDeviceContent rpDeviceContent = {0};

  if (!bInitialized) return false;

  if (!floppy[lFloppyDriveNo].enabled) return false;

  rpDeviceContent.btDeviceCategory = RP_DEVICECATEGORY_FLOPPY;
  rpDeviceContent.btDeviceNumber = lFloppyDriveNo;
  rpDeviceContent.dwInputDevice = 0;

  if (szImageName)
    mbstowcs(rpDeviceContent.szContent, szImageName, CFG_FILENAME_LENGTH);
  else
    wcscpy(rpDeviceContent.szContent, L"");

  rpDeviceContent.dwFlags = (bWriteProtected ? RP_DEVICEFLAGS_RW_READONLY : RP_DEVICEFLAGS_RW_READWRITE);

#ifdef _DEBUG
  _core.Log->AddLog(
      "RetroPlatform::SendFloppyDriveContent(): "
      "RP_IPC_TO_HOST_DEVICECONTENT cat=%d num=%d type=%d '%s'\n",
      rpDeviceContent.btDeviceCategory,
      rpDeviceContent.btDeviceNumber,
      rpDeviceContent.dwInputDevice,
      szImageName);
#endif

  bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_DEVICECONTENT, 0, 0, &rpDeviceContent, sizeof(struct RPDeviceContent), &GuestInfo, nullptr);

  _core.Log->AddLog("RetroPlatform::SendFloppyDriveContent(%d, '%s'): %s.\n", lFloppyDriveNo, szImageName, bResult ? "successful" : "failed");

  return bResult;
}

/** Send actual write protection state of drive to RetroPlatform host.
 * Ignores drives that are not enabled.
 * @param[in] lFloppyDriveNo floppy drive index (0-3)
 * @param[in] bWriteProtected flag indicating the read-only state of the drive
 * @return true if message sent successfully, false otherwise.
 * @callergraph
 */
bool RetroPlatform::SendFloppyDriveReadOnly(const uint32_t lFloppyDriveNo, const bool bWriteProtected)
{
  if (!bInitialized) return false;

  if (!floppy[lFloppyDriveNo].enabled) return false;

  bool bResult = RetroPlatform::SendMessageToHost(
      RP_IPC_TO_HOST_DEVICEREADWRITE,
      MAKEWORD(RP_DEVICECATEGORY_FLOPPY, lFloppyDriveNo),
      bWriteProtected ? RP_DEVICE_READONLY : RP_DEVICE_READWRITE,
      nullptr,
      0,
      &GuestInfo,
      nullptr);

  _core.Log->AddLog("RetroPlatform::SendFloppyDriveReadOnly(): %s.\n", bResult ? "successful" : "failed");

  return bResult;
}

/** Send floppy turbo mode state to RetroPlatform host.
 * @param[in] bTurbo flag indicating state of turbo mode
 * @return true if message sent successfully, false otherwise.
 * @callergraph
 */
bool RetroPlatform::SendFloppyTurbo(const bool bTurbo)
{
  if (!bInitialized) return false;

  bool bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_TURBO, RP_TURBO_FLOPPY, bTurbo ? RP_TURBO_FLOPPY : 0, nullptr, 0, &GuestInfo, nullptr);

  _core.Log->AddLog("RetroPlatform::SendFloppyDriveTurbo(): %s.\n", bResult ? "successful" : "failed");

  return bResult;
}

/**
 * Send floppy drive seek events to RetroPlatform host.
 *
 * Will notify the RetroPlatform player about changes in the drive head
 * position.
 * @param[in] lFloppyDriveNo index of floppy drive
 * @param[in] lTrackNo index of floppy track
 * @return true if message sent successfully, false otherwise.
 * @callergraph
 */
bool RetroPlatform::PostFloppyDriveSeek(const uint32_t lFloppyDriveNo, const uint32_t lTrackNo)
{
  if (!bInitialized) return false;

  return RetroPlatform::PostMessageToHost(RP_IPC_TO_HOST_DEVICESEEK, MAKEWORD(RP_DEVICECATEGORY_FLOPPY, lFloppyDriveNo), lTrackNo, &GuestInfo);
}

/**
 * Send gameport activity to RetroPlatform host.
 */
bool RetroPlatform::PostGameportActivity(const uint32_t lGameport, const uint32_t lGameportMask)
{
  bool bResult;

#ifdef _DEBUG
  _core.Log->AddLog("RetroPlatform::PostGameportActivity(): lGameport=%u, lGameportMask=%u\n", lGameport, lGameportMask);
#endif

  if (!bInitialized) return false;

  if (lGameport >= RETRO_PLATFORM_NUM_GAMEPORTS) return false;

  bResult = RetroPlatform::PostMessageToHost(RP_IPC_TO_HOST_DEVICEACTIVITY, MAKEWORD(RP_DEVICECATEGORY_INPUTPORT, lGameport), lGameportMask, &GuestInfo);

  return bResult;
}

/** Send content of hard drive to RetroPlatform host.
 * @param[in] lHardDriveNo hard drive index (0-...)
 * @param[in] szImageName ANSI string containing the floppy image name
 * @param[in] bWriteProtected flag indicating the read-only state of the drive
 * @return true if message sent successfully, false otherwise.
 * @callergraph
 */
bool RetroPlatform::SendHardDriveContent(const uint32_t lHardDriveNo, const char *szImageName, const bool bWriteProtected)
{
  bool bResult;
  struct RPDeviceContent rpDeviceContent = {0};

  if (!bInitialized) return false;

  rpDeviceContent.btDeviceCategory = RP_DEVICECATEGORY_HD;
  rpDeviceContent.btDeviceNumber = lHardDriveNo;
  rpDeviceContent.dwInputDevice = 0;

  if (szImageName)
    mbstowcs(rpDeviceContent.szContent, szImageName, CFG_FILENAME_LENGTH);
  else
    wcscpy(rpDeviceContent.szContent, L"");

  rpDeviceContent.dwFlags = (bWriteProtected ? RP_DEVICEFLAGS_RW_READONLY : RP_DEVICEFLAGS_RW_READWRITE);

#ifdef _DEBUG
  _core.Log->AddLog(
      "RP_IPC_TO_HOST_DEVICECONTENT cat=%d num=%d type=%d '%s'\n",
      rpDeviceContent.btDeviceCategory,
      rpDeviceContent.btDeviceNumber,
      rpDeviceContent.dwInputDevice,
      rpDeviceContent.szContent);
#endif

  bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_DEVICECONTENT, 0, 0, &rpDeviceContent, sizeof(struct RPDeviceContent), &GuestInfo, nullptr);

  _core.Log->AddLog("RetroPlatform::SendHardDriveContent(%d, '%s'): %s.\n", lHardDriveNo, szImageName, bResult ? "successful" : "failed");

  return bResult;
}

/** Control status of power LED in RetroPlatform player.
 *
 * Examines the current on/off state of the emulator session and sends it to the
 * RetroPlatform player.
 * @param[in] wIntensityPercent intensity of the power LED in percent, with 0
 * being off, 100 being full intensity.
 * @return true, if valid value was passed, false if invalid value.
 */
bool RetroPlatform::PostPowerLEDIntensityPercent(const WPARAM wIntensityPercent)
{
  if (wIntensityPercent <= 100 && wIntensityPercent >= 0)
    return RetroPlatform::PostMessageToHost(RP_IPC_TO_HOST_POWERLED, wIntensityPercent, 0, &GuestInfo);
  else
    return false;
}

/** Set RetroPlatform escape key.
 *
 * Called during parsing of the command-line parameters, which is why the
 * keyboard modules have to be initialized before the config modules, as we use
 * the key mappings here.
 */
void RetroPlatform::SetEscapeKey(const uint32_t lNewEscapeKey)
{
  lEscapeKey = lNewEscapeKey;
  _core.Log->AddLog("RetroPlatform::SetEscapeKey(): escape key configured to %s.\n", kbdDrvKeyString(lEscapeKey));
}

void RetroPlatform::SetEscapeKeyHoldTime(const uint32_t lNewEscapeKeyHoldtime)
{
  lEscapeKeyHoldTime = lNewEscapeKeyHoldtime;
  _core.Log->AddLog(
      "RetroPlatform::SetEscapeKeyHoldTime(): escape hold time "
      "configured to %d.\n",
      lEscapeKeyHoldTime);
}

ULONGLONG RetroPlatform::SetEscapeKeyHeld(const bool bEscapeKeyHeld)
{
  ULONGLONG t = 0;

  if (bEscapeKeyHeld)
  {
    if (lEscapeKeyHeldSince == 0)
    {
      lEscapeKeyHeldSince = RetroPlatform::GetTime();
    }
  }
  else
  {
    if (lEscapeKeyHeldSince)
    {
      t = RetroPlatform::GetTime() - lEscapeKeyHeldSince;
      lEscapeKeyHeldSince = 0;
    }
  }
  return t;
}

void RetroPlatform::SetEscapeKeySimulatedTargetTime(const ULONGLONG tTargetTime)
{
  lEscapeKeySimulatedTargetTime = tTargetTime;
}

void RetroPlatform::SetEmulationState(const bool bNewState)
{
  if (bEmulationState != bNewState)
  {
    bEmulationState = bNewState;
    _core.Log->AddLog("RetroPlatform::SetEmulationState(%s).\n", bNewState ? "active" : "inactive");
    RetroPlatform::PostPowerLEDIntensityPercent(bNewState ? 100 : 0);
  }
}

void RetroPlatform::SetEmulationPaused(const bool bPaused)
{
  if (bPaused != bEmulationPaused)
  {
    bEmulationPaused = bPaused;

    _core.Log->AddLog("RetroPlatform::SetEmulationPaused(): emulation is now %s.\n", bPaused ? "paused" : "active");
  }
}

void RetroPlatform::SetEmulatorQuit(const bool bNewEmulatorQuit)
{
  bEmulatorQuit = bNewEmulatorQuit;

  _core.Log->AddLog("RetroPlatform::SetEmulatorQuit(%s).\n", bNewEmulatorQuit ? "true" : "false");
}

void RetroPlatform::SetHostID(const char *szNewHostID)
{
  strncpy(szHostID, szNewHostID, CFG_FILENAME_LENGTH);
  _core.Log->AddLog("RetroPlatform::SetHostID(): host ID configured to %s.\n", szHostID);
}

void RetroPlatform::SetHeadlessMode(const bool bRPMode)
{
  bRetroPlatformMode = bRPMode;

  _core.Log->AddLog("RetroPlatform::SetHeadlessMode(%s)\n", bRPMode ? "true" : "false");
}

void RetroPlatform::SetScanlines(const bool bNewScanlines)
{
  bScanlines = bNewScanlines;

  _core.Log->AddLog("RetroPlatform::SetScanlines(%s)\n", bScanlines ? "true" : "false");
}

/** Set screen height.
 */
void RetroPlatform::SetScreenHeight(const uint32_t lHeight)
{
  lScreenHeightRP = lHeight;

  _core.Log->AddLog("RetroPlatform::SetScreenHeight(): height configured to %u\n", lScreenHeightRP);
}

/** Set screen width.
 */
void RetroPlatform::SetScreenWidth(const uint32_t lWidth)
{
  lScreenWidthRP = lWidth;

  _core.Log->AddLog("RetroPlatform::SetScreenWidth(): width configured to %u\n", lScreenWidthRP);
}

void RetroPlatform::SetScreenWindowed(const bool bWindowed)
{
  bScreenWindowed = bWindowed;

  if (pConfig != nullptr)
  {
    cfgSetScreenWindowed(pConfig, bWindowed);
  }

  _core.Log->AddLog("RetroPlatform::SetScreenWindowed(): configured to %s\n", bWindowed ? "true" : "false");
}

void RetroPlatform::SetDisplayScale(const uint32_t lNewDisplayScale)
{
  lDisplayScale = lNewDisplayScale;

  _core.Log->AddLog("RetroPlatform::SetDisplayScale(): display scale configured to %u\n", lDisplayScale);
}

void RetroPlatform::SetScreenMode(const char *szScreenMode)
{
  uint32_t lScalingFactor = 0;

  lScreenMode = atol(szScreenMode);
  _core.Log->AddLog("RetroPlatform::SetScreenMode(): screen mode configured to 0x%x.\n", lScreenMode);

  lScalingFactor = RP_SCREENMODE_SCALE(lScreenMode);

  switch (lScalingFactor)
  {
    case RP_SCREENMODE_SCALE_1X: RetroPlatform::SetDisplayScale(1); break;
    case RP_SCREENMODE_SCALE_2X: RetroPlatform::SetDisplayScale(2); break;
    case RP_SCREENMODE_SCALE_3X: RetroPlatform::SetDisplayScale(3); break;
    case RP_SCREENMODE_SCALE_4X: RetroPlatform::SetDisplayScale(4); break;
    default:
      _core.Log->AddLog(
          "RetroPlatform::SetScreenMode(): WARNING: unknown display "
          "scaling factor 0x%x\n",
          lScalingFactor);
  }

  RetroPlatform::SetScanlines((lScreenMode & RP_SCREENMODE_SCANLINES) ? true : false);
}

void RetroPlatform::SetScreenModeStruct(struct RPScreenMode *sm)
{
  uint32_t lScalingFactor = 0, lDisplay = 0;

  lScalingFactor = RP_SCREENMODE_SCALE(sm->dwScreenMode);
  lDisplay = RP_SCREENMODE_DISPLAY(sm->dwScreenMode);

#ifdef _DEBUG
  _core.Log->AddLog(
      "RetroPlatform::SetScreenModeStruct(): dwScreenMode=0x%x, "
      "dwClipFlags=0x%x, lTargetWidth=%u, lTargetHeight=%u\n",
      sm->dwScreenMode,
      sm->dwClipFlags,
      sm->lTargetWidth,
      sm->lTargetHeight);

  _core.Log->AddLog(
      "RetroPlatform::SetScreenModeStruct(): lClipWidth=%u, "
      "lClipHeight=%u, lClipLeft=%u, lClipTop=%u\n",
      sm->lClipWidth,
      sm->lClipHeight,
      sm->lClipLeft,
      sm->lClipTop);

  _core.Log->AddLog(
      "RetroPlatform::SetScreenModeStruct(): lScalingFactor=0x%x, "
      "lDisplay=0x%x\n",
      lScalingFactor,
      lDisplay);
#endif

  if (lDisplay == 0)
  {
    RetroPlatform::SetScreenWindowed(true);

    switch (lScalingFactor)
    {
      case RP_SCREENMODE_SCALE_1X: RetroPlatform::SetDisplayScale(1); break;
      case RP_SCREENMODE_SCALE_2X: RetroPlatform::SetDisplayScale(2); break;
      case RP_SCREENMODE_SCALE_3X: RetroPlatform::SetDisplayScale(3); break;
      case RP_SCREENMODE_SCALE_4X: RetroPlatform::SetDisplayScale(4); break;
      default:
        _core.Log->AddLog(
            "RetroPlatform::SetScreenModeStruct(): WARNING: unknown "
            "windowed display scaling factor 0x%x.\n",
            lScalingFactor);
    }
  }

  if (lDisplay == 1)
  {
    RetroPlatform::SetScreenWindowed(false);

    switch (lScalingFactor)
    {
      case RP_SCREENMODE_SCALE_MAX:
        // automatically scale to max - set in conjunction with fullscreen mode
        RetroPlatform::SetDisplayScale(1);
        break;
      default:
        _core.Log->AddLog(
            "RetroPlatform::SetScreenModeStruct(): WARNING: unknown "
            "fullscreen 1 display scaling factor 0x%x.\n",
            lScalingFactor);
    }
  }

  RetroPlatform::SetClippingOffsetLeft(sm->lClipLeft);
  RetroPlatform::SetClippingOffsetTop(sm->lClipTop);
  RetroPlatform::SetScreenHeight(sm->lClipHeight);
  RetroPlatform::SetScreenWidth(sm->lClipWidth);
  _core.Log->AddLog("2 - SetScreenHeight and width: (%d, %d)\n", sm->lClipWidth, sm->lClipHeight);
  cfgSetScreenHeight(pConfig, sm->lClipHeight);
  cfgSetScreenWidth(pConfig, sm->lClipWidth);
  // Resume emulation, as graph module will crash otherwise if emulation is
  // paused. As the pause mode is not changed, after the restart of the session
  // it will be paused again.
  gfxDrvCommon->RunEventSet();
  RegisterRetroPlatformScreenMode(false);
  fellowRequestEmulationStop();
}

void RetroPlatform::RegisterRetroPlatformScreenMode(const bool bStartup)
{
  uint32_t lHeight, lWidth, lDisplayScale;

  if (RP.GetScanlines())
    cfgSetDisplayScaleStrategy(gfxDrvCommon->rp_startup_config, DISPLAYSCALE_STRATEGY::DISPLAYSCALE_STRATEGY_SCANLINES);
  else
    cfgSetDisplayScaleStrategy(gfxDrvCommon->rp_startup_config, DISPLAYSCALE_STRATEGY::DISPLAYSCALE_STRATEGY_SOLID);

  if (bStartup)
  {
    RP.SetScreenHeight(cfgGetScreenHeight(gfxDrvCommon->rp_startup_config));
    RP.SetScreenWidth(cfgGetScreenWidth(gfxDrvCommon->rp_startup_config));
  }

  lHeight = RP.GetScreenHeightAdjusted();
  lWidth = RP.GetScreenWidthAdjusted();
  lDisplayScale = RP.GetDisplayScale();

  cfgSetScreenHeight(gfxDrvCommon->rp_startup_config, lHeight);
  cfgSetScreenWidth(gfxDrvCommon->rp_startup_config, lWidth);

  drawSetInternalClip(draw_rect(92, 26, 468, 314));

  draw_rect output_clip(
      (RP.GetClippingOffsetLeft() / 2),
      RP.GetClippingOffsetTop(),
      ((RP.GetClippingOffsetLeft() + RP.GetScreenWidth()) / 2),
      (RP.GetClippingOffsetTop() + RP.GetScreenHeight()));

  cfgSetClipLeft(gfxDrvCommon->rp_startup_config, output_clip.left);
  cfgSetClipTop(gfxDrvCommon->rp_startup_config, output_clip.top);
  cfgSetClipRight(gfxDrvCommon->rp_startup_config, output_clip.right);
  cfgSetClipBottom(gfxDrvCommon->rp_startup_config, output_clip.bottom);

  drawSetOutputClip(output_clip);

  if (cfgGetScreenWindowed(gfxDrvCommon->rp_startup_config))
  {
    drawSetWindowedMode(cfgGetScreenWidth(gfxDrvCommon->rp_startup_config), cfgGetScreenHeight(gfxDrvCommon->rp_startup_config));
  }
  else
  {
    drawSetFullScreenMode(
        cfgGetScreenWidth(gfxDrvCommon->rp_startup_config),
        cfgGetScreenHeight(gfxDrvCommon->rp_startup_config),
        cfgGetScreenColorBits(gfxDrvCommon->rp_startup_config),
        cfgGetScreenRefresh(gfxDrvCommon->rp_startup_config));
  }
}

void RetroPlatform::SetWindowInstance(HINSTANCE hNewWindowInstance)
{
  hWindowInstance = hNewWindowInstance;
  _core.Log->AddLog("RetroPlatform::SetWindowInstance():  window instance set to %d.\n", hWindowInstance);
}

bool RetroPlatform::SendActivated(const bool bActive, const LPARAM lParam)
{
  bool bResult = RetroPlatform::SendMessageToHost(bActive ? RP_IPC_TO_HOST_ACTIVATED : RP_IPC_TO_HOST_DEACTIVATED, 0, lParam, nullptr, 0, &GuestInfo, nullptr);

  _core.Log->AddLog("RetroPlatform::SendActivated(): %s.\n", bResult ? "successful" : "failed");

  return bResult;
}

/** Notify the player that the user request to close the emulation session.
 *
 *  The player will examine if changes to the package were performed that
 *  require user feedback (media changed where undo is enabled, parameters
 *  like e.g. clipping were changed, ...).
 *  The user can choose what to commit and proceed with quitting, or cancel.
 *  The player can then either notify the emulator to quit via an IPC message
 *  RP_IPC_TO_GUEST_CLOSE, or do nothing and let the session continue.
 */
bool RetroPlatform::SendClose()
{
  bool bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_CLOSE, 0, 0, nullptr, 0, &GuestInfo, nullptr);

  _core.Log->AddLog("RetroPlatform::SendClose(): %s.\n", bResult ? "sucessful" : "failed");

  return bResult;
}

/** Send enable/disable messages to the RetroPlatform player.
 *
 * These are sent on WM_ENABLE messages.
 */
bool RetroPlatform::SendEnable(const bool bEnabled)
{
  LRESULT lResult;

  if (!bInitialized) return false;

  bool bResult = RetroPlatform::SendMessageToHost(bEnabled ? RP_IPC_TO_HOST_ENABLED : RP_IPC_TO_HOST_DISABLED, 0, 0, nullptr, 0, &GuestInfo, &lResult);

  _core.Log->AddLog("RetroPlatform::SendEnable() %s, result was %d.\n", bResult ? "successful" : "failed", lResult);

  return bResult;
}

/** Send list of features supported by the guest to the RetroPlatform host.
 *
 * An RP_IPC_TO_HOST_FEATURES message is sent to the host, with flags indicating
 * the features supported by the guest.
 * @return true if message was sent successfully, false otherwise.
 */
bool RetroPlatform::SendFeatures()
{
  LRESULT lResult;

  if (pConfig == nullptr)
  {
    _core.Log->AddLog("RetroPlatform::SendFeatures(): ERROR: config not initialzed.\n");
    return false;
  }

  DWORD dFeatureFlags = RP_FEATURE_POWERLED | RP_FEATURE_PAUSE;
  dFeatureFlags |= RP_FEATURE_TURBO_FLOPPY | RP_FEATURE_TURBO_CPU;
  dFeatureFlags |= RP_FEATURE_VOLUME | RP_FEATURE_DEVICEREADWRITE;
  dFeatureFlags |= RP_FEATURE_INPUTDEVICE_MOUSE | RP_FEATURE_INPUTDEVICE_JOYSTICK;
  dFeatureFlags |= RP_FEATURE_SCREEN1X;

  // features that are currently implemented only for DirectDraw
  if (pConfig->m_displaydriver == DISPLAYDRIVER::DISPLAYDRIVER_DIRECTDRAW)
  {
    dFeatureFlags |= RP_FEATURE_SCREEN2X | RP_FEATURE_SCREEN3X | RP_FEATURE_SCREEN4X;
    dFeatureFlags |= RP_FEATURE_SCANLINES;
    dFeatureFlags |= RP_FEATURE_SCREENCAPTURE;

#ifdef _DEBUG
    dFeatureFlags |= RP_FEATURE_FULLSCREEN;
    _core.Log->AddLog("RetroPlatform::SendFeatures(): Display driver is DirectDraw\n");
#endif
  }
  else if (pConfig->m_displaydriver == DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11)
  {
    dFeatureFlags |= RP_FEATURE_SCREEN2X | RP_FEATURE_SCREEN3X | RP_FEATURE_SCREEN4X;
    dFeatureFlags |= RP_FEATURE_SCANLINES;
    dFeatureFlags |= RP_FEATURE_SCREENCAPTURE;

#ifdef _DEBUG
    _core.Log->AddLog("RetroPlatform::SendFeatures(): Display driver is Direct3D 11\n");
#endif
  }
  else
    _core.Log->AddLog(
        "RetroPlatform::SendFeatures(): WARNING: unknown display "
        "driver type %u\n",
        pConfig->m_displaydriver);

  // currently missing features: RP_FEATURE_FULLSCREEN,
  // RP_FEATURE_STATE, RP_FEATURE_SCALING_SUBPIXEL, RP_FEATURE_SCALING_STRETCH
  // RP_FEATURE_INPUTDEVICE_GAMEPAD, RP_FEATURE_INPUTDEVICE_JOYPAD,
  // RP_FEATURE_INPUTDEVICE_ANALOGSTICK, RP_FEATURE_INPUTDEVICE_LIGHTPEN

  bool bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_FEATURES, dFeatureFlags, 0, nullptr, 0, &GuestInfo, &lResult);

  _core.Log->AddLog("RetroPlatform::SendFeatures() %s, result was %d.\n", bResult ? "successful" : "failed", lResult);

  return bResult;
}

/** Send list of enabled floppy drives to the RetroPlatform host.
 *
 * An RP_IPC_TO_HOST_DEVICES message is sent to the host, indicating the floppy
 * drives enabled in the guest. Must be called after the activation of the
 * config, and before sending the screen mode.
 * @return true if message was sent successfully, false otherwise.
 */
bool RetroPlatform::SendEnabledFloppyDrives()
{
  LRESULT lResult;

  DWORD dFeatureFlags = 0;
  for (int i = 0; i < 4; i++)
  {
#ifdef _DEBUG
    _core.Log->AddLog("RetroPlatform::SendEnabledFloppyDrives(): floppy drive %d is %s.\n", i, floppy[i].enabled ? "enabled" : "disabled");
#endif
    if (floppy[i].enabled) dFeatureFlags |= 1 << i;
  }

  bool bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_DEVICES, RP_DEVICECATEGORY_FLOPPY, dFeatureFlags, nullptr, 0, &GuestInfo, &lResult);

  _core.Log->AddLog("RetroPlatform::SendEnabledFloppyDrives() %s, lResult=%d.\n", bResult ? "successful" : "failed", lResult);

  return bResult;
}

/** Send list of enabled hard drives to the RetroPlatform host.
 *
 * An RP_IPC_TO_HOST_DEVICES message is sent to the host, indicating the hard
 * drives enabled in the guest. Must be called after the activation of the
 * config, and before sending the screen mode.
 * @return true if message was sent successfully, false otherwise.
 */
bool RetroPlatform::SendEnabledHardDrives()
{
  DWORD dFeatureFlags = 0;
  LRESULT lResult;

  _core.Log->AddLog("RetroPlatform::SendEnabledHardDrives(): %d hard drives are enabled.\n", cfgGetHardfileCount(pConfig));

  for (uint32_t i = 0; i < cfgGetHardfileCount(pConfig); i++)
    dFeatureFlags |= 1 << i;

  bool bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_DEVICES, RP_DEVICECATEGORY_HD, dFeatureFlags, nullptr, 0, &GuestInfo, &lResult);

  _core.Log->AddLog("RetroPlatform::SendEnabledHardDrives() %s, lResult=%d.\n", bResult ? "successful" : "failed", lResult);

  return bResult;
}

bool RetroPlatform::SendGameports(const uint32_t lNumGameports)
{
  LRESULT lResult;

  bool bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_DEVICES, RP_DEVICECATEGORY_INPUTPORT, (1 << lNumGameports) - 1, nullptr, 0, &GuestInfo, &lResult);

  _core.Log->AddLog("RetroPlatform::SendGameports() %s, lResult=%d.\n", bResult ? "successful" : "failed", lResult);

  return bResult;
}

/** Send a single input device to the RetroPlatform player.
 */
bool RetroPlatform::SendInputDevice(
    const DWORD dwHostInputType, const DWORD dwInputDeviceFeatures, const DWORD dwFlags, const WCHAR *szHostInputID, const WCHAR *szHostInputName)
{
  LRESULT lResult;
  char szHostInputNameA[CFG_FILENAME_LENGTH];
  struct RPInputDeviceDescription rpInputDevDesc;

  wcscpy(rpInputDevDesc.szHostInputID, szHostInputID);
  wcscpy(rpInputDevDesc.szHostInputName, szHostInputName);
  rpInputDevDesc.dwHostInputType = dwHostInputType;
  rpInputDevDesc.dwHostInputVendorID = 0;
  rpInputDevDesc.dwHostInputProductID = 0;
  rpInputDevDesc.dwInputDeviceFeatures = dwInputDeviceFeatures;
  rpInputDevDesc.dwFlags = dwFlags;

  wcstombs(szHostInputNameA, szHostInputName, CFG_FILENAME_LENGTH);

  bool bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_INPUTDEVICE, 0, 0, &rpInputDevDesc, sizeof rpInputDevDesc, &GuestInfo, &lResult);

#ifdef _DEBUG
  _core.Log->AddLog("RetroPlatform::SendInputDevice('%s') %s, lResult=%d.\n", szHostInputNameA, bResult ? "successful" : "failed", lResult);
#endif

  return bResult;
}

/** Send list of available input device options to the RetroPlatform player.
 *
 * The emulator is supposed to enumerate the Windows devices and identify
 * them via unique IDs; joysticks are sent after enumeration during
 * emulator session start, other devices are sent here
 */
bool RetroPlatform::SendInputDevices()
{
  bool bResult = true;

  // Windows mouse
  if (!RetroPlatform::SendInputDevice(
          RP_HOSTINPUT_MOUSE, RP_FEATURE_INPUTDEVICE_MOUSE | RP_FEATURE_INPUTDEVICE_LIGHTPEN, RP_HOSTINPUTFLAGS_MOUSE_SMART, L"GP_MOUSE0", L"Windows Mouse"))
  {
    bResult = false;
  }

  // report custom keyboard layout support
  if (!RetroPlatform::SendInputDevice(RP_HOSTINPUT_KEYBOARD, RP_FEATURE_INPUTDEVICE_JOYSTICK, 0, L"GP_JOYKEYCUSTOM", L"KeyboardCustom"))
  {
    bResult = false;
  }

  // report available joysticks
  RetroPlatform::EnumerateJoysticks();

  // end enumeration
  if (!RetroPlatform::SendInputDevice(RP_HOSTINPUT_END, 0, 0, L"RP_END", L"END"))
  {
    bResult = false;
  }

  _core.Log->AddLog("RetroPlatform::SendInputDevices() %s.\n", bResult ? "successful" : "failed");

  return bResult;
}

bool RetroPlatform::SendMouseCapture(const bool bActive)
{
  WPARAM wFlags = (WPARAM)0;

  if (!bInitialized) return false;

  if (bActive) wFlags |= RP_MOUSECAPTURE_CAPTURED;

  bool bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_MOUSECAPTURE, wFlags, 0, nullptr, 0, &GuestInfo, nullptr);

  _core.Log->AddLog("RetroPlatform::SendMouseCapture(): %s.\n", bResult ? "successful" : "failed");

  return bResult;
}

/** Send screen mode to the player.
 *
 * This step finalizes the transfer of guest features to the player and will
 * enable the emulation.
 */
bool RetroPlatform::SendScreenMode(HWND hWnd)
{
  RPScreenMode ScreenMode = {0};

  if (!bInitialized) return false;

  hGuestWindow = hWnd;
  RetroPlatform::DetermineScreenModeFromConfig(&ScreenMode, pConfig);

  bool bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_SCREENMODE, 0, 0, &ScreenMode, sizeof ScreenMode, &GuestInfo, nullptr);

  _core.Log->AddLog("RetroPlatform::SendScreenMode(): %s.\n", bResult ? "successful" : "failed");

  return bResult;
}

uint32_t RetroPlatform::GetEscapeKey()
{
  return lEscapeKey;
}

uint32_t RetroPlatform::GetEscapeKeyHoldTime()
{
  return lEscapeKeyHoldTime;
}

ULONGLONG RetroPlatform::GetEscapeKeyHeldSince()
{
  return lEscapeKeyHeldSince;
}

ULONGLONG RetroPlatform::GetEscapeKeySimulatedTargetTime()
{
  return lEscapeKeySimulatedTargetTime;
}

HWND RetroPlatform::GetParentWindowHandle()
{
  LRESULT lResult;

  if (!bInitialized) return nullptr;

  bool bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_PARENT, 0, 0, nullptr, 0, &GuestInfo, &lResult);

  if (bResult)
  {
    _core.Log->AddLog(
        "RetroPlatform::GetParentWindowHandle(): parent window handle "
        "returned was %u.\n",
        lResult);
    return (HWND)lResult;
  }
  else
    return nullptr;
}

HWND RetroPlatform::GetTopWindowHandle()
{
  LRESULT lResult;

  if (!bInitialized) return nullptr;

  bool bResult = RetroPlatform::SendMessageToHost(RP_IPC_TO_HOST_TOPWINDOW, 0, 0, nullptr, 0, &GuestInfo, &lResult);

  if (bResult)
  {
    _core.Log->AddLog(
        "RetroPlatform::GetTopWindowHandle(): top window handle "
        "returned was %u.\n",
        lResult);

    if (lResult == NULL) // older RetroPlatform versions don't use the top
      // window handle as emulator runs in a separate context
      return gfxDrvCommon->GetHWND();
    else
      return (HWND)lResult;
  }
  else
    return nullptr;
}

void RetroPlatform::Startup()
{
  pConfig = cfgManagerGetCurrentConfig(&cfg_manager);

  LRESULT lResult = RPInitializeGuest(&GuestInfo, hWindowInstance, szHostID, RetroPlatformHostMessageFunction, 0);

  if (SUCCEEDED(lResult))
  {
    bInitialized = true;

    RetroPlatform::GetHostVersion(&lMainVersion, &lRevision, &lBuild);

    _core.Log->AddLog("RetroPlatform::Startup(%s) successful. Host version: %d.%d.%d\n", szHostID, lMainVersion, lRevision, lBuild);

    RetroPlatform::SendFeatures();
  }
  else
    _core.Log->AddLog("RetroPlatform::Startup(%s) failed, error code %08x\n", szHostID, lResult);
}

/** Verifies that the prerequisites to start the emulation are available.
 *
 * Validates that the configuration contains a path to a Kickstart ROM, and that
 * the file can be opened successfully for reading.
 * @return true, when Kickstart ROM can be opened successfully for reading;
 * false otherwise
 */
bool RetroPlatform::CheckEmulationNecessities()
{
  if (strcmp(cfgGetKickImage(pConfig), "") != 0)
  {
    FILE *F = fopen(cfgGetKickImage(pConfig), "rb");
    if (F != nullptr)
    {
      fclose(F);
      return true;
    }
    return false;
  }
  else
    return false;
}

/** The main control function when operating in RetroPlatform headless mode.
 *
 * This function performs the start of the emulator session. On a reset event,
 * winDrvEmulationStart will exit without bRetroPlatformEmulatorQuit being set.
 */
void RetroPlatform::EnterHeadlessMode()
{
  if (RetroPlatform::CheckEmulationNecessities() == true)
  {
    cfgManagerSetCurrentConfig(&cfg_manager, pConfig);
    // check for manual or needed reset
    bool configurationRequiresReset = cfgManagerConfigurationActivate(&cfg_manager) == TRUE;
    fellowSetPreStartReset(fellowGetPreStartReset() || configurationRequiresReset);

    RetroPlatform::SendEnabledFloppyDrives();
    RetroPlatform::SendEnabledHardDrives();
    RetroPlatform::SendGameports(RETRO_PLATFORM_NUM_GAMEPORTS);
    RetroPlatform::SendInputDevices();

    while (!bEmulatorQuit)
    {
      RetroPlatform::SetEmulationState(true);
      winDrvEmulationStart();
      RetroPlatform::SetEmulationState(false);
    }
  }
  else
    MessageBox(nullptr, "Specified KickImage does not exist", "Configuration Error", 0);
}

void RetroPlatform::Shutdown()
{
  if (!bInitialized) return;

  RetroPlatform::SendScreenMode(nullptr);
  RetroPlatform::PostMessageToHost(RP_IPC_TO_HOST_CLOSED, 0, 0, &GuestInfo);
  RPUninitializeGuest(&GuestInfo);
  bInitialized = false;
}

void RetroPlatform::EmulationStart()
{
  RetroPlatform::SendScreenMode(gfxDrvCommon->GetHWND());
}

void RetroPlatform::EmulationStop()
{
}

RetroPlatform::RetroPlatform()
{
  GuestInfo = {nullptr};
}

RetroPlatform::~RetroPlatform()
{
}

#endif