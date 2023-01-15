/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/* Keyboard driver for Windows                                             */
/* Author: Marco Nova (novamarco@hotmail.com)                              */
/*         Torsten Enderling (carfesh@gmx.net) (Nov 2007 DirectX SDK fixes)*/
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

#include "fellow/os/windows/io/KeyboardDriver.h"
#include "fellow/chipset/Keycodes.h"
#include "fellow/chipset/Keyboard.h"
#include "fellow/api/Drivers.h"
#include "fellow/application/Gameport.h"
#include "fellow/os/windows/application/WindowsDriver.h"
#include "fellow/os/windows/io/Keyparser.h"
#include "fellow/api/Services.h"
#include "fellow/os/windows/graphics/GraphicsDriver.h"

#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#endif

using namespace fellow::api;

constexpr auto DINPUT_BUFFERSIZE = 256;

constexpr auto MAPPING_FILENAME = "mapping.key";

//=========================================
// Map symbolic key to a description string
//=========================================

const char *KeyboardDriver::_pc_symbol_to_string[PCSymbolCount] = {
    "NONE",
    "ESCAPE",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "PRINT_SCREEN",
    "SCROLL_LOCK",
    "PAUSE",
    "GRAVE",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "MINUS",
    "EQUALS",
    "BACKSPACE",
    "INSERT",
    "HOME",
    "PAGE_UP",
    "NUM_LOCK",
    "NUMPAD_DIVIDE",
    "NUMPAD_MULTIPLY",
    "NUMPAD_MINUS",
    "TAB",
    "Q",
    "W",
    "E",
    "R",
    "T",
    "Y",
    "U",
    "I",
    "O",
    "P",
    "LEFT_BRACKET",
    "RIGHT_BRACKET",
    "RETURN",
    "DELETE",
    "END",
    "PAGE_DOWN",
    "NUMPAD_7",
    "NUMPAD_8",
    "NUMPAD_9",
    "NUMPAD_PLUS",
    "CAPS_LOCK",
    "A",
    "S",
    "D",
    "F",
    "G",
    "H",
    "J",
    "K",
    "L",
    "SEMICOLON",
    "APOSTROPHE",
    "BACKSLASH",
    "NUMPAD_4",
    "NUMPAD_5",
    "NUMPAD_6",
    "LEFT_SHIFT",
    "NONAME1",
    "Z",
    "X",
    "C",
    "V",
    "B",
    "N",
    "M",
    "COMMA",
    "PERIOD",
    "SLASH",
    "RIGHT_SHIFT",
    "UP",
    "NUMPAD_1",
    "NUMPAD_2",
    "NUMPAD_3",
    "NUMPAD_ENTER",
    "LEFT_CTRL",
    "LEFT_WINDOWS",
    "LEFT_ALT",
    "SPACE",
    "RIGHT_ALT",
    "RIGHT_WINDOWS",
    "RIGHT_MENU",
    "RIGHT_CTRL",
    "LEFT",
    "DOWN",
    "RIGHT",
    "NUMPAD_0",
    "NUMPAD_DOT"};

const char *KeyboardDriver::_symbol_pretty_name[PCSymbolCount] = {"none",        "Escape",      "F1",
                                                                  "F2",          "F3",          "F4",
                                                                  "F5",          "F6",          "F7",
                                                                  "F8",          "F9",          "F10",
                                                                  "F11",         "F12",         "Print screen",
                                                                  "Scroll lock", "Pause",       "Grave",
                                                                  "1",           "2",           "3",
                                                                  "4",           "5",           "6",
                                                                  "7",           "8",           "9",
                                                                  "0",           "-",           "=",
                                                                  "Backspace",   "Insert",      "Home",
                                                                  "Page up",     "Num lock",    "/ (numpad)",
                                                                  "* (numpad)",  "- (numpad)",  "Tab",
                                                                  "Q",           "W",           "E",
                                                                  "R",           "T",           "Y",
                                                                  "U",           "I",           "O",
                                                                  "P",           "[",           "]",
                                                                  "Return",      "Delete",      "End",
                                                                  "Page down",   "7 (numpad)",  "8 (numpad)",
                                                                  "9 (numpad)",  "+ (numpad)",  "Caps lock",
                                                                  "A",           "S",           "D",
                                                                  "F",           "G",           "H",
                                                                  "J",           "K",           "L",
                                                                  ";",           "'",           "\\",
                                                                  "4 (numpad)",  "5 (numpad)",  "6 (numpad)",
                                                                  "Left shift",  "<",           "Z",
                                                                  "X",           "C",           "V",
                                                                  "B",           "N",           "M",
                                                                  ",",           ".",           "/",
                                                                  "Right shift", "Arrow up",    "1 (numpad)",
                                                                  "2 (numpad)",  "3 (numpad)",  "Enter (numpad)",
                                                                  "Left ctrl",   "Left win",    "Left alt",
                                                                  " ",           "Right alt",   "Right win",
                                                                  "Right menu",  "Right ctrl",  "Arrow left",
                                                                  "Arrow down",  "Arrow right", "0 (numpad)",
                                                                  ". (numpad)"};

//===================================================
// Map windows virtual key to intermediate key symbol
//===================================================

int KeyboardDriver::_symbol_to_dik[PCSymbolCount] = {
    0,
    DIK_ESCAPE,
    DIK_F1,
    DIK_F2,
    DIK_F3,
    DIK_F4,
    DIK_F5,
    DIK_F6,
    DIK_F7,
    DIK_F8,
    DIK_F9,
    DIK_F10,
    DIK_F11,
    DIK_F12,
    DIK_SYSRQ,
    DIK_SCROLL,
    0 /*DIK_PAUSE*/,
    DIK_GRAVE,
    DIK_1,
    DIK_2,
    DIK_3,
    DIK_4,
    DIK_5,
    DIK_6,
    DIK_7,
    DIK_8,
    DIK_9,
    DIK_0,
    DIK_MINUS,
    DIK_EQUALS,
    DIK_BACK,
    DIK_INSERT,
    DIK_HOME,
    DIK_PRIOR,
    DIK_NUMLOCK,
    DIK_DIVIDE,
    DIK_MULTIPLY,
    DIK_MINUS,
    DIK_TAB,
    DIK_Q,
    DIK_W,
    DIK_E,
    DIK_R,
    DIK_T,
    DIK_Y,
    DIK_U,
    DIK_I,
    DIK_O,
    DIK_P,
    DIK_LBRACKET,
    DIK_RBRACKET,
    DIK_RETURN,
    DIK_DELETE,
    DIK_END,
    DIK_NEXT,
    DIK_NUMPAD7,
    DIK_NUMPAD8,
    DIK_NUMPAD9,
    DIK_ADD,
    DIK_CAPITAL,
    DIK_A,
    DIK_S,
    DIK_D,
    DIK_F,
    DIK_G,
    DIK_H,
    DIK_J,
    DIK_K,
    DIK_L,
    DIK_SEMICOLON,
    DIK_APOSTROPHE,
    DIK_BACKSLASH,
    DIK_NUMPAD4,
    DIK_NUMPAD5,
    DIK_NUMPAD6,
    DIK_LSHIFT,
    0x56, /* Not defined */
    DIK_Z,
    DIK_X,
    DIK_C,
    DIK_V,
    DIK_B,
    DIK_N,
    DIK_M,
    DIK_COMMA,
    DIK_PERIOD,
    DIK_SLASH,
    DIK_RSHIFT,
    DIK_UP,
    DIK_NUMPAD1,
    DIK_NUMPAD2,
    DIK_NUMPAD3,
    DIK_NUMPADENTER,
    DIK_LCONTROL,
    DIK_LWIN,
    DIK_LMENU,
    DIK_SPACE,
    DIK_RMENU,
    DIK_RWIN,
    DIK_APPS,
    DIK_RCONTROL,
    DIK_LEFT,
    DIK_DOWN,
    DIK_RIGHT,
    DIK_NUMPAD0,
    DIK_DECIMAL};

//======================================
// Map symbolic pc key to amiga scancode
//======================================

UBY KeyboardDriver::_pc_symbol_to_amiga_scancode[PCSymbolCount] = {
    /* 0x00 */

    A_NONE,   /* PCK_NONE            */
    A_ESCAPE, /* PCK_ESCAPE          */
    A_F1,     /* PCK_F1              */
    A_F2,     /* PCK_F2              */

    A_F3, /* PCK_F3              */
    A_F4, /* PCK_F4              */
    A_F5, /* PCK_F5              */
    A_F6, /* PCK_F6              */

    A_F7,  /* PCK_F7              */
    A_F8,  /* PCK_F8              */
    A_F9,  /* PCK_F9              */
    A_F10, /* PCK_F10             */

    A_NONE,       /* PCK_F11             */
    A_NONE,       /* PCK_F12             */
    A_NONE,       /* PCK_PRINT_SCREEN    */
    A_BACKSLASH2, /* PCK_SCROLL_LOCK     */

    /* 0x10 */

    A_NUMPAD_RIGHT_BRACKET, /* PCK_PAUSE           */
    A_GRAVE,                /* PCK_GRAVE           */
    A_1,                    /* PCK_1               */
    A_2,                    /* PCK_2               */

    A_3, /* PCK_3               */
    A_4, /* PCK_4               */
    A_5, /* PCK_5               */
    A_6, /* PCK_6               */

    A_7, /* PCK_7               */
    A_8, /* PCK_8               */
    A_9, /* PCK_9               */
    A_0, /* PCK_0               */

    A_MINUS,     /* PCK_MINUS           */
    A_EQUALS,    /* PCK_EQUALS          */
    A_BACKSPACE, /* PCK_BACKSPACE       */
    A_HELP,      /* PCK_INSERT          */

    /* 0x20 */

    A_NONE,                /* PCK_HOME            */
    A_RIGHT_AMIGA,         /* PCK_PAGE_UP         */
    A_NUMPAD_LEFT_BRACKET, /* PCK_NUM_LOCK        */
    A_NUMPAD_DIVIDE,       /* PCK_NUMPAD_DIVIDE   */

    A_NUMPAD_MULTIPLY, /* PCK_NUMPAD_MULTIPLY */
    A_NUMPAD_MINUS,    /* PCK_NUMPAD_MINUS    */
    A_TAB,             /* PCK_TAB             */
    A_Q,               /* PCK_Q               */

    A_W, /* PCK_W               */
    A_E, /* PCK_E               */
    A_R, /* PCK_R               */
    A_T, /* PCK_T               */

    A_Y, /* PCK_Y               */
    A_U, /* PCK_U               */
    A_I, /* PCK_I               */
    A_O, /* PCK_O               */

    /* 0x30 */

    A_P,             /* PCK_P               */
    A_LEFT_BRACKET,  /* PCK_LBRACKET        */
    A_RIGHT_BRACKET, /* PCK_RBRACKET        */
    A_RETURN,        /* PCK_RETURN          */

    A_DELETE,     /* PCK_DELETE          */
    A_NONE,       /* PCK_END             */
    A_LEFT_AMIGA, /* PCK_PAGE_DOWN       */
    A_NUMPAD_7,   /* PCK_NUMPAD_7        */

    A_NUMPAD_8,    /* PCK_NUMPAD_8        */
    A_NUMPAD_9,    /* PCK_NUMPAD_9        */
    A_NUMPAD_PLUS, /* PCK_NUMPAD_PLUS     */
    A_CAPS_LOCK,   /* PCK_CAPS_LOCK       */

    A_A, /* PCK_A               */
    A_S, /* PCK_S               */
    A_D, /* PCK_D               */
    A_F, /* PCK_F               */

    /* 0x40 */

    A_G, /* PCK_G               */
    A_H, /* PCK_H               */
    A_J, /* PCK_J               */
    A_K, /* PCK_K               */

    A_L,          /* PCK_L               */
    A_SEMICOLON,  /* PCK_SEMICOLON       */
    A_APOSTROPHE, /* PCK_APOSTROPHE      */
    A_BACKSLASH,  /* PCK_BACKSLASH       */

    A_NUMPAD_4,   /* PCK_NUMPAD_4        */
    A_NUMPAD_5,   /* PCK_NUMPAD_5        */
    A_NUMPAD_6,   /* PCK_NUMPAD_6        */
    A_LEFT_SHIFT, /* PCK_LEFT_SHIFT      */

    A_LESS_THAN, /* PCK_NONAME1         */
    A_Z,         /* PCK_Z               */
    A_X,         /* PCK_X               */
    A_C,         /* PCK_C               */

    /* 0x50 */

    A_V, /* PCK_V               */
    A_B, /* PCK_B               */
    A_N, /* PCK_N               */
    A_M, /* PCK_M               */

    A_COMMA,       /* PCK_COMMA           */
    A_PERIOD,      /* PCK_PERIOD          */
    A_SLASH,       /* PCK_SLASH           */
    A_RIGHT_SHIFT, /* PCK_RIGHT_SHIFT     */

    A_UP,       /* PCK_UP              */
    A_NUMPAD_1, /* PCK_NUMPAD_1        */
    A_NUMPAD_2, /* PCK_NUMPAD_2        */
    A_NUMPAD_3, /* PCK_NUMPAD_3        */

    A_NUMPAD_ENTER, /* PCK_NUMPAD_ENTER    */
    A_LEFT_AMIGA,   /* PCK_LEFT_CTRL       */
    A_LEFT_AMIGA,   /* PCK_LEFT_WINDOWS    */
    A_LEFT_ALT,     /* PCK_LEFT_ALT        */

    /* 0x60 */

    A_SPACE,       /* PCK_SPACE           */
    A_RIGHT_ALT,   /* PCK_RIGHT_ALT       */
    A_RIGHT_AMIGA, /* PCK_RIGHT_WINDOWS   */
    A_NONE,        /* PCK_START_MENU      */

    A_CTRL,  /* PCK_RIGHT_CTRL      */
    A_LEFT,  /* PCK_LEFT            */
    A_DOWN,  /* PCK_DOWN            */
    A_RIGHT, /* PCK_RIGHT           */

    A_NUMPAD_0,  /* PCK_NUMPAD_0        */
    A_NUMPAD_DOT /* PCK_NUMPAD_DOT      */
};

constexpr unsigned int JOYKEY_LEFT = 0;
constexpr unsigned int JOYKEY_RIGHT = 1;
constexpr unsigned int JOYKEY_UP = 2;
constexpr unsigned int JOYKEY_DOWN = 3;
constexpr unsigned int JOYKEY_FIRE0 = 4;
constexpr unsigned int JOYKEY_FIRE1 = 5;
constexpr unsigned int JOYKEY_AUTOFIRE0 = 6;
constexpr unsigned int JOYKEY_AUTOFIRE1 = 7;

void KeyboardDriver::ClearPressedKeys()
{
  memset(_prevkeys, 0, sizeof(_prevkeys));
  memset(_keys, 0, sizeof(_keys));
}

const char *KeyboardDriver::DInputErrorString(HRESULT hResult)
{
  switch (hResult)
  {
    case DI_OK: return "The operation completed successfully.";
    case DI_BUFFEROVERFLOW: return "The device buffer overflowed and some input was lost.";
    case DI_POLLEDDEVICE: return "The device is a polled device.";
    case DIERR_ACQUIRED: return "The operation cannot be performed while the device is acquired.";
    case DIERR_ALREADYINITIALIZED: return "This object is already initialized.";
    case DIERR_BADDRIVERVER: return "The object could not be created due to an incompatible driver version or mismatched or incomplete driver components.";
    case DIERR_BETADIRECTINPUTVERSION: return "The application was written for an unsupported prerelease version of DirectInput.";
    case DIERR_DEVICENOTREG: return "The device or device instance is not registered with DirectInput.";
    case DIERR_GENERIC: return "An undetermined error occurred inside the DirectInput subsystem.";
    case DIERR_HANDLEEXISTS: return "The device already has an event notification associated with it.";
    case DIERR_INPUTLOST: return "Access to the input device has been lost. It must be re-acquired.";
    case DIERR_INVALIDPARAM:
      return "An invalid parameter was passed to the returning function, or the object was not in a state that permitted the function to be called.";
    case DIERR_NOAGGREGATION: return "This object does not support aggregation.";
    case DIERR_NOINTERFACE: return "The specified interface is not supported by the object.";
    case DIERR_NOTACQUIRED: return "The operation cannot be performed unless the device is acquired.";
    case DIERR_NOTINITIALIZED: return "This object has not been initialized.";
    case DIERR_OBJECTNOTFOUND: return "The requested object does not exist.";
    case DIERR_OLDDIRECTINPUTVERSION: return "The application requires a newer version of DirectInput.";
    case DIERR_OUTOFMEMORY: return "The DirectInput subsystem couldn't allocate sufficient memory to complete the caller's request.";
    case DIERR_UNSUPPORTED: return "The function called is not supported at this time.";
    case E_PENDING: return "Data is not yet available.";
    case E_POINTER: return "Invalid pointer.";
  }
  return "Not a DirectInput Error";
}

const char *KeyboardDriver::DInputUnaquireReturnValueString(HRESULT hResult)
{
  switch (hResult)
  {
    case DI_OK: return "The operation completed successfully.";
    case DI_NOEFFECT: return "The device was not in an acquired state.";
  }
  return "Not a known Unacquire() DirectInput return value.";
}

void KeyboardDriver::DInputFailure(const char *header, HRESULT err)
{
  Service->Log.AddLog("%s %s\n", header, DInputErrorString(err));
}

void KeyboardDriver::DInputUnacquireFailure(const char *header, HRESULT err)
{
  Service->Log.AddLog("%s %s\n", header, DInputUnaquireReturnValueString(err));
}

bool KeyboardDriver::DInputSetCooperativeLevel()
{
  HWND hwnd = ((GraphicsDriver *)Driver->Graphics)->GetHWND();
  HRESULT res = IDirectInputDevice_SetCooperativeLevel(_lpDID, hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
  if (res != DI_OK)
  {
    DInputFailure("KeyboardDriver::DInputSetCooperativeLevel():", res);
    return false;
  }
  return true;
}

void KeyboardDriver::DInputAcquireFailure(const char *header, HRESULT err)
{
  if (err == DI_NOEFFECT)
  {
    Service->Log.AddLog("%s %s\n", header, "The device was already in an acquired state.");
  }
  else
  {
    DInputFailure(header, err);
  }
}

#ifdef RETRO_PLATFORM

/// the end-of-frame handler for the keyboard driver is called only in RetroPlatform mode
/// it's purpose is to relay simulated escape key presses to the Amiga, as well as to
/// escape devices when the escape key has been held longer than the configured interval

void KeyboardDriver::EOFHandler()
{
  ULONGLONG t = 0;

  t = RP.GetEscapeKeyHeldSince();

  if (t && (RP.GetTime() - RP.GetEscapeKeyHeldSince()) > RP.GetEscapeKeyHoldTime())
  {
    Service->Log.AddLog("RetroPlatform: Escape key held longer than hold time, releasing devices...\n");
#ifndef FELLOW_SUPPORT_RP_API_VERSION_71
    RP.PostEscaped();
#endif
    RP.SetEscapeKeyHeld(false);
  }

  t = RP.GetEscapeKeySimulatedTargetTime();

  if (t)
  {
    ULONGLONG tCurrentTime = RP.GetTime();

    if (t < tCurrentTime)
    {
      UBY a_code = _pc_symbol_to_amiga_scancode[RP.GetEscapeKey()];

      Service->Log.AddLog("RetroPlatform escape key simulation interval ended.\n");
      RP.SetEscapeKeySimulatedTargetTime(0);
      _keyboard->KeyAdd(a_code | 0x80); // release escape key
    }
  }
}

#endif

void KeyboardDriver::DInputUnacquire()
{
  if (_lpDID == nullptr)
  {
    return;
  }

  HRESULT res = IDirectInputDevice_Unacquire(_lpDID);
  if (res != DI_OK)
  {
    // Should only "fail" if device is not acquired, it is not an error.
    DInputUnacquireFailure("KeyboardDriver::DInputUnacquire():", res);
  }
}

void KeyboardDriver::DInputAcquire()
{
  if (_lpDID == nullptr)
  {
    return;
  }

  HRESULT res = IDirectInputDevice_Acquire(_lpDID);
  if (res != DI_OK)
  {
    DInputAcquireFailure("KeyboardDriver::DInputAcquire():", res);
  }
}

void KeyboardDriver::DInputRelease()
{
  if (_lpDID != nullptr)
  {
    IDirectInputDevice_Release(_lpDID);
    _lpDID = nullptr;
  }
  if (_di_event != nullptr)
  {
    CloseHandle(_di_event);
    _di_event = nullptr;
  }
  if (_lpDI != nullptr)
  {
    IDirectInput_Release(_lpDI);
    _lpDI = nullptr;
  }
}

bool KeyboardDriver::DInputInitialize()
{
  DIPROPDWORD dipdw = {
      {
          sizeof(DIPROPDWORD),  // diph.dwSize
          sizeof(DIPROPHEADER), // diph.dwHeaderSize
          0,                    // diph.dwObj
          DIPH_DEVICE,          // diph.dwHow
      },
      DINPUT_BUFFERSIZE // dwData
  };

  // Create Direct Input object

  _lpDI = nullptr;
  _lpDID = nullptr;
  _di_event = nullptr;
  _initializationFailed = false;
  HRESULT res = DirectInput8Create(win_drv_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&_lpDI, nullptr);
  if (res != DI_OK)
  {
    DInputFailure("KeyboardDriver::DInputInitialize(): DirectInput8Create()", res);
    _initializationFailed = true;
    DInputRelease();
    return false;
  }

  // Create Direct Input 1 keyboard device

  res = IDirectInput_CreateDevice(_lpDI, GUID_SysKeyboard, &_lpDID, NULL);
  if (res != DI_OK)
  {
    DInputFailure("KeyboardDriver::DInputInitialize(): CreateDevice()", res);
    _initializationFailed = true;
    DInputRelease();
    return false;
  }

  // Set data format for mouse device

  res = IDirectInputDevice_SetDataFormat(_lpDID, &c_dfDIKeyboard);
  if (res != DI_OK)
  {
    DInputFailure("KeyboardDriver::DInputInitialize(): SetDataFormat()", res);
    _initializationFailed = true;
    DInputRelease();
    return false;
  }

  // Set cooperative level

  if (!DInputSetCooperativeLevel())
  {
    _initializationFailed = true;
    DInputRelease();
    return false;
  }

  // Create event for notification

  _di_event = CreateEvent(nullptr, 0, 0, nullptr);
  if (_di_event == nullptr)
  {
    Service->Log.AddLog("KeyboardDriver::DInputInitialize(): CreateEvent() failed\n");
    _initializationFailed = true;
    DInputRelease();
    return false;
  }

  // Set property for buffered data
  res = IDirectInputDevice_SetProperty(_lpDID, DIPROP_BUFFERSIZE, &dipdw.diph);
  if (res != DI_OK)
  {
    DInputFailure("KeyboardDriver::DInputInitialize(): SetProperty()", res);
    _initializationFailed = true;
    DInputRelease();
    return false;
  }

  // Set event notification
  res = IDirectInputDevice_SetEventNotification(_lpDID, _di_event);
  if (res != DI_OK)
  {
    DInputFailure("KeyboardDriver::DInputInitialize(): SetEventNotification()", res);
    _initializationFailed = true;
    DInputRelease();
    return false;
  }
  return true;
}

void KeyboardDriver::StateHasChanged(bool active)
{
  _active = active;
  if (_active)
  {
    DInputAcquire();
  }
  else
  {
    DInputUnacquire();
    ClearPressedKeys();
  }
}

kbd_drv_pc_symbol KeyboardDriver::ToSymbolicKey(unsigned int scancode) const
{
  return _dik_to_symbol[scancode];
}

int KeyboardDriver::Map(kbd_drv_pc_symbol sym) const
{
  return _symbol_to_dik[sym];
}

bool KeyboardDriver::WasPressed(kbd_drv_pc_symbol sym) const
{
  return _prevkeys[Map(sym)];
}

bool KeyboardDriver::IsPressed(kbd_drv_pc_symbol sym) const
{
  return _keys[Map(sym)];
}

bool KeyboardDriver::Released(kbd_drv_pc_symbol sym) const
{
  return !IsPressed(sym) && WasPressed(sym);
}

bool KeyboardDriver::Pressed(kbd_drv_pc_symbol sym) const
{
  return IsPressed(sym) && !WasPressed(sym);
}

//==================================================
// Keyboard keypress emulator control event checker
// Returns TRUE if the key generated an event
// which means it must not be passed to the emulator
//==================================================

bool KeyboardDriver::EventChecker(kbd_drv_pc_symbol symbol_key)
{
  bool eventIssued = false;

  for (;;)
  {

    if (_capture)
    {

#ifdef _DEBUG
      Service->Log.AddLog("Key captured: %s\n", GetPCSymbolDescription(symbol_key));
#endif

      _captured_key = symbol_key;
      return true;
    }

    if (Released(PCK_PAGE_DOWN))
    {
      if (IsPressed(PCK_HOME))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_RESOLUTION_NEXT);
        eventIssued = true;
        break;
      }
      else
      {
        if (IsPressed(PCK_END))
        {
          _keyboard->EventEOFAdd(kbd_event::EVENT_SCALEY_NEXT);
          eventIssued = true;
          break;
        }
      }
    }
    if (Released(PCK_PAGE_UP))
    {
      if (IsPressed(PCK_HOME))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_RESOLUTION_PREV);
        eventIssued = true;
        break;
      }
      else
      {
        if (IsPressed(PCK_END))
        {
          _keyboard->EventEOFAdd(kbd_event::EVENT_SCALEY_PREV);
          eventIssued = true;
          break;
        }
      }
    }

    if (IsPressed(PCK_RIGHT_WINDOWS) || IsPressed(PCK_START_MENU))
    {
      if (IsPressed(PCK_LEFT_WINDOWS))
      {
        if (IsPressed(PCK_LEFT_CTRL))
        {
          _keyboard->EventEOFAdd(kbd_event::EVENT_HARD_RESET);
          eventIssued = true;
          break;
        }
      }
    }

    if (IsPressed(PCK_PRINT_SCREEN))
    {
      _keyboard->EventEOFAdd(kbd_event::EVENT_BMP_DUMP);
      eventIssued = true;
      break;
    }

#ifdef RETRO_PLATFORM
    if (RP.GetHeadlessMode())
    {
      if (IsPressed(PCK_LEFT_ALT))
      {
        if (IsPressed(PCK_F4))
        {
          _keyboard->EventEOFAdd(kbd_event::EVENT_EXIT);
          eventIssued = true;
          break;
        }
      }

      if (!RP.GetEmulationPaused())
      {
        if (Pressed((kbd_drv_pc_symbol)RP.GetEscapeKey()))
        {
          RP.SetEscapeKeyHeld(true);
          return true;
        }
        if (Released((kbd_drv_pc_symbol)RP.GetEscapeKey()))
        {
          ULONGLONG t = RP.SetEscapeKeyHeld(false);

          if ((t != 0) && (t < RP.GetEscapeKeyHoldTime()))
          {
            UBY a_code = _pc_symbol_to_amiga_scancode[RP.GetEscapeKey()];

            Service->Log.AddLog("RetroPlatform escape key held shorter than escape interval, simulate key being pressed for %u milliseconds...\n", t);
            RP.SetEscapeKeySimulatedTargetTime(RP.GetTime() + RP.GetEscapeKeyHoldTime());
            _keyboard->KeyAdd(a_code); // hold escape key
            return true;
          }
          else
          {
            return true;
          }
        }
      }
#ifndef FELLOW_SUPPORT_RP_API_VERSION_71
      else
      {
        if (Pressed(RP.GetEscapeKey())) RP.PostEscaped();
      }
#endif
    }
#endif

#ifdef RETRO_PLATFORM
    if (!RP.GetHeadlessMode())
#endif
      if (Released(PCK_F11))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_EXIT);
        eventIssued = true;
        break;
      }

#ifdef RETRO_PLATFORM
    if (!RP.GetHeadlessMode())
#endif
      if (Released(PCK_F12))
      {
        Driver->Mouse->ToggleFocus();
        Driver->Joystick->ToggleFocus();
        break;
      }

    if (IsPressed(PCK_HOME))
    {
      if (Released(PCK_F1))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_INSERT_DF0);
        eventIssued = true;
        break;
      }
      if (Released(PCK_F2))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_INSERT_DF1);
        eventIssued = true;
        break;
      }
      if (Released(PCK_F3))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_INSERT_DF2);
        eventIssued = true;
        break;
      }
      if (Released(PCK_F4))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_INSERT_DF3);
        eventIssued = true;
        break;
      }
      if (Released(PCK_F5))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_DF1_INTO_DF0);
        eventIssued = true;
        break;
      }
      if (Released(PCK_F6))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_DF2_INTO_DF0);
        eventIssued = true;
        break;
      }
      if (Released(PCK_F7))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_DF3_INTO_DF0);
        eventIssued = true;
        break;
      }
      if (Released(PCK_F8))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_DEBUG_TOGGLE_RENDERER);
        eventIssued = true;
        break;
      }
      if (Released(PCK_F9))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_DEBUG_TOGGLE_LOGGING);
        eventIssued = true;
        break;
      }
    }
    else if (IsPressed(PCK_END))
    {
      if (Released(PCK_F1))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_EJECT_DF0);
        eventIssued = true;
        break;
      }
      if (Released(PCK_F2))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_EJECT_DF1);
        eventIssued = true;
        break;
      }
      if (Released(PCK_F3))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_EJECT_DF2);
        eventIssued = true;
        break;
      }
      if (Released(PCK_F4))
      {
        _keyboard->EventEOFAdd(kbd_event::EVENT_EJECT_DF3);
        eventIssued = true;
        break;
      }
    }

    // Check joysticks replacements
    // New here: Must remember last value to decide if a change has happened

    for (unsigned int port = 0; port < 2; port++)
    {
      for (unsigned int setting = 0; setting < 2; setting++)
      {
        if (_joykey_enabled[port][setting])
        {
          for (unsigned int direction = 0; direction < MAX_JOYKEY_VALUE; direction++)
          {
            if (symbol_key == _joykey[setting][direction])
            {
              if (Pressed(symbol_key) || Released(symbol_key))
              {
                _keyboard->EventEOLAdd(_joykey_event[port][Pressed(symbol_key)][direction]);
                eventIssued = true;
              }
            }
          }
        }
      }
    }
    break;
  }

  return eventIssued;
}

//===================================
// Handle one specific keycode change
//===================================

void KeyboardDriver::Keypress(ULO keycode, BOOL pressed)
{
  kbd_drv_pc_symbol symbolic_key = ToSymbolicKey(keycode);
  bool keycode_pressed = pressed;
  bool keycode_was_pressed = _prevkeys[keycode];

  // Bad hack - stuck key after ALT-TAB switch to another application dialog
  // Unfortunately Windows does not have an easy way to tell the app that the task switcher has been activated,
  // and only some of the user keypresses involved are reported leaving the emulator with stuck keys on return to the emulator.
  // In particular, missing release of LEFT-ALT is problematic.

  // left-alt already pressed, and now TAB pressed as well
  if (!_kbd_in_task_switcher && _keys[Map(PCK_LEFT_ALT)] && keycode == Map(PCK_TAB) && pressed)
  {
    Service->Log.AddLog("KeyboardDriver::Keypress(): ALT-TAB start detected\n");

    // Apart from the fake LEFT-ALT release event, full-screen does not need additional handling.
    _kbd_in_task_switcher = ((GraphicsDriver *)Driver->Graphics)->IsHostBufferWindowed();

    // Don't pass this TAB press along to emulation, pass left-ALT release instead
    keycode = Map(PCK_LEFT_ALT);
    symbolic_key = ToSymbolicKey(keycode);
    pressed = false;
    keycode_pressed = pressed;
    keycode_was_pressed = _prevkeys[keycode];

#ifdef _DEBUG
    Service->Log.AddLog(
        "Keypress TAB converted to %s %s due to ALT-TAB started\n", GetPCSymbolDescription(symbolic_key), pressed ? "pressed" : "released");
#endif
  }
  else if (_kbd_in_task_switcher && symbolic_key == PCK_LEFT_ALT && !pressed)
  {
    Service->Log.AddLog("KeyboardDriver::Keypress(): ALT-TAB end detected\n");
    _kbd_in_task_switcher = false;

#ifdef _DEBUG
    Service->Log.AddLog("Keypress LEFT-ALT released ignored due to ALT-TAB ending\n");
#endif
    return;
  }
  else if (_kbd_in_task_switcher)
  {
    return;
  }

  _keys[keycode] = pressed;

  if ((!keycode_pressed) && keycode_was_pressed)
  {
    // If key is not eaten by a Fellow "event", add it to Amiga kbd queue
    if (!EventChecker(symbolic_key))
    {
      UBY a_code = _pc_symbol_to_amiga_scancode[symbolic_key];
      _keyboard->KeyAdd(a_code | 0x80);
    }
  }
  else if (keycode_pressed && !keycode_was_pressed)
  {
    // If key is not eaten by a Fellow "event", add it to Amiga kbd queue
    if (!EventChecker(symbolic_key))
    {
      UBY a_code = _pc_symbol_to_amiga_scancode[symbolic_key];
      _keyboard->KeyAdd(a_code);
    }
  }
  _prevkeys[keycode] = pressed;
}

//=======================================
// Handle one specific RAW keycode change
//=======================================

void KeyboardDriver::KeypressRaw(ULO lRawKeyCode, bool pressed)
{
#ifdef FELLOW_DELAY_RP_KEYBOARD_INPUT
  BOOLE keycode_was_pressed = prevkeys[lRawKeyCode];
#endif

#ifdef FELLOW_DELAY_RP_KEYBOARD_INPUT
  keys[lRawKeyCode] = pressed;

  if ((!pressed) && keycode_was_pressed)
  {
    kbdKeyAdd(lRawKeyCode | 0x80);
    Sleep(10);
  }
  else if (pressed && !keycode_was_pressed)
  {
    kbdKeyAdd(lRawKeyCode);
  }

  prevkeys[lRawKeyCode] = pressed;
#else
  if (!pressed)
  {
    _keyboard->KeyAdd(lRawKeyCode | 0x80);
  }
  else
  {
    _keyboard->KeyAdd(lRawKeyCode);
  }
#endif
}

void KeyboardDriver::BufferOverflowHandler()
{
}

void KeyboardDriver::KeypressHandler()
{
  if (!_active)
  {
    return;
  }

  DIDEVICEOBJECTDATA rgod[DINPUT_BUFFERSIZE];
  DWORD itemcount = DINPUT_BUFFERSIZE;
  HRESULT res;

  do
  {
    res = IDirectInputDevice_GetDeviceData(_lpDID, sizeof(DIDEVICEOBJECTDATA), rgod, &itemcount, 0);
    if (res == DIERR_INPUTLOST)
    {
      DInputAcquire();
    }
  } while (res == DIERR_INPUTLOST);

  if ((res != DI_OK) && (res != DI_BUFFEROVERFLOW))
  {
    DInputFailure("KeyboardDriver::KeypressHandler(): GetDeviceData()", res);
  }
  else
  {
    for (unsigned int i = 0; i < itemcount; i++)
    {
      Keypress(rgod[i].dwOfs, (rgod[i].dwData & 0x80));
    }
  }
}

//================================================
// Return string describing the given symbolic key
//================================================

//=========================================
// Get index (kbd_drv_pc_symbol) of the key
//=========================================

kbd_drv_pc_symbol KeyboardDriver::GetPCSymbolFromDescription(const char *pcSymbolDescription)
{
  for (unsigned int i = 0; i < PCSymbolCount; i++)
  {
    if (stricmp(pcSymbolDescription, _pc_symbol_to_string[i]) == 0)
    {
      return (kbd_drv_pc_symbol)i;
    }
  }

  Service->Log.AddLogDebug(
      "KeyboardDriver::GetPCSymbolFromDescription() - No match for '%s' was found in the pc symbolic key mapping table", pcSymbolDescription);
  return PCK_NONE;
}

const char *KeyboardDriver::GetPCSymbolDescription(kbd_drv_pc_symbol symbolickey)
{
  unsigned int pcSymbolIndex = (unsigned int)symbolickey;

  if (symbolickey >= PCSymbolCount)
  {
    Service->Log.AddLogDebug("KeyboardDriver::GetPCSymbolDescription() - %u is not a valid PC symbolic keycode", pcSymbolIndex);
    return "";
  }

  return _pc_symbol_to_string[symbolickey];
}

const char *KeyboardDriver::GetPCSymbolPrettyDescription(kbd_drv_pc_symbol symbolickey)
{
  unsigned int pcSymbolIndex = (unsigned int)symbolickey;

  if (symbolickey >= PCSymbolCount)
  {
    Service->Log.AddLogDebug("KeyboardDriver::GetPCSymbolPrettyDescription() - %u is not a valid pc symbolic keycode", pcSymbolIndex);
    return "";
  }

  return _symbol_pretty_name[pcSymbolIndex];
}

kbd_drv_pc_symbol KeyboardDriver::GetPCSymbolFromDIK(int dikkey)
{
  for (unsigned int j = 0; j < PCSymbolCount; j++)
  {
    if (dikkey == _symbol_to_dik[j])
    {
      return (kbd_drv_pc_symbol)j;
    }
  }

  Service->Log.AddLogDebug("KeyboardDriver::GetPCSymbolFromDIK() - DIK %d is not mapped to a pc symbolic keycode", dikkey);
  return PCK_NONE;
}

const char *KeyboardDriver::GetDIKDescription(int dikkey)
{
  for (unsigned int j = 0; j < PCSymbolCount; j++)
  {
    if (dikkey == _symbol_to_dik[j])
    {
      return GetPCSymbolDescription((kbd_drv_pc_symbol)j);
    }
  }

  Service->Log.AddLogDebug("KeyboardDriver::GetDIKDescription() - DIK %d is not mapped to a pc symbolic keycode", dikkey);
  return "UNKNOWN";
}

//============================================================
// Set joystick replacement for a given joystick and direction
//============================================================

void KeyboardDriver::JoystickReplacementSet(kbd_event event, kbd_drv_pc_symbol symbolickey)
{
  switch (event)
  {
    case kbd_event::EVENT_JOY0_UP_ACTIVE: _joykey[0][JOYKEY_UP] = symbolickey; break;
    case kbd_event::EVENT_JOY0_DOWN_ACTIVE: _joykey[0][JOYKEY_DOWN] = symbolickey; break;
    case kbd_event::EVENT_JOY0_LEFT_ACTIVE: _joykey[0][JOYKEY_LEFT] = symbolickey; break;
    case kbd_event::EVENT_JOY0_RIGHT_ACTIVE: _joykey[0][JOYKEY_RIGHT] = symbolickey; break;
    case kbd_event::EVENT_JOY0_FIRE0_ACTIVE: _joykey[0][JOYKEY_FIRE0] = symbolickey; break;
    case kbd_event::EVENT_JOY0_FIRE1_ACTIVE: _joykey[0][JOYKEY_FIRE1] = symbolickey; break;
    case kbd_event::EVENT_JOY0_AUTOFIRE0_ACTIVE: _joykey[0][JOYKEY_AUTOFIRE0] = symbolickey; break;
    case kbd_event::EVENT_JOY0_AUTOFIRE1_ACTIVE: _joykey[0][JOYKEY_AUTOFIRE1] = symbolickey; break;
    case kbd_event::EVENT_JOY1_UP_ACTIVE: _joykey[1][JOYKEY_UP] = symbolickey; break;
    case kbd_event::EVENT_JOY1_DOWN_ACTIVE: _joykey[1][JOYKEY_DOWN] = symbolickey; break;
    case kbd_event::EVENT_JOY1_LEFT_ACTIVE: _joykey[1][JOYKEY_LEFT] = symbolickey; break;
    case kbd_event::EVENT_JOY1_RIGHT_ACTIVE: _joykey[1][JOYKEY_RIGHT] = symbolickey; break;
    case kbd_event::EVENT_JOY1_FIRE0_ACTIVE: _joykey[1][JOYKEY_FIRE0] = symbolickey; break;
    case kbd_event::EVENT_JOY1_FIRE1_ACTIVE: _joykey[1][JOYKEY_FIRE1] = symbolickey; break;
    case kbd_event::EVENT_JOY1_AUTOFIRE0_ACTIVE: _joykey[1][JOYKEY_AUTOFIRE0] = symbolickey; break;
    case kbd_event::EVENT_JOY1_AUTOFIRE1_ACTIVE: _joykey[1][JOYKEY_AUTOFIRE1] = symbolickey; break;
  }
}

//============================================================
// Get joystick replacement for a given joystick and direction
//============================================================

kbd_drv_pc_symbol KeyboardDriver::JoystickReplacementGet(kbd_event event)
{
  switch (event)
  {
    case kbd_event::EVENT_JOY0_UP_ACTIVE: return _joykey[0][JOYKEY_UP];
    case kbd_event::EVENT_JOY0_DOWN_ACTIVE: return _joykey[0][JOYKEY_DOWN];
    case kbd_event::EVENT_JOY0_LEFT_ACTIVE: return _joykey[0][JOYKEY_LEFT];
    case kbd_event::EVENT_JOY0_RIGHT_ACTIVE: return _joykey[0][JOYKEY_RIGHT];
    case kbd_event::EVENT_JOY0_FIRE0_ACTIVE: return _joykey[0][JOYKEY_FIRE0];
    case kbd_event::EVENT_JOY0_FIRE1_ACTIVE: return _joykey[0][JOYKEY_FIRE1];
    case kbd_event::EVENT_JOY0_AUTOFIRE0_ACTIVE: return _joykey[0][JOYKEY_AUTOFIRE0];
    case kbd_event::EVENT_JOY0_AUTOFIRE1_ACTIVE: return _joykey[0][JOYKEY_AUTOFIRE1];
    case kbd_event::EVENT_JOY1_UP_ACTIVE: return _joykey[1][JOYKEY_UP];
    case kbd_event::EVENT_JOY1_DOWN_ACTIVE: return _joykey[1][JOYKEY_DOWN];
    case kbd_event::EVENT_JOY1_LEFT_ACTIVE: return _joykey[1][JOYKEY_LEFT];
    case kbd_event::EVENT_JOY1_RIGHT_ACTIVE: return _joykey[1][JOYKEY_RIGHT];
    case kbd_event::EVENT_JOY1_FIRE0_ACTIVE: return _joykey[1][JOYKEY_FIRE0];
    case kbd_event::EVENT_JOY1_FIRE1_ACTIVE: return _joykey[1][JOYKEY_FIRE1];
    case kbd_event::EVENT_JOY1_AUTOFIRE0_ACTIVE: return _joykey[1][JOYKEY_AUTOFIRE0];
    case kbd_event::EVENT_JOY1_AUTOFIRE1_ACTIVE: return _joykey[1][JOYKEY_AUTOFIRE1];
  }
  return kbd_drv_pc_symbol::PCK_NONE;
}

void KeyboardDriver::InitializeDIKToSymbolKeyTable()
{
  for (int i = 0; i < PCK_LAST_KEY; i++)
  {
    _dik_to_symbol[i] = PCK_NONE;
  }

  _dik_to_symbol[DIK_ESCAPE] = PCK_ESCAPE; /* First row */
  _dik_to_symbol[DIK_F1] = PCK_F1;
  _dik_to_symbol[DIK_F2] = PCK_F2;
  _dik_to_symbol[DIK_F3] = PCK_F3;
  _dik_to_symbol[DIK_F4] = PCK_F4;
  _dik_to_symbol[DIK_F5] = PCK_F5;
  _dik_to_symbol[DIK_F6] = PCK_F6;
  _dik_to_symbol[DIK_F7] = PCK_F7;
  _dik_to_symbol[DIK_F8] = PCK_F8;
  _dik_to_symbol[DIK_F9] = PCK_F9;
  _dik_to_symbol[DIK_F10] = PCK_F10;
  _dik_to_symbol[DIK_F11] = PCK_F11;
  _dik_to_symbol[DIK_F12] = PCK_F12;
  _dik_to_symbol[DIK_SYSRQ] = PCK_PRINT_SCREEN;
  _dik_to_symbol[DIK_SCROLL] = PCK_SCROLL_LOCK;
  /*_dik_to_symbol[ DIK_PAUSE           ] = PCK_PAUSE; */
  _dik_to_symbol[DIK_GRAVE] = PCK_GRAVE; /* Second row */
  _dik_to_symbol[DIK_1] = PCK_1;
  _dik_to_symbol[DIK_2] = PCK_2;
  _dik_to_symbol[DIK_3] = PCK_3;
  _dik_to_symbol[DIK_4] = PCK_4;
  _dik_to_symbol[DIK_5] = PCK_5;
  _dik_to_symbol[DIK_6] = PCK_6;
  _dik_to_symbol[DIK_7] = PCK_7;
  _dik_to_symbol[DIK_8] = PCK_8;
  _dik_to_symbol[DIK_9] = PCK_9;
  _dik_to_symbol[DIK_0] = PCK_0;
  _dik_to_symbol[DIK_MINUS] = PCK_MINUS;
  _dik_to_symbol[DIK_EQUALS] = PCK_EQUALS;
  _dik_to_symbol[DIK_BACK] = PCK_BACKSPACE;
  _dik_to_symbol[DIK_INSERT] = PCK_INSERT;
  _dik_to_symbol[DIK_HOME] = PCK_HOME;
  _dik_to_symbol[DIK_PRIOR] = PCK_PAGE_UP;
  _dik_to_symbol[DIK_NUMLOCK] = PCK_NUM_LOCK;
  _dik_to_symbol[DIK_DIVIDE] = PCK_NUMPAD_DIVIDE;
  _dik_to_symbol[DIK_MULTIPLY] = PCK_NUMPAD_MULTIPLY;
  _dik_to_symbol[DIK_SUBTRACT] = PCK_NUMPAD_MINUS;
  _dik_to_symbol[DIK_TAB] = PCK_TAB; /* Third row */
  _dik_to_symbol[DIK_Q] = PCK_Q;
  _dik_to_symbol[DIK_W] = PCK_W;
  _dik_to_symbol[DIK_E] = PCK_E;
  _dik_to_symbol[DIK_R] = PCK_R;
  _dik_to_symbol[DIK_T] = PCK_T;
  _dik_to_symbol[DIK_Y] = PCK_Y;
  _dik_to_symbol[DIK_U] = PCK_U;
  _dik_to_symbol[DIK_I] = PCK_I;
  _dik_to_symbol[DIK_O] = PCK_O;
  _dik_to_symbol[DIK_P] = PCK_P;
  _dik_to_symbol[DIK_LBRACKET] = PCK_LBRACKET;
  _dik_to_symbol[DIK_RBRACKET] = PCK_RBRACKET;
  _dik_to_symbol[DIK_RETURN] = PCK_RETURN;
  _dik_to_symbol[DIK_DELETE] = PCK_DELETE;
  _dik_to_symbol[DIK_END] = PCK_END;
  _dik_to_symbol[DIK_NEXT] = PCK_PAGE_DOWN;
  _dik_to_symbol[DIK_NUMPAD7] = PCK_NUMPAD_7;
  _dik_to_symbol[DIK_NUMPAD8] = PCK_NUMPAD_8;
  _dik_to_symbol[DIK_NUMPAD9] = PCK_NUMPAD_9;
  _dik_to_symbol[DIK_ADD] = PCK_NUMPAD_PLUS;

  _dik_to_symbol[DIK_CAPITAL] = PCK_CAPS_LOCK; /* Fourth row */
  _dik_to_symbol[DIK_A] = PCK_A;
  _dik_to_symbol[DIK_S] = PCK_S;
  _dik_to_symbol[DIK_D] = PCK_D;
  _dik_to_symbol[DIK_F] = PCK_F;
  _dik_to_symbol[DIK_G] = PCK_G;
  _dik_to_symbol[DIK_H] = PCK_H;
  _dik_to_symbol[DIK_J] = PCK_J;
  _dik_to_symbol[DIK_K] = PCK_K;
  _dik_to_symbol[DIK_L] = PCK_L;
  _dik_to_symbol[DIK_SEMICOLON] = PCK_SEMICOLON;
  _dik_to_symbol[DIK_APOSTROPHE] = PCK_APOSTROPHE;
  _dik_to_symbol[DIK_BACKSLASH] = PCK_BACKSLASH;
  _dik_to_symbol[DIK_NUMPAD4] = PCK_NUMPAD_4;
  _dik_to_symbol[DIK_NUMPAD5] = PCK_NUMPAD_5;
  _dik_to_symbol[DIK_NUMPAD6] = PCK_NUMPAD_6;

  _dik_to_symbol[DIK_LSHIFT] = PCK_LEFT_SHIFT; /* Fifth row */
  _dik_to_symbol[0x56] = PCK_NONAME1;
  _dik_to_symbol[DIK_Z] = PCK_Z;
  _dik_to_symbol[DIK_X] = PCK_X;
  _dik_to_symbol[DIK_C] = PCK_C;
  _dik_to_symbol[DIK_V] = PCK_V;
  _dik_to_symbol[DIK_B] = PCK_B;
  _dik_to_symbol[DIK_N] = PCK_N;
  _dik_to_symbol[DIK_M] = PCK_M;
  _dik_to_symbol[DIK_COMMA] = PCK_COMMA;
  _dik_to_symbol[DIK_PERIOD] = PCK_PERIOD;
  _dik_to_symbol[DIK_SLASH] = PCK_SLASH;
  _dik_to_symbol[DIK_RSHIFT] = PCK_RIGHT_SHIFT;
  _dik_to_symbol[DIK_UP] = PCK_UP;
  _dik_to_symbol[DIK_NUMPAD1] = PCK_NUMPAD_1;
  _dik_to_symbol[DIK_NUMPAD2] = PCK_NUMPAD_2;
  _dik_to_symbol[DIK_NUMPAD3] = PCK_NUMPAD_3;
  _dik_to_symbol[DIK_NUMPADENTER] = PCK_NUMPAD_ENTER;

  _dik_to_symbol[DIK_LCONTROL] = PCK_LEFT_CTRL; /* Sixth row */
  _dik_to_symbol[DIK_LWIN] = PCK_LEFT_WINDOWS;
  _dik_to_symbol[DIK_LMENU] = PCK_LEFT_ALT;
  _dik_to_symbol[DIK_SPACE] = PCK_SPACE;
  _dik_to_symbol[DIK_RMENU] = PCK_RIGHT_ALT;
  _dik_to_symbol[DIK_RWIN] = PCK_RIGHT_WINDOWS;
  _dik_to_symbol[DIK_APPS] = PCK_START_MENU;
  _dik_to_symbol[DIK_RCONTROL] = PCK_RIGHT_CTRL;
  _dik_to_symbol[DIK_LEFT] = PCK_LEFT;
  _dik_to_symbol[DIK_DOWN] = PCK_DOWN;
  _dik_to_symbol[DIK_RIGHT] = PCK_RIGHT;
  _dik_to_symbol[DIK_NUMPAD0] = PCK_NUMPAD_0;
  _dik_to_symbol[DIK_DECIMAL] = PCK_NUMPAD_DOT;
}

void KeyboardDriver::SetJoyKeyEnabled(ULO lGameport, ULO lSetting, bool bEnabled)
{
  _joykey_enabled[lGameport][lSetting] = bEnabled;
}

HANDLE KeyboardDriver::GetDirectInputEvent() const
{
  return _di_event;
}

void KeyboardDriver::HardReset()
{
}

void KeyboardDriver::EmulationStart()
{
  for (unsigned int port = 0; port < 2; port++)
  {
    _joykey_enabled[port][0] = (gameport_input[port] == gameport_inputs::GP_JOYKEY0);
    _joykey_enabled[port][1] = (gameport_input[port] == gameport_inputs::GP_JOYKEY1);
  }

  ClearPressedKeys();
  DInputInitialize();
  _kbd_in_task_switcher = false;
}

void KeyboardDriver::EmulationStop()
{
  DInputRelease();
}

bool KeyboardDriver::GetInitializationFailed()
{
  return false;
}

void KeyboardDriver::Initialize(Keyboard *keyboard)
{
  _keyboard = keyboard;

  _joykey_event[0][0][JOYKEY_UP] = kbd_event::EVENT_JOY0_UP_INACTIVE;
  _joykey_event[0][1][JOYKEY_UP] = kbd_event::EVENT_JOY0_UP_ACTIVE;
  _joykey_event[1][0][JOYKEY_UP] = kbd_event::EVENT_JOY1_UP_INACTIVE;
  _joykey_event[1][1][JOYKEY_UP] = kbd_event::EVENT_JOY1_UP_ACTIVE;
  _joykey_event[0][0][JOYKEY_DOWN] = kbd_event::EVENT_JOY0_DOWN_INACTIVE;
  _joykey_event[0][1][JOYKEY_DOWN] = kbd_event::EVENT_JOY0_DOWN_ACTIVE;
  _joykey_event[1][0][JOYKEY_DOWN] = kbd_event::EVENT_JOY1_DOWN_INACTIVE;
  _joykey_event[1][1][JOYKEY_DOWN] = kbd_event::EVENT_JOY1_DOWN_ACTIVE;
  _joykey_event[0][0][JOYKEY_LEFT] = kbd_event::EVENT_JOY0_LEFT_INACTIVE;
  _joykey_event[0][1][JOYKEY_LEFT] = kbd_event::EVENT_JOY0_LEFT_ACTIVE;
  _joykey_event[1][0][JOYKEY_LEFT] = kbd_event::EVENT_JOY1_LEFT_INACTIVE;
  _joykey_event[1][1][JOYKEY_LEFT] = kbd_event::EVENT_JOY1_LEFT_ACTIVE;
  _joykey_event[0][0][JOYKEY_RIGHT] = kbd_event::EVENT_JOY0_RIGHT_INACTIVE;
  _joykey_event[0][1][JOYKEY_RIGHT] = kbd_event::EVENT_JOY0_RIGHT_ACTIVE;
  _joykey_event[1][0][JOYKEY_RIGHT] = kbd_event::EVENT_JOY1_RIGHT_INACTIVE;
  _joykey_event[1][1][JOYKEY_RIGHT] = kbd_event::EVENT_JOY1_RIGHT_ACTIVE;
  _joykey_event[0][0][JOYKEY_FIRE0] = kbd_event::EVENT_JOY0_FIRE0_INACTIVE;
  _joykey_event[0][1][JOYKEY_FIRE0] = kbd_event::EVENT_JOY0_FIRE0_ACTIVE;
  _joykey_event[1][0][JOYKEY_FIRE0] = kbd_event::EVENT_JOY1_FIRE0_INACTIVE;
  _joykey_event[1][1][JOYKEY_FIRE0] = kbd_event::EVENT_JOY1_FIRE0_ACTIVE;
  _joykey_event[0][0][JOYKEY_FIRE1] = kbd_event::EVENT_JOY0_FIRE1_INACTIVE;
  _joykey_event[0][1][JOYKEY_FIRE1] = kbd_event::EVENT_JOY0_FIRE1_ACTIVE;
  _joykey_event[1][0][JOYKEY_FIRE1] = kbd_event::EVENT_JOY1_FIRE1_INACTIVE;
  _joykey_event[1][1][JOYKEY_FIRE1] = kbd_event::EVENT_JOY1_FIRE1_ACTIVE;
  _joykey_event[0][0][JOYKEY_AUTOFIRE0] = kbd_event::EVENT_JOY0_AUTOFIRE0_INACTIVE;
  _joykey_event[0][1][JOYKEY_AUTOFIRE0] = kbd_event::EVENT_JOY0_AUTOFIRE0_ACTIVE;
  _joykey_event[1][0][JOYKEY_AUTOFIRE0] = kbd_event::EVENT_JOY1_AUTOFIRE0_INACTIVE;
  _joykey_event[1][1][JOYKEY_AUTOFIRE0] = kbd_event::EVENT_JOY1_AUTOFIRE0_ACTIVE;
  _joykey_event[0][0][JOYKEY_AUTOFIRE1] = kbd_event::EVENT_JOY0_AUTOFIRE1_INACTIVE;
  _joykey_event[0][1][JOYKEY_AUTOFIRE1] = kbd_event::EVENT_JOY0_AUTOFIRE1_ACTIVE;
  _joykey_event[1][0][JOYKEY_AUTOFIRE1] = kbd_event::EVENT_JOY1_AUTOFIRE1_INACTIVE;
  _joykey_event[1][1][JOYKEY_AUTOFIRE1] = kbd_event::EVENT_JOY1_AUTOFIRE1_ACTIVE;

  _joykey[0][JOYKEY_UP] = PCK_UP;
  _joykey[0][JOYKEY_DOWN] = PCK_DOWN;
  _joykey[0][JOYKEY_LEFT] = PCK_LEFT;
  _joykey[0][JOYKEY_RIGHT] = PCK_RIGHT;
  _joykey[0][JOYKEY_FIRE0] = PCK_RIGHT_CTRL;
  _joykey[0][JOYKEY_FIRE1] = PCK_RIGHT_ALT;
  _joykey[0][JOYKEY_AUTOFIRE0] = PCK_O;
  _joykey[0][JOYKEY_AUTOFIRE1] = PCK_P;
  _joykey[1][JOYKEY_UP] = PCK_R;
  _joykey[1][JOYKEY_DOWN] = PCK_F;
  _joykey[1][JOYKEY_LEFT] = PCK_D;
  _joykey[1][JOYKEY_RIGHT] = PCK_G;
  _joykey[1][JOYKEY_FIRE0] = PCK_LEFT_CTRL;
  _joykey[1][JOYKEY_FIRE1] = PCK_LEFT_ALT;
  _joykey[1][JOYKEY_AUTOFIRE0] = PCK_A;
  _joykey[1][JOYKEY_AUTOFIRE1] = PCK_S;

  for (unsigned int port = 0; port < 2; port++)
  {
    for (unsigned int setting = 0; setting < 2; setting++)
    {
      _joykey_enabled[port][setting] = false;
    }
  }

  _capture = false;
  _captured_key = PCK_NONE;

  InitializeDIKToSymbolKeyTable();

  Service->Fileops.GetGenericFileName(_mapping_filename, "WinFellow", MAPPING_FILENAME);

  _prs_rewrite_mapping_file = prsReadFile(_mapping_filename, _pc_symbol_to_amiga_scancode, _joykey);

  _active = false;
  _lpDI = nullptr;
  _lpDID = nullptr;
}

void KeyboardDriver::Release()
{
  if (_prs_rewrite_mapping_file)
  {
    prsWriteFile(_mapping_filename, _pc_symbol_to_amiga_scancode, _joykey);
  }
}

KeyboardDriver::KeyboardDriver() : IKeyboardDriver()
{
}

KeyboardDriver::~KeyboardDriver() = default;
