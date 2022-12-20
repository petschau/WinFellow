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
#include "fellow/chipset/Kbd.h"
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

/*===========================================================================*/
/* Map symbolic key to a description string                                  */
/*===========================================================================*/

const char *kbd_drv_pc_symbol_to_string[106] = {
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

const char *symbol_pretty_name[106] = {"none",        "Escape",      "F1",
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

/*===========================================================================*/
/* Map windows virtual key to intermediate key symbol                        */
/*===========================================================================*/

kbd_drv_pc_symbol kbddrv_DIK_to_symbol[MAX_KEYS];

int symbol_to_DIK_kbddrv[kbd_drv_pc_symbol::PCK_LAST_KEY] = {
    0,           DIK_ESCAPE,  DIK_F1,          DIK_F2,          DIK_F3,        DIK_F4,         DIK_F5,        DIK_F6,      DIK_F7,      DIK_F8,      DIK_F9,       DIK_F10,      DIK_F11,
    DIK_F12,     DIK_SYSRQ,   DIK_SCROLL,      0 /*DIK_PAUSE*/, DIK_GRAVE,     DIK_1,          DIK_2,         DIK_3,       DIK_4,       DIK_5,       DIK_6,        DIK_7,        DIK_8,
    DIK_9,       DIK_0,       DIK_MINUS,       DIK_EQUALS,      DIK_BACK,      DIK_INSERT,     DIK_HOME,      DIK_PRIOR,   DIK_NUMLOCK, DIK_DIVIDE,  DIK_MULTIPLY, DIK_MINUS,    DIK_TAB,
    DIK_Q,       DIK_W,       DIK_E,           DIK_R,           DIK_T,         DIK_Y,          DIK_U,         DIK_I,       DIK_O,       DIK_P,       DIK_LBRACKET, DIK_RBRACKET, DIK_RETURN,
    DIK_DELETE,  DIK_END,     DIK_NEXT,        DIK_NUMPAD7,     DIK_NUMPAD8,   DIK_NUMPAD9,    DIK_ADD,       DIK_CAPITAL, DIK_A,       DIK_S,       DIK_D,        DIK_F,        DIK_G,
    DIK_H,       DIK_J,       DIK_K,           DIK_L,           DIK_SEMICOLON, DIK_APOSTROPHE, DIK_BACKSLASH, DIK_NUMPAD4, DIK_NUMPAD5, DIK_NUMPAD6, DIK_LSHIFT,   0x56, /* Not defined */
    DIK_Z,       DIK_X,       DIK_C,           DIK_V,           DIK_B,         DIK_N,          DIK_M,         DIK_COMMA,   DIK_PERIOD,  DIK_SLASH,   DIK_RSHIFT,   DIK_UP,       DIK_NUMPAD1,
    DIK_NUMPAD2, DIK_NUMPAD3, DIK_NUMPADENTER, DIK_LCONTROL,    DIK_LWIN,      DIK_LMENU,      DIK_SPACE,     DIK_RMENU,   DIK_RWIN,    DIK_APPS,    DIK_RCONTROL, DIK_LEFT,     DIK_DOWN,
    DIK_RIGHT,   DIK_NUMPAD0, DIK_DECIMAL};

/*===========================================================================*/
/* Map symbolic pc key to amiga scancode                                     */
/*===========================================================================*/

UBY kbd_drv_pc_symbol_to_amiga_scancode[106] = {
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

/*===========================================================================*/
/* Keyboard handler state data                                               */
/*===========================================================================*/

BOOLE kbd_drv_home_pressed;
BOOLE kbd_drv_end_pressed;

constexpr unsigned int JOYKEY_LEFT = 0;
constexpr unsigned int JOYKEY_RIGHT = 1;
constexpr unsigned int JOYKEY_UP = 2;
constexpr unsigned int JOYKEY_DOWN = 3;
constexpr unsigned int JOYKEY_FIRE0 = 4;
constexpr unsigned int JOYKEY_FIRE1 = 5;
constexpr unsigned int JOYKEY_AUTOFIRE0 = 6;
constexpr unsigned int JOYKEY_AUTOFIRE1 = 7;

kbd_drv_pc_symbol kbd_drv_joykey[2][MAX_JOYKEY_VALUE];  /* Keys for joykeys 0 and 1 */
BOOLE kbd_drv_joykey_enabled[2][2];                     /* For each port, the enabled joykeys */
kbd_event kbd_drv_joykey_event[2][2][MAX_JOYKEY_VALUE]; /* Event ID for each joykey [port][pressed][direction] */
volatile kbd_drv_pc_symbol kbd_drv_captured_key;
BOOLE kbd_drv_capture;

void KeyboardDriver::ClearPressedKeys()
{
  kbd_drv_home_pressed = FALSE;
  kbd_drv_end_pressed = FALSE;
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
    case DIERR_INVALIDPARAM: return "An invalid parameter was passed to the returning function, or the object was not in a state that permitted the function to be called.";
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
  HWND hwnd = ((GraphicsDriver &)Driver->Graphics).GetHWND();
  HRESULT res = IDirectInputDevice_SetCooperativeLevel(_lpDID, hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
  if (res != DI_OK)
  {
    DInputFailure("kbdDrvDInputSetCooperativeLevel():", res);
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
      UBY a_code = kbd_drv_pc_symbol_to_amiga_scancode[RP.GetEscapeKey()];

      Service->Log.AddLog("RetroPlatform escape key simulation interval ended.\n");
      RP.SetEscapeKeySimulatedTargetTime(0);
      kbdKeyAdd(a_code | 0x80); // release escape key
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
    DInputUnacquireFailure("kbdDrvDInputUnacquire():", res);
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
    DInputAcquireFailure("kbdDrvDInputAcquire():", res);
  }
}

void KeyboardDriver::DInputRelease()
{
  if (_lpDID != nullptr)
  {
    IDirectInputDevice_Release(_lpDID);
    _lpDID = nullptr;
  }
  if (_DIevent != nullptr)
  {
    CloseHandle(_DIevent);
    _DIevent = nullptr;
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
          sizeof(DIPROPDWORD),  /* diph.dwSize */
          sizeof(DIPROPHEADER), /* diph.dwHeaderSize */
          0,                    /* diph.dwObj */
          DIPH_DEVICE,          /* diph.dwHow */
      },
      DINPUT_BUFFERSIZE /* dwData */
  };

  /* Create Direct Input object */

  _lpDI = nullptr;
  _lpDID = nullptr;
  _DIevent = nullptr;
  _initialization_failed = false;
  HRESULT res = DirectInput8Create(win_drv_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&_lpDI, nullptr);
  if (res != DI_OK)
  {
    DInputFailure("kbdDrvDInputInitialize(): DirectInput8Create()", res);
    _initialization_failed = true;
    DInputRelease();
    return false;
  }

  /* Create Direct Input 1 keyboard device */

  res = IDirectInput_CreateDevice(_lpDI, GUID_SysKeyboard, &_lpDID, NULL);
  if (res != DI_OK)
  {
    DInputFailure("kbdDrvDInputInitialize(): CreateDevice()", res);
    _initialization_failed = true;
    DInputRelease();
    return false;
  }

  /* Set data format for mouse device */

  res = IDirectInputDevice_SetDataFormat(_lpDID, &c_dfDIKeyboard);
  if (res != DI_OK)
  {
    DInputFailure("kbdDrvDInputInitialize(): SetDataFormat()", res);
    _initialization_failed = true;
    DInputRelease();
    return false;
  }

  /* Set cooperative level */

  if (!DInputSetCooperativeLevel())
  {
    _initialization_failed = true;
    DInputRelease();
    return false;
  }

  /* Create event for notification */

  _DIevent = CreateEvent(nullptr, 0, 0, nullptr);
  if (_DIevent == nullptr)
  {
    Service->Log.AddLog("kbdDrvDInputInitialize(): CreateEvent() failed\n");
    _initialization_failed = true;
    DInputRelease();
    return false;
  }

  /* Set property for buffered data */
  res = IDirectInputDevice_SetProperty(_lpDID, DIPROP_BUFFERSIZE, &dipdw.diph);
  if (res != DI_OK)
  {
    DInputFailure("kbdDrvDInputInitialize(): SetProperty()", res);
    _initialization_failed = true;
    DInputRelease();
    return false;
  }

  /* Set event notification */
  res = IDirectInputDevice_SetEventNotification(_lpDID, _DIevent);
  if (res != DI_OK)
  {
    DInputFailure("kbdDrvDInputInitialize(): SetEventNotification()", res);
    _initialization_failed = true;
    DInputRelease();
    return false;
  }
  return true;
}

void KeyboardDriver::StateHasChanged(BOOLE active)
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

#define map(sym) symbol_to_DIK_kbddrv[(sym)]
#define symbolickey(scancode) kbddrv_DIK_to_symbol[scancode]
#define ispressed(sym) (_keys[map(sym)])
#define waspressed(sym) (_prevkeys[map(sym)])
#define issue_event(the_event)                                                                                                                                                                         \
  {                                                                                                                                                                                                    \
    kbdEventEOFAdd((the_event));                                                                                                                                                                       \
    break;                                                                                                                                                                                             \
  }
#define released(sym) (!ispressed((sym)) && waspressed((sym)))
#define pressed(sym) ((ispressed((sym))) && (!waspressed((sym))))

/*===========================================================================*/
/* Keyboard keypress emulator control event checker                          */
/* Returns TRUE if the key generated an event                                */
/* which means it must not be passed to the emulator                         */
/*===========================================================================*/

BOOLE KeyboardDriver::EventChecker(kbd_drv_pc_symbol symbol_key)
{
  ULO eol_evpos = kbd_state.eventsEOL.inpos;
  ULO eof_evpos = kbd_state.eventsEOF.inpos;

  for (;;)
  {

    if (kbd_drv_capture)
    {

#ifdef _DEBUG
      Service->Log.AddLog("Key captured: %s\n", KeyString(symbol_key));
#endif

      kbd_drv_captured_key = symbol_key;
      return TRUE;
    }

    if (released(PCK_PAGE_DOWN))
    {
      if (ispressed(PCK_HOME))
      {
        issue_event(kbd_event::EVENT_RESOLUTION_NEXT);
      }
      else
      {
        if (ispressed(PCK_END))
        {
          issue_event(kbd_event::EVENT_SCALEY_NEXT);
        }
      }
    }
    if (released(PCK_PAGE_UP))
    {
      if (ispressed(PCK_HOME))
      {
        issue_event(kbd_event::EVENT_RESOLUTION_PREV);
      }
      else
      {
        if (ispressed(PCK_END))
        {
          issue_event(kbd_event::EVENT_SCALEY_PREV);
        }
      }
    }

    if (ispressed(PCK_RIGHT_WINDOWS) || ispressed(PCK_START_MENU))
    {
      if (ispressed(PCK_LEFT_WINDOWS))
      {
        if (ispressed(PCK_LEFT_CTRL))
        {
          issue_event(kbd_event::EVENT_HARD_RESET);
        }
      }
    }

    if (ispressed(PCK_PRINT_SCREEN))
    {
      issue_event(kbd_event::EVENT_BMP_DUMP);
    }

#ifdef RETRO_PLATFORM
    if (RP.GetHeadlessMode())
    {
      if (ispressed(PCK_LEFT_ALT))
      {
        if (ispressed(PCK_F4))
        {
          issue_event(kbd_event::EVENT_EXIT);
        }
      }

      if (!RP.GetEmulationPaused())
      {
        if (pressed(RP.GetEscapeKey()))
        {
          RP.SetEscapeKeyHeld(true);
          return TRUE;
        }
        if (released(RP.GetEscapeKey()))
        {
          ULONGLONG t = RP.SetEscapeKeyHeld(false);

          if ((t != 0) && (t < RP.GetEscapeKeyHoldTime()))
          {
            UBY a_code = kbd_drv_pc_symbol_to_amiga_scancode[RP.GetEscapeKey()];

            Service->Log.AddLog("RetroPlatform escape key held shorter than escape interval, simulate key being pressed for %u milliseconds...\n", t);
            RP.SetEscapeKeySimulatedTargetTime(RP.GetTime() + RP.GetEscapeKeyHoldTime());
            kbdKeyAdd(a_code); // hold escape key
            return TRUE;
          }
          else
            return TRUE;
        }
      }
#ifndef FELLOW_SUPPORT_RP_API_VERSION_71
      else
      {
        if (pressed(RP.GetEscapeKey())) RP.PostEscaped();
      }
#endif
    }
#endif

#ifdef RETRO_PLATFORM
    if (!RP.GetHeadlessMode())
#endif
      if (released(PCK_F11))
      {
        issue_event(kbd_event::EVENT_EXIT);
      }

#ifdef RETRO_PLATFORM
    if (!RP.GetHeadlessMode())
#endif
      if (released(PCK_F12))
      {
        Driver->Mouse.ToggleFocus();
        Driver->Joystick.ToggleFocus();
        break;
      }

    if (ispressed(PCK_HOME))
    {
      if (released(PCK_F1)) issue_event(kbd_event::EVENT_INSERT_DF0);
      if (released(PCK_F2)) issue_event(kbd_event::EVENT_INSERT_DF1);
      if (released(PCK_F3)) issue_event(kbd_event::EVENT_INSERT_DF2);
      if (released(PCK_F4)) issue_event(kbd_event::EVENT_INSERT_DF3);
      if (released(PCK_F5)) issue_event(kbd_event::EVENT_DF1_INTO_DF0);
      if (released(PCK_F6)) issue_event(kbd_event::EVENT_DF2_INTO_DF0);
      if (released(PCK_F7)) issue_event(kbd_event::EVENT_DF3_INTO_DF0);
      if (released(PCK_F8)) issue_event(kbd_event::EVENT_DEBUG_TOGGLE_RENDERER);
      if (released(PCK_F9)) issue_event(kbd_event::EVENT_DEBUG_TOGGLE_LOGGING);
    }
    else if (ispressed(PCK_END))
    {
      if (released(PCK_F1)) issue_event(kbd_event::EVENT_EJECT_DF0);
      if (released(PCK_F2)) issue_event(kbd_event::EVENT_EJECT_DF1);
      if (released(PCK_F3)) issue_event(kbd_event::EVENT_EJECT_DF2);
      if (released(PCK_F4)) issue_event(kbd_event::EVENT_EJECT_DF3);
    }

    // Check joysticks replacements
    // New here: Must remember last value to decide if a change has happened

    for (ULO port = 0; port < 2; port++)
    {
      for (ULO setting = 0; setting < 2; setting++)
      {
        if (kbd_drv_joykey_enabled[port][setting])
        {
          for (int direction = 0; direction < MAX_JOYKEY_VALUE; direction++)
          {
            if (symbol_key == kbd_drv_joykey[setting][direction])
            {
              if (pressed(symbol_key) || released(symbol_key))
              {
                kbdEventEOLAdd(kbd_drv_joykey_event[port][pressed(symbol_key)][direction]);
              }
            }
          }
        }
      }
    }
    break;
  }

  return (eol_evpos != kbd_state.eventsEOL.inpos) || (eof_evpos != kbd_state.eventsEOF.inpos);
}

/*===========================================================================*/
/* Handle one specific keycode change                                        */
/*===========================================================================*/

void KeyboardDriver::Keypress(ULO keycode, BOOL pressed)
{
  kbd_drv_pc_symbol symbolic_key = symbolickey(keycode);
  BOOLE keycode_pressed = pressed;
  BOOLE keycode_was_pressed = _prevkeys[keycode];

  /* DEBUG info, not needed now*/
#ifdef _DEBUG
  Service->Log.AddLog("Keypress %s %s%s\n", KeyString(symbolic_key), pressed ? "pressed" : "released", _kbd_in_task_switcher ? " ignored due to ALT-TAB" : "");
#endif

  // Bad hack
  // Unfortunately Windows does not tell the app in any way that the task switcher has been activated,
  // unless you use a hook in the windows accessibility framework.

  // left-alt already pressed, and now TAB pressed as well
  if (!_kbd_in_task_switcher && _keys[map(PCK_LEFT_ALT)] && keycode == map(PCK_TAB) && pressed)
  {
    Service->Log.AddLog("kbdDrvKeypress(): ALT-TAB start detected\n");

    // Apart from the fake LEFT-ALT release event, full-screen does not need additional handling.
    _kbd_in_task_switcher = ((GraphicsDriver&)Driver->Graphics).IsHostBufferWindowed();

    // Don't pass this TAB press along to emulation, pass left-ALT release instead
    keycode = map(PCK_LEFT_ALT);
    symbolic_key = symbolickey(keycode);
    pressed = false;
    keycode_pressed = pressed;
    keycode_was_pressed = _prevkeys[keycode];

#ifdef _DEBUG
    Service->Log.AddLog("Keypress TAB converted to %s %s due to ALT-TAB started\n", KeyString(symbolic_key), pressed ? "pressed" : "released");
#endif
  }
  else if (_kbd_in_task_switcher && symbolic_key == PCK_LEFT_ALT && !pressed)
  {
    Service->Log.AddLog("kbdDrvKeypress(): ALT-TAB end detected\n");
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
      UBY a_code = kbd_drv_pc_symbol_to_amiga_scancode[symbolic_key];
      kbdKeyAdd(a_code | 0x80);
    }
  }
  else if (keycode_pressed && !keycode_was_pressed)
  {
    // If key is not eaten by a Fellow "event", add it to Amiga kbd queue
    if (!EventChecker(symbolic_key))
    {
      UBY a_code = kbd_drv_pc_symbol_to_amiga_scancode[symbolic_key];
      kbdKeyAdd(a_code);
    }
  }
  _prevkeys[keycode] = pressed;
}

/*===========================================================================*/
/* Handle one specific RAW keycode change                                    */
/*===========================================================================*/

void KeyboardDriver::KeypressRaw(ULO lRawKeyCode, BOOLE pressed)
{
#ifdef FELLOW_DELAY_RP_KEYBOARD_INPUT
  BOOLE keycode_was_pressed = prevkeys[lRawKeyCode];
#endif

#ifdef _DEBUG
  Service->Log.AddLog(
      "  kbdDrvKeypressRaw(0x%x, %s): current buffer pos %u, inpos %u\n", lRawKeyCode, pressed ? "pressed" : "released", kbd_state.scancodes.inpos & KBDBUFFERMASK, kbd_state.scancodes.inpos);
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
    kbdKeyAdd(lRawKeyCode | 0x80);
  }
  else
  {
    kbdKeyAdd(lRawKeyCode);
  }
#endif
}

void KeyboardDriver::BufferOverflowHandler()
{
}

void KeyboardDriver:: KeypressHandler()
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
    DInputFailure("kbdDrvKeypressHandler(): GetDeviceData()", res);
  }
  else
  {
    for (ULO i = 0; i < itemcount; i++)
    {
      Keypress(rgod[i].dwOfs, (rgod[i].dwData & 0x80));
    }
  }
}

/*===========================================================================*/
/* Return string describing the given symbolic key                           */
/*===========================================================================*/

const char *KeyboardDriver::KeyString(kbd_drv_pc_symbol symbolickey)
{
  if (symbolickey >= kbd_drv_pc_symbol::PCK_LAST_KEY)
  {
    symbolickey = kbd_drv_pc_symbol::PCK_NONE;
  }
  return kbd_drv_pc_symbol_to_string[symbolickey];
}

const char *KeyboardDriver::KeyPrettyString(kbd_drv_pc_symbol symbolickey)
{
  if (symbolickey >= 106)
  {
    symbolickey = kbd_drv_pc_symbol::PCK_NONE;
  }
  return symbol_pretty_name[symbolickey];
}

const char *KeyboardDriver::DikKeyString(int dikkey)
{
  for (int j = kbd_drv_pc_symbol::PCK_NONE; j < kbd_drv_pc_symbol::PCK_LAST_KEY; j++)
  {
    if (dikkey == symbol_to_DIK_kbddrv[j])
    {
      return KeyString((kbd_drv_pc_symbol)j);
    }
  }
  return "UNKNOWN";
}

/*===========================================================================*/
/* Set joystick replacement for a given joystick and direction               */
/*===========================================================================*/

void KeyboardDriver::JoystickReplacementSet(kbd_event event, kbd_drv_pc_symbol symbolickey)
{
  switch (event)
  {
    case kbd_event::EVENT_JOY0_UP_ACTIVE: kbd_drv_joykey[0][JOYKEY_UP] = symbolickey; break;
    case kbd_event::EVENT_JOY0_DOWN_ACTIVE: kbd_drv_joykey[0][JOYKEY_DOWN] = symbolickey; break;
    case kbd_event::EVENT_JOY0_LEFT_ACTIVE: kbd_drv_joykey[0][JOYKEY_LEFT] = symbolickey; break;
    case kbd_event::EVENT_JOY0_RIGHT_ACTIVE: kbd_drv_joykey[0][JOYKEY_RIGHT] = symbolickey; break;
    case kbd_event::EVENT_JOY0_FIRE0_ACTIVE: kbd_drv_joykey[0][JOYKEY_FIRE0] = symbolickey; break;
    case kbd_event::EVENT_JOY0_FIRE1_ACTIVE: kbd_drv_joykey[0][JOYKEY_FIRE1] = symbolickey; break;
    case kbd_event::EVENT_JOY0_AUTOFIRE0_ACTIVE: kbd_drv_joykey[0][JOYKEY_AUTOFIRE0] = symbolickey; break;
    case kbd_event::EVENT_JOY0_AUTOFIRE1_ACTIVE: kbd_drv_joykey[0][JOYKEY_AUTOFIRE1] = symbolickey; break;
    case kbd_event::EVENT_JOY1_UP_ACTIVE: kbd_drv_joykey[1][JOYKEY_UP] = symbolickey; break;
    case kbd_event::EVENT_JOY1_DOWN_ACTIVE: kbd_drv_joykey[1][JOYKEY_DOWN] = symbolickey; break;
    case kbd_event::EVENT_JOY1_LEFT_ACTIVE: kbd_drv_joykey[1][JOYKEY_LEFT] = symbolickey; break;
    case kbd_event::EVENT_JOY1_RIGHT_ACTIVE: kbd_drv_joykey[1][JOYKEY_RIGHT] = symbolickey; break;
    case kbd_event::EVENT_JOY1_FIRE0_ACTIVE: kbd_drv_joykey[1][JOYKEY_FIRE0] = symbolickey; break;
    case kbd_event::EVENT_JOY1_FIRE1_ACTIVE: kbd_drv_joykey[1][JOYKEY_FIRE1] = symbolickey; break;
    case kbd_event::EVENT_JOY1_AUTOFIRE0_ACTIVE: kbd_drv_joykey[1][JOYKEY_AUTOFIRE0] = symbolickey; break;
    case kbd_event::EVENT_JOY1_AUTOFIRE1_ACTIVE: kbd_drv_joykey[1][JOYKEY_AUTOFIRE1] = symbolickey; break;
  }
}

/*===========================================================================*/
/* Get joystick replacement for a given joystick and direction               */
/*===========================================================================*/

kbd_drv_pc_symbol KeyboardDriver::JoystickReplacementGet(kbd_event event)
{
  switch (event)
  {
    case kbd_event::EVENT_JOY0_UP_ACTIVE: return kbd_drv_joykey[0][JOYKEY_UP];
    case kbd_event::EVENT_JOY0_DOWN_ACTIVE: return kbd_drv_joykey[0][JOYKEY_DOWN];
    case kbd_event::EVENT_JOY0_LEFT_ACTIVE: return kbd_drv_joykey[0][JOYKEY_LEFT];
    case kbd_event::EVENT_JOY0_RIGHT_ACTIVE: return kbd_drv_joykey[0][JOYKEY_RIGHT];
    case kbd_event::EVENT_JOY0_FIRE0_ACTIVE: return kbd_drv_joykey[0][JOYKEY_FIRE0];
    case kbd_event::EVENT_JOY0_FIRE1_ACTIVE: return kbd_drv_joykey[0][JOYKEY_FIRE1];
    case kbd_event::EVENT_JOY0_AUTOFIRE0_ACTIVE: return kbd_drv_joykey[0][JOYKEY_AUTOFIRE0];
    case kbd_event::EVENT_JOY0_AUTOFIRE1_ACTIVE: return kbd_drv_joykey[0][JOYKEY_AUTOFIRE1];
    case kbd_event::EVENT_JOY1_UP_ACTIVE: return kbd_drv_joykey[1][JOYKEY_UP];
    case kbd_event::EVENT_JOY1_DOWN_ACTIVE: return kbd_drv_joykey[1][JOYKEY_DOWN];
    case kbd_event::EVENT_JOY1_LEFT_ACTIVE: return kbd_drv_joykey[1][JOYKEY_LEFT];
    case kbd_event::EVENT_JOY1_RIGHT_ACTIVE: return kbd_drv_joykey[1][JOYKEY_RIGHT];
    case kbd_event::EVENT_JOY1_FIRE0_ACTIVE: return kbd_drv_joykey[1][JOYKEY_FIRE0];
    case kbd_event::EVENT_JOY1_FIRE1_ACTIVE: return kbd_drv_joykey[1][JOYKEY_FIRE1];
    case kbd_event::EVENT_JOY1_AUTOFIRE0_ACTIVE: return kbd_drv_joykey[1][JOYKEY_AUTOFIRE0];
    case kbd_event::EVENT_JOY1_AUTOFIRE1_ACTIVE: return kbd_drv_joykey[1][JOYKEY_AUTOFIRE1];
  }
  return kbd_drv_pc_symbol::PCK_NONE;
}

void KeyboardDriver::InitializeDIKToSymbolKeyTable()
{
  for (int i = 0; i < PCK_LAST_KEY; i++)
  {
    kbddrv_DIK_to_symbol[i] = PCK_NONE;
  }

  kbddrv_DIK_to_symbol[DIK_ESCAPE] = PCK_ESCAPE; /* First row */
  kbddrv_DIK_to_symbol[DIK_F1] = PCK_F1;
  kbddrv_DIK_to_symbol[DIK_F2] = PCK_F2;
  kbddrv_DIK_to_symbol[DIK_F3] = PCK_F3;
  kbddrv_DIK_to_symbol[DIK_F4] = PCK_F4;
  kbddrv_DIK_to_symbol[DIK_F5] = PCK_F5;
  kbddrv_DIK_to_symbol[DIK_F6] = PCK_F6;
  kbddrv_DIK_to_symbol[DIK_F7] = PCK_F7;
  kbddrv_DIK_to_symbol[DIK_F8] = PCK_F8;
  kbddrv_DIK_to_symbol[DIK_F9] = PCK_F9;
  kbddrv_DIK_to_symbol[DIK_F10] = PCK_F10;
  kbddrv_DIK_to_symbol[DIK_F11] = PCK_F11;
  kbddrv_DIK_to_symbol[DIK_F12] = PCK_F12;
  kbddrv_DIK_to_symbol[DIK_SYSRQ] = PCK_PRINT_SCREEN;
  kbddrv_DIK_to_symbol[DIK_SCROLL] = PCK_SCROLL_LOCK;
  /*kbddrv_DIK_to_symbol[ DIK_PAUSE           ] = PCK_PAUSE; */
  kbddrv_DIK_to_symbol[DIK_GRAVE] = PCK_GRAVE; /* Second row */
  kbddrv_DIK_to_symbol[DIK_1] = PCK_1;
  kbddrv_DIK_to_symbol[DIK_2] = PCK_2;
  kbddrv_DIK_to_symbol[DIK_3] = PCK_3;
  kbddrv_DIK_to_symbol[DIK_4] = PCK_4;
  kbddrv_DIK_to_symbol[DIK_5] = PCK_5;
  kbddrv_DIK_to_symbol[DIK_6] = PCK_6;
  kbddrv_DIK_to_symbol[DIK_7] = PCK_7;
  kbddrv_DIK_to_symbol[DIK_8] = PCK_8;
  kbddrv_DIK_to_symbol[DIK_9] = PCK_9;
  kbddrv_DIK_to_symbol[DIK_0] = PCK_0;
  kbddrv_DIK_to_symbol[DIK_MINUS] = PCK_MINUS;
  kbddrv_DIK_to_symbol[DIK_EQUALS] = PCK_EQUALS;
  kbddrv_DIK_to_symbol[DIK_BACK] = PCK_BACKSPACE;
  kbddrv_DIK_to_symbol[DIK_INSERT] = PCK_INSERT;
  kbddrv_DIK_to_symbol[DIK_HOME] = PCK_HOME;
  kbddrv_DIK_to_symbol[DIK_PRIOR] = PCK_PAGE_UP;
  kbddrv_DIK_to_symbol[DIK_NUMLOCK] = PCK_NUM_LOCK;
  kbddrv_DIK_to_symbol[DIK_DIVIDE] = PCK_NUMPAD_DIVIDE;
  kbddrv_DIK_to_symbol[DIK_MULTIPLY] = PCK_NUMPAD_MULTIPLY;
  kbddrv_DIK_to_symbol[DIK_SUBTRACT] = PCK_NUMPAD_MINUS;
  kbddrv_DIK_to_symbol[DIK_TAB] = PCK_TAB; /* Third row */
  kbddrv_DIK_to_symbol[DIK_Q] = PCK_Q;
  kbddrv_DIK_to_symbol[DIK_W] = PCK_W;
  kbddrv_DIK_to_symbol[DIK_E] = PCK_E;
  kbddrv_DIK_to_symbol[DIK_R] = PCK_R;
  kbddrv_DIK_to_symbol[DIK_T] = PCK_T;
  kbddrv_DIK_to_symbol[DIK_Y] = PCK_Y;
  kbddrv_DIK_to_symbol[DIK_U] = PCK_U;
  kbddrv_DIK_to_symbol[DIK_I] = PCK_I;
  kbddrv_DIK_to_symbol[DIK_O] = PCK_O;
  kbddrv_DIK_to_symbol[DIK_P] = PCK_P;
  kbddrv_DIK_to_symbol[DIK_LBRACKET] = PCK_LBRACKET;
  kbddrv_DIK_to_symbol[DIK_RBRACKET] = PCK_RBRACKET;
  kbddrv_DIK_to_symbol[DIK_RETURN] = PCK_RETURN;
  kbddrv_DIK_to_symbol[DIK_DELETE] = PCK_DELETE;
  kbddrv_DIK_to_symbol[DIK_END] = PCK_END;
  kbddrv_DIK_to_symbol[DIK_NEXT] = PCK_PAGE_DOWN;
  kbddrv_DIK_to_symbol[DIK_NUMPAD7] = PCK_NUMPAD_7;
  kbddrv_DIK_to_symbol[DIK_NUMPAD8] = PCK_NUMPAD_8;
  kbddrv_DIK_to_symbol[DIK_NUMPAD9] = PCK_NUMPAD_9;
  kbddrv_DIK_to_symbol[DIK_ADD] = PCK_NUMPAD_PLUS;

  kbddrv_DIK_to_symbol[DIK_CAPITAL] = PCK_CAPS_LOCK; /* Fourth row */
  kbddrv_DIK_to_symbol[DIK_A] = PCK_A;
  kbddrv_DIK_to_symbol[DIK_S] = PCK_S;
  kbddrv_DIK_to_symbol[DIK_D] = PCK_D;
  kbddrv_DIK_to_symbol[DIK_F] = PCK_F;
  kbddrv_DIK_to_symbol[DIK_G] = PCK_G;
  kbddrv_DIK_to_symbol[DIK_H] = PCK_H;
  kbddrv_DIK_to_symbol[DIK_J] = PCK_J;
  kbddrv_DIK_to_symbol[DIK_K] = PCK_K;
  kbddrv_DIK_to_symbol[DIK_L] = PCK_L;
  kbddrv_DIK_to_symbol[DIK_SEMICOLON] = PCK_SEMICOLON;
  kbddrv_DIK_to_symbol[DIK_APOSTROPHE] = PCK_APOSTROPHE;
  kbddrv_DIK_to_symbol[DIK_BACKSLASH] = PCK_BACKSLASH;
  kbddrv_DIK_to_symbol[DIK_NUMPAD4] = PCK_NUMPAD_4;
  kbddrv_DIK_to_symbol[DIK_NUMPAD5] = PCK_NUMPAD_5;
  kbddrv_DIK_to_symbol[DIK_NUMPAD6] = PCK_NUMPAD_6;

  kbddrv_DIK_to_symbol[DIK_LSHIFT] = PCK_LEFT_SHIFT; /* Fifth row */
  kbddrv_DIK_to_symbol[0x56] = PCK_NONAME1;
  kbddrv_DIK_to_symbol[DIK_Z] = PCK_Z;
  kbddrv_DIK_to_symbol[DIK_X] = PCK_X;
  kbddrv_DIK_to_symbol[DIK_C] = PCK_C;
  kbddrv_DIK_to_symbol[DIK_V] = PCK_V;
  kbddrv_DIK_to_symbol[DIK_B] = PCK_B;
  kbddrv_DIK_to_symbol[DIK_N] = PCK_N;
  kbddrv_DIK_to_symbol[DIK_M] = PCK_M;
  kbddrv_DIK_to_symbol[DIK_COMMA] = PCK_COMMA;
  kbddrv_DIK_to_symbol[DIK_PERIOD] = PCK_PERIOD;
  kbddrv_DIK_to_symbol[DIK_SLASH] = PCK_SLASH;
  kbddrv_DIK_to_symbol[DIK_RSHIFT] = PCK_RIGHT_SHIFT;
  kbddrv_DIK_to_symbol[DIK_UP] = PCK_UP;
  kbddrv_DIK_to_symbol[DIK_NUMPAD1] = PCK_NUMPAD_1;
  kbddrv_DIK_to_symbol[DIK_NUMPAD2] = PCK_NUMPAD_2;
  kbddrv_DIK_to_symbol[DIK_NUMPAD3] = PCK_NUMPAD_3;
  kbddrv_DIK_to_symbol[DIK_NUMPADENTER] = PCK_NUMPAD_ENTER;

  kbddrv_DIK_to_symbol[DIK_LCONTROL] = PCK_LEFT_CTRL; /* Sixth row */
  kbddrv_DIK_to_symbol[DIK_LWIN] = PCK_LEFT_WINDOWS;
  kbddrv_DIK_to_symbol[DIK_LMENU] = PCK_LEFT_ALT;
  kbddrv_DIK_to_symbol[DIK_SPACE] = PCK_SPACE;
  kbddrv_DIK_to_symbol[DIK_RMENU] = PCK_RIGHT_ALT;
  kbddrv_DIK_to_symbol[DIK_RWIN] = PCK_RIGHT_WINDOWS;
  kbddrv_DIK_to_symbol[DIK_APPS] = PCK_START_MENU;
  kbddrv_DIK_to_symbol[DIK_RCONTROL] = PCK_RIGHT_CTRL;
  kbddrv_DIK_to_symbol[DIK_LEFT] = PCK_LEFT;
  kbddrv_DIK_to_symbol[DIK_DOWN] = PCK_DOWN;
  kbddrv_DIK_to_symbol[DIK_RIGHT] = PCK_RIGHT;
  kbddrv_DIK_to_symbol[DIK_NUMPAD0] = PCK_NUMPAD_0;
  kbddrv_DIK_to_symbol[DIK_DECIMAL] = PCK_NUMPAD_DOT;
}

void KeyboardDriver::SetJoyKeyEnabled(ULO lGameport, ULO lSetting, BOOLE bEnabled)
{
  kbd_drv_joykey_enabled[lGameport][lSetting] = bEnabled;
}

void KeyboardDriver::HardReset()
{
}

void KeyboardDriver::EmulationStart()
{
  for (ULO port = 0; port < 2; port++)
  {
    kbd_drv_joykey_enabled[port][0] = (gameport_input[port] == gameport_inputs::GP_JOYKEY0);
    kbd_drv_joykey_enabled[port][1] = (gameport_input[port] == gameport_inputs::GP_JOYKEY1);
  }

  ClearPressedKeys();
  DInputInitialize();
  _kbd_in_task_switcher = false;
}

void KeyboardDriver::EmulationStop()
{
  DInputRelease();
}

void KeyboardDriver::Startup()
{
  kbd_drv_joykey_event[0][0][JOYKEY_UP] = kbd_event::EVENT_JOY0_UP_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_UP] = kbd_event::EVENT_JOY0_UP_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_UP] = kbd_event::EVENT_JOY1_UP_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_UP] = kbd_event::EVENT_JOY1_UP_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_DOWN] = kbd_event::EVENT_JOY0_DOWN_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_DOWN] = kbd_event::EVENT_JOY0_DOWN_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_DOWN] = kbd_event::EVENT_JOY1_DOWN_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_DOWN] = kbd_event::EVENT_JOY1_DOWN_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_LEFT] = kbd_event::EVENT_JOY0_LEFT_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_LEFT] = kbd_event::EVENT_JOY0_LEFT_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_LEFT] = kbd_event::EVENT_JOY1_LEFT_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_LEFT] = kbd_event::EVENT_JOY1_LEFT_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_RIGHT] = kbd_event::EVENT_JOY0_RIGHT_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_RIGHT] = kbd_event::EVENT_JOY0_RIGHT_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_RIGHT] = kbd_event::EVENT_JOY1_RIGHT_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_RIGHT] = kbd_event::EVENT_JOY1_RIGHT_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_FIRE0] = kbd_event::EVENT_JOY0_FIRE0_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_FIRE0] = kbd_event::EVENT_JOY0_FIRE0_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_FIRE0] = kbd_event::EVENT_JOY1_FIRE0_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_FIRE0] = kbd_event::EVENT_JOY1_FIRE0_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_FIRE1] = kbd_event::EVENT_JOY0_FIRE1_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_FIRE1] = kbd_event::EVENT_JOY0_FIRE1_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_FIRE1] = kbd_event::EVENT_JOY1_FIRE1_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_FIRE1] = kbd_event::EVENT_JOY1_FIRE1_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_AUTOFIRE0] = kbd_event::EVENT_JOY0_AUTOFIRE0_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_AUTOFIRE0] = kbd_event::EVENT_JOY0_AUTOFIRE0_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_AUTOFIRE0] = kbd_event::EVENT_JOY1_AUTOFIRE0_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_AUTOFIRE0] = kbd_event::EVENT_JOY1_AUTOFIRE0_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_AUTOFIRE1] = kbd_event::EVENT_JOY0_AUTOFIRE1_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_AUTOFIRE1] = kbd_event::EVENT_JOY0_AUTOFIRE1_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_AUTOFIRE1] = kbd_event::EVENT_JOY1_AUTOFIRE1_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_AUTOFIRE1] = kbd_event::EVENT_JOY1_AUTOFIRE1_ACTIVE;

  kbd_drv_joykey[0][JOYKEY_UP] = PCK_UP;
  kbd_drv_joykey[0][JOYKEY_DOWN] = PCK_DOWN;
  kbd_drv_joykey[0][JOYKEY_LEFT] = PCK_LEFT;
  kbd_drv_joykey[0][JOYKEY_RIGHT] = PCK_RIGHT;
  kbd_drv_joykey[0][JOYKEY_FIRE0] = PCK_RIGHT_CTRL;
  kbd_drv_joykey[0][JOYKEY_FIRE1] = PCK_RIGHT_ALT;
  kbd_drv_joykey[0][JOYKEY_AUTOFIRE0] = PCK_O;
  kbd_drv_joykey[0][JOYKEY_AUTOFIRE1] = PCK_P;
  kbd_drv_joykey[1][JOYKEY_UP] = PCK_R;
  kbd_drv_joykey[1][JOYKEY_DOWN] = PCK_F;
  kbd_drv_joykey[1][JOYKEY_LEFT] = PCK_D;
  kbd_drv_joykey[1][JOYKEY_RIGHT] = PCK_G;
  kbd_drv_joykey[1][JOYKEY_FIRE0] = PCK_LEFT_CTRL;
  kbd_drv_joykey[1][JOYKEY_FIRE1] = PCK_LEFT_ALT;
  kbd_drv_joykey[1][JOYKEY_AUTOFIRE0] = PCK_A;
  kbd_drv_joykey[1][JOYKEY_AUTOFIRE1] = PCK_S;

  for (ULO port = 0; port < 2; port++)
  {
    for (ULO setting = 0; setting < 2; setting++)
    {
      kbd_drv_joykey_enabled[port][setting] = FALSE;
    }
  }

  kbd_drv_capture = FALSE;
  kbd_drv_captured_key = PCK_NONE;

  InitializeDIKToSymbolKeyTable();

  Service->Fileops.GetGenericFileName(_mapping_filename, "WinFellow", MAPPING_FILENAME);

  _prs_rewrite_mapping_file = prsReadFile(_mapping_filename, kbd_drv_pc_symbol_to_amiga_scancode, kbd_drv_joykey);

  _active = FALSE;
  _lpDI = nullptr;
  _lpDID = nullptr;
}

void KeyboardDriver::Shutdown()
{
  if (_prs_rewrite_mapping_file)
  {
    prsWriteFile(_mapping_filename, kbd_drv_pc_symbol_to_amiga_scancode, kbd_drv_joykey);
  }
}
