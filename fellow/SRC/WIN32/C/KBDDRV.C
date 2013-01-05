/* @(#) $Id: KBDDRV.C,v 1.18 2013-01-05 15:24:46 carfesh Exp $             */
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
/* ---------------- KNOWN BUGS/FIXLIST ----------------- 
- additional key functions for wide usage by the emulator
*/

/* ---------------- CHANGE LOG ----------------- 
Friday evening, January 05, 2001: nova
- fixed a little mistake (a " was missing)
- if the mapping file is not found, a new one will be written at the end of the emulation

Friday, January 05, 2001: nova
- fixed CaptureKey
- added kbdDrvKeyPrettyString used to display a more pretty name of a key

Tuesday, September 19, 2000: nova
- added autofire support

Tuesday, September 05, 2000: nova
- added DikKeyString for the translation of a DIK key to a string - highly not optimized - ** internal **
- added kbd_drv_joykey to the prsReadFile call ** internal **
- now joy replacement are first filled with DIK keys and then translated to PCK symbolic key
- added dxver.h include, dx version is decided with USE_DX3 or USE_DX5 macro

Sunday, February 03, 2008: carfesh
- rebuild to use DirectInput8Create instead of DirectInputCreate

Sunday, February 10, 2008: carfesh
- move mapping.key to application data
*/

#include "defs.h"
#include "keycodes.h"
#include "kbd.h"
#include "kbddrv.h"
#include "mousedrv.h"
#include "joydrv.h"
#include "gameport.h"
#include "windrv.h"
#include "kbdparser.h"
#include "fellow.h"

#include "dxver.h"
#include "fileops.h"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif

#define MAX_KEYS	256

#define DINPUT_BUFFERSIZE 256

#define MAPPING_FILENAME  "mapping.key"

/*===========================================================================*/
/* Keyboard specific data                                                    */
/*===========================================================================*/

BOOLE			          kbd_drv_active;
BOOLE               kbd_drv_in_use;
LPDIRECTINPUT		    kbd_drv_lpDI;
LPDIRECTINPUTDEVICE	kbd_drv_lpDID;
HANDLE			        kbd_drv_DIevent;
BYTE			          keys[MAX_KEYS];					// contains boolean values (pressed/not pressed) for actual keystroke
BYTE			          prevkeys[MAX_KEYS];				// contains boolean values (pressed/not pressed) for past keystroke
BOOLE               kbd_drv_initialization_failed;
BOOLE               prs_rewrite_mapping_file;
char                kbd_drv_mapping_filename[MAX_PATH];


/*===========================================================================*/
/* Map symbolic key to a description string                                  */
/*===========================================================================*/

STR *kbd_drv_pc_symbol_to_string[106] = {
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

STR *symbol_pretty_name[106] = {
  "none",
  "Escape",
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
  "Print screen",
  "Scroll lock",
  "Pause",
  "Grave",
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
  "-",
  "=",
  "Backspace",
  "Insert",
  "Home",
  "Page up",
  "Num lock",
  "/ (numpad)",
  "* (numpad)",
  "- (numpad)",
  "Tab",
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
  "[",
  "]",
  "Return",
  "Delete",
  "End",
  "Page down",
  "7 (numpad)",
  "8 (numpad)",
  "9 (numpad)",
  "+ (numpad)",
  "Caps lock",
  "A",
  "S",
  "D",
  "F",
  "G",
  "H",
  "J",
  "K",
  "L",
  ";",
  "'",
  "\\",
  "4 (numpad)",
  "5 (numpad)",
  "6 (numpad)",
  "Left shift",
  "<",
  "Z",
  "X",
  "C",
  "V",
  "B",
  "N",
  "M",
  ",",
  ".",
  "/",
  "Right shift",
  "Arrow up",
  "1 (numpad)",
  "2 (numpad)",
  "3 (numpad)",
  "Enter (numpad)",
  "Left ctrl",
  "Left win",
  "Left alt",
  " ",
  "Right alt",
  "Right win",
  "Right menu",
  "Right ctrl",
  "Arrow left",
  "Arrow down",
  "Arrow right",
  "0 (numpad)",
  ". (numpad)"};

    
/*===========================================================================*/
/* Map windows virtual key to intermediate key symbol                        */
/*===========================================================================*/
    
kbd_drv_pc_symbol kbddrv_DIK_to_symbol[MAX_KEYS];
    
int symbol_to_DIK_kbddrv[PCK_LAST_KEY] = {
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
  0x56,  /* Not defined */
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
  DIK_DECIMAL
};

/*===========================================================================*/
/* Map symbolic pc key to amiga scancode                                     */
/*===========================================================================*/

UBY kbd_drv_pc_symbol_to_amiga_scancode[106] = {
  /* 0x00 */

  A_NONE,           /* PCK_NONE            */
  A_ESCAPE,         /* PCK_ESCAPE          */
  A_F1,             /* PCK_F1              */
  A_F2,             /* PCK_F2              */
  
  A_F3,             /* PCK_F3              */
  A_F4,             /* PCK_F4              */
  A_F5,             /* PCK_F5              */
  A_F6,             /* PCK_F6              */
  
  A_F7,             /* PCK_F7              */
  A_F8,             /* PCK_F8              */
  A_F9,             /* PCK_F9              */
  A_F10,            /* PCK_F10             */
  
  A_NONE,           /* PCK_F11             */
  A_NONE,           /* PCK_F12             */
  A_NONE,           /* PCK_PRINT_SCREEN    */
  A_BACKSLASH2,     /* PCK_SCROLL_LOCK     */

  /* 0x10 */

  A_NUMPAD_RIGHT_BRACKET,/* PCK_PAUSE           */
  A_GRAVE,          /* PCK_GRAVE           */
  A_1,              /* PCK_1               */
  A_2,              /* PCK_2               */
  
  A_3,              /* PCK_3               */
  A_4,              /* PCK_4               */
  A_5,              /* PCK_5               */
  A_6,              /* PCK_6               */
  
  A_7,              /* PCK_7               */
  A_8,              /* PCK_8               */
  A_9,              /* PCK_9               */
  A_0,              /* PCK_0               */
  
  A_MINUS,          /* PCK_MINUS           */
  A_EQUALS,         /* PCK_EQUALS          */
  A_BACKSPACE,      /* PCK_BACKSPACE       */
  A_HELP,           /* PCK_INSERT          */
  
  /* 0x20 */
  
  A_NONE,           /* PCK_HOME            */
  A_RIGHT_AMIGA,    /* PCK_PAGE_UP         */
  A_NUMPAD_LEFT_BRACKET,/* PCK_NUM_LOCK        */
  A_NUMPAD_DIVIDE,  /* PCK_NUMPAD_DIVIDE   */
  
  A_NUMPAD_MULTIPLY,/* PCK_NUMPAD_MULTIPLY */
  A_NUMPAD_MINUS,   /* PCK_NUMPAD_MINUS    */
  A_TAB,            /* PCK_TAB             */
  A_Q,              /* PCK_Q               */
  
  A_W,              /* PCK_W               */
  A_E,              /* PCK_E               */
  A_R,              /* PCK_R               */
  A_T,              /* PCK_T               */
  
  A_Y,              /* PCK_Y               */
  A_U,              /* PCK_U               */
  A_I,              /* PCK_I               */
  A_O,              /* PCK_O               */
  
  /* 0x30 */
  
  A_P,              /* PCK_P               */
  A_LEFT_BRACKET,   /* PCK_LBRACKET        */
  A_RIGHT_BRACKET,  /* PCK_RBRACKET        */
  A_RETURN,         /* PCK_RETURN          */
  
  A_DELETE,         /* PCK_DELETE          */
  A_NONE,           /* PCK_END             */
  A_LEFT_AMIGA,     /* PCK_PAGE_DOWN       */
  A_NUMPAD_7,       /* PCK_NUMPAD_7        */
  
  A_NUMPAD_8,       /* PCK_NUMPAD_8        */
  A_NUMPAD_9,       /* PCK_NUMPAD_9        */
  A_NUMPAD_PLUS,    /* PCK_NUMPAD_PLUS     */
  A_CAPS_LOCK,      /* PCK_CAPS_LOCK       */
  
  A_A,              /* PCK_A               */
  A_S,              /* PCK_S               */
  A_D,              /* PCK_D               */
  A_F,              /* PCK_F               */
  
  /* 0x40 */
  
  A_G,              /* PCK_G               */
  A_H,              /* PCK_H               */
  A_J,              /* PCK_J               */
  A_K,              /* PCK_K               */

  A_L,              /* PCK_L               */
  A_SEMICOLON,      /* PCK_SEMICOLON       */
  A_APOSTROPHE,     /* PCK_APOSTROPHE      */
  A_BACKSLASH,      /* PCK_BACKSLASH       */
  
  A_NUMPAD_4,       /* PCK_NUMPAD_4        */
  A_NUMPAD_5,       /* PCK_NUMPAD_5        */
  A_NUMPAD_6,       /* PCK_NUMPAD_6        */
  A_LEFT_SHIFT,     /* PCK_LEFT_SHIFT      */
  
  A_LESS_THAN,      /* PCK_NONAME1         */
  A_Z,              /* PCK_Z               */
  A_X,              /* PCK_X               */
  A_C,              /* PCK_C               */
  
  /* 0x50 */  
  
  A_V,              /* PCK_V               */
  A_B,              /* PCK_B               */
  A_N,              /* PCK_N               */
  A_M,              /* PCK_M               */

  A_COMMA,          /* PCK_COMMA           */
  A_PERIOD,         /* PCK_PERIOD          */
  A_SLASH,          /* PCK_SLASH           */
  A_RIGHT_SHIFT,    /* PCK_RIGHT_SHIFT     */
  
  A_UP,             /* PCK_UP              */
  A_NUMPAD_1,       /* PCK_NUMPAD_1        */
  A_NUMPAD_2,       /* PCK_NUMPAD_2        */
  A_NUMPAD_3,       /* PCK_NUMPAD_3        */
  
  A_NUMPAD_ENTER,   /* PCK_NUMPAD_ENTER    */
  A_LEFT_AMIGA,     /* PCK_LEFT_CTRL       */
  A_LEFT_AMIGA,     /* PCK_LEFT_WINDOWS    */
  A_LEFT_ALT,       /* PCK_LEFT_ALT        */
  
  /* 0x60 */  
  
  A_SPACE,          /* PCK_SPACE           */
  A_RIGHT_ALT,      /* PCK_RIGHT_ALT       */
  A_RIGHT_AMIGA,    /* PCK_RIGHT_WINDOWS   */
  A_NONE,           /* PCK_START_MENU      */

  A_CTRL,           /* PCK_RIGHT_CTRL      */
  A_LEFT,           /* PCK_LEFT            */
  A_DOWN,           /* PCK_DOWN            */
  A_RIGHT,          /* PCK_RIGHT           */
  
  A_NUMPAD_0,       /* PCK_NUMPAD_0        */
  A_NUMPAD_DOT      /* PCK_NUMPAD_DOT      */
};

/*===========================================================================*/
/* Keyboard handler state data                                               */
/*===========================================================================*/

BOOLE kbd_drv_home_pressed;
BOOLE kbd_drv_end_pressed;

// the order of kbd_drv_joykey_directions enum influence the order of the replacement_keys array in kbdparser.c

typedef enum {
  JOYKEY_LEFT = 0,
  JOYKEY_RIGHT = 1,
  JOYKEY_UP = 2,
  JOYKEY_DOWN = 3,
  JOYKEY_FIRE0 = 4,
  JOYKEY_FIRE1 = 5,
  JOYKEY_AUTOFIRE0 = 6,
  JOYKEY_AUTOFIRE1 = 7
} kbd_drv_joykey_directions;

kbd_drv_pc_symbol		kbd_drv_joykey[2][MAX_JOYKEY_VALUE];			/* Keys for joykeys 0 and 1 */
BOOLE				kbd_drv_joykey_enabled[2][2];	/* For each port, the enabled joykeys */
kbd_event			kbd_drv_joykey_event[2][2][MAX_JOYKEY_VALUE];	/* Event ID for each joykey [port][pressed][direction] */
volatile kbd_drv_pc_symbol	kbd_drv_captured_key;
BOOLE				kbd_drv_capture;


/*==========================================================================*/
/* Returns textual error message. Adapted from DX SDK                       */
/*==========================================================================*/

STR *kbdDrvDInputErrorString(HRESULT hResult) {
  switch (hResult) {
    case DI_OK:				            return "The operation completed successfully.";
    case DI_BUFFEROVERFLOW:		        return "The device buffer overflowed and some input was lost.";
    case DI_POLLEDDEVICE:		        return "The device is a polled device.";
    case DIERR_ACQUIRED:		        return "The operation cannot be performed while the device is acquired.";
    case DIERR_ALREADYINITIALIZED:	    return "This object is already initialized.";
    case DIERR_BADDRIVERVER:		    return "The object could not be created due to an incompatible driver version or mismatched or incomplete driver components.";
    case DIERR_BETADIRECTINPUTVERSION:	return "The application was written for an unsupported prerelease version of DirectInput.";
    case DIERR_DEVICENOTREG:		    return "The device or device instance is not registered with DirectInput.";
    case DIERR_GENERIC:			        return "An undetermined error occurred inside the DirectInput subsystem.";
    case DIERR_HANDLEEXISTS:	        return "The device already has an event notification associated with it.";
    case DIERR_INPUTLOST:		        return "Access to the input device has been lost. It must be re-acquired.";
    case DIERR_INVALIDPARAM:		    return "An invalid parameter was passed to the returning function, or the object was not in a state that permitted the function to be called.";
    case DIERR_NOAGGREGATION:		    return "This object does not support aggregation.";
    case DIERR_NOINTERFACE:		        return "The specified interface is not supported by the object.";
    case DIERR_NOTACQUIRED:		        return "The operation cannot be performed unless the device is acquired.";
    case DIERR_NOTINITIALIZED:		    return "This object has not been initialized.";
    case DIERR_OBJECTNOTFOUND:		    return "The requested object does not exist.";
    case DIERR_OLDDIRECTINPUTVERSION:	return "The application requires a newer version of DirectInput.";
    case DIERR_OUTOFMEMORY:		        return "The DirectInput subsystem couldn't allocate sufficient memory to complete the caller's request.";
    case DIERR_UNSUPPORTED:		        return "The function called is not supported at this time.";
    case E_PENDING:			            return "Data is not yet available.";
	case E_POINTER:                     return "Invalid pointer.";
  }
  return "Not a DirectInput Error";
}


/*==========================================================================*/
/* Logs a sensible error message                                            */
/*==========================================================================*/

void kbdDrvDInputFailure(STR *header, HRESULT err) {
  fellowAddLog(header);
  fellowAddLog(kbdDrvDInputErrorString(err));
  fellowAddLog("\n");
}


/*===========================================================================*/
/* Set keyboard cooperative level                                            */
/*===========================================================================*/

void kbdDrvDInputSetCooperativeLevel(void) {
  HRESULT res = IDirectInputDevice_SetCooperativeLevel(kbd_drv_lpDID,
						       gfx_drv_hwnd,
						       DISCL_NONEXCLUSIVE | 
						       DISCL_FOREGROUND );
  if( res != DI_OK )
    kbdDrvDInputFailure("kbdDrvDInputSetCooperativeLevel(): ", res );
}


/*===========================================================================*/
/* Unacquire DirectInput keyboard device                                     */
/*===========================================================================*/

void kbdDrvDInputUnacquire(void) {
  HRESULT res;
  
  fellowAddLog("kbdDrvDInputUnacquire()\n");
  if (kbd_drv_lpDID == NULL) return;
  res = IDirectInputDevice_Unacquire(kbd_drv_lpDID);
  if (res)
    kbdDrvDInputFailure("kbdDrvDInputUnacquire(): ", res);
}


/*===========================================================================*/
/* Acquire DirectInput keyboard device                                       */
/*===========================================================================*/

void kbdDrvDInputAcquire(void) {
  HRESULT res;
  
  fellowAddLog("kbdDrvDInputAcquire()\n");
  if (kbd_drv_lpDID == NULL) return;
  kbdDrvDInputUnacquire();
  if ((res = IDirectInputDevice_Acquire( kbd_drv_lpDID )) != DI_OK)
    kbdDrvDInputFailure("kbdDrvDInputAcquire(): ", res);
}


/*===========================================================================*/
/* Release DirectInput for keyboard                                          */
/*===========================================================================*/

void kbdDrvDInputRelease(void) {
  if (kbd_drv_lpDID != NULL) {
    IDirectInputDevice_Release(kbd_drv_lpDID);
    kbd_drv_lpDID = NULL;
  }
  if (kbd_drv_DIevent != NULL) {
    CloseHandle(kbd_drv_DIevent);
    kbd_drv_DIevent = NULL;
  }
  if (kbd_drv_lpDI != NULL) {
    IDirectInput_Release(kbd_drv_lpDI);
    kbd_drv_lpDI = NULL;
  }
}


/*===========================================================================*/
/* Initialize DirectInput for keyboard                                       */
/*===========================================================================*/

void kbdDrvDInputInitializeOld(void) {
  HRESULT res;
  
  fellowAddLog("kbdDrvDInputInitialize()\n");
  if(!kbd_drv_lpDI) {
    if(( res = DirectInput8Create(win_drv_hInstance,
      DIRECTINPUT_VERSION,
      &IID_IDirectInput8,
      (void**)&kbd_drv_lpDI,
	  NULL)) != DI_OK )
    {
      kbdDrvDInputFailure("kbdDrvDInputInitialize(): DirectInput8Create() ", res );
      return;
    }
  }
  
  if( !kbd_drv_lpDID )
  {
    res = IDirectInput_CreateDevice(kbd_drv_lpDI,
      &GUID_SysKeyboard,
      &kbd_drv_lpDID,
      NULL );
    if( res != DI_OK )
    {
      kbdDrvDInputFailure("kbdDrvDInputInitialize(): CreateDevice() ", res );
      return;
    }
  }
  
  res = IDirectInputDevice_SetDataFormat( kbd_drv_lpDID, &c_dfDIKeyboard );
  if( res != DI_OK )
  {
    kbdDrvDInputFailure("kbdDrvDInputInitialize(): SetDataFormat() ", res );
    return;
  }
}


BOOLE kbdDrvDInputInitialize(void) {
  HRESULT res;
  DIPROPDWORD dipdw =
  {
    {
      sizeof(DIPROPDWORD),        /* diph.dwSize */
      sizeof(DIPROPHEADER),       /* diph.dwHeaderSize */
      0,                          /* diph.dwObj */
      DIPH_DEVICE,                /* diph.dwHow */
    },
    DINPUT_BUFFERSIZE,            /* dwData */
  };
  
  /* Create Direct Input object */
  
  kbd_drv_lpDI = NULL;
  kbd_drv_lpDID = NULL;
  kbd_drv_DIevent = NULL;
  kbd_drv_initialization_failed = FALSE;
  if ((res = DirectInput8Create(win_drv_hInstance,
                               DIRECTINPUT_VERSION,
                               &IID_IDirectInput8,
                               (void**)&kbd_drv_lpDI,
							   NULL)) != DI_OK) {
    kbdDrvDInputFailure("kbdDrvDInputInitialize(): DirectInput8Create() ", res );
    kbd_drv_initialization_failed = TRUE;
    kbdDrvDInputRelease();
    return FALSE;
  }
  
  /* Create Direct Input 1 keyboard device */
  
  if ((res = IDirectInput_CreateDevice(kbd_drv_lpDI,
                                       &GUID_SysKeyboard,
                                       &kbd_drv_lpDID,
                                       NULL)) != DI_OK) {
    kbdDrvDInputFailure("kbdDrvDInputInitialize(): CreateDevice() ", res );
    kbd_drv_initialization_failed = TRUE;
    kbdDrvDInputRelease();
    return FALSE;
  }
  
  /* Set data format for mouse device */
  
  if ((res = IDirectInputDevice_SetDataFormat(kbd_drv_lpDID,
                                              &c_dfDIKeyboard)) != DI_OK) {
    kbdDrvDInputFailure("kbdDrvDInputInitialize(): SetDataFormat() ", res );
    kbd_drv_initialization_failed = TRUE;
    kbdDrvDInputRelease();
    return FALSE;
  }

  /* Set cooperative level */
  
  if ((res = IDirectInputDevice_SetCooperativeLevel(kbd_drv_lpDID,
						    gfx_drv_hwnd,
						    DISCL_NONEXCLUSIVE |
						    DISCL_FOREGROUND)) != DI_OK) {
    kbdDrvDInputFailure("kbdDrvDInputInitialize(): SetCooperativeLevel() ", res );
    kbd_drv_initialization_failed = TRUE;
    kbdDrvDInputRelease();
    return FALSE;
  }

  /* Create event for notification */
  
  if ((kbd_drv_DIevent = CreateEvent(0, 0, 0, 0)) == NULL) {
    fellowAddLog("kbdDrvDInputInitialize(): CreateEvent() failed\n");
    kbd_drv_initialization_failed = TRUE;
    kbdDrvDInputRelease();
    return FALSE;
  }

  /* Set property for buffered data */
  
  if ((res = IDirectInputDevice_SetProperty(kbd_drv_lpDID,
				            DIPROP_BUFFERSIZE,
				            &dipdw.diph)) != DI_OK) {
    kbdDrvDInputFailure("kbdDrvDInputInitialize(): SetProperty() ", res );
    kbd_drv_initialization_failed = TRUE;
    kbdDrvDInputRelease();
  }

  /* Set event notification */

  if ((res = IDirectInputDevice_SetEventNotification(kbd_drv_lpDID,
                                                     kbd_drv_DIevent)) != DI_OK) {
    kbdDrvDInputFailure("kbdDrvDInputInitialize(): SetEventNotification() ", res );
    kbd_drv_initialization_failed = TRUE;
    kbdDrvDInputRelease();
    return FALSE;
  }
  return TRUE;
}


/*===========================================================================*/
/* keyboard grab status has changed                                          */
/*===========================================================================*/

void kbdDrvStateHasChanged(BOOL active) {
  kbd_drv_active = active;
  if (kbd_drv_active) kbdDrvDInputAcquire();
  else kbdDrvDInputUnacquire();
}

#define map(sym) symbol_to_DIK_kbddrv[(sym)]
#define symbolickey(scancode) kbddrv_DIK_to_symbol[scancode]
#define ispressed(sym) (keys[map(sym)])
#define waspressed(sym) (prevkeys[map(sym)])
#define issue_event(the_event) { kbdEventEOFAdd((the_event)); break; }
#define released(sym) (!ispressed( (sym) ) && waspressed( (sym) ))
#define pressed(sym) ((ispressed((sym))) && (!waspressed((sym))))


/*===========================================================================*/
/* Keyboard keypress emulator control event checker                          */
/* Returns TRUE if the key generated an event                                */
/* which means it must not be passed to the emulator                         */
/*===========================================================================*/

BOOLE kbdDrvEventChecker(kbd_drv_pc_symbol symbol_key) {
  ULO eol_evpos = kbd_state.eventsEOL.inpos;
  ULO eof_evpos = kbd_state.eventsEOF.inpos;
  
  ULO port, setting;
  
  for (;;) {
	
	if (kbd_drv_capture) {

#ifdef _DEBUG
		fellowAddLog( "Key captured: %s\n", kbdDrvKeyString( symbol_key ));
#endif

		kbd_drv_captured_key = symbol_key;
		return TRUE;
	}
	
	if( released( PCK_PAGE_DOWN ))
	{
	  if( ispressed(PCK_HOME) )
	  {
		issue_event( EVENT_RESOLUTION_NEXT );
	  } else {
		if( ispressed(PCK_END) )
		  issue_event( EVENT_SCALEY_NEXT );
	  }
	}
	if( released( PCK_PAGE_UP ))
	{
	  if( ispressed(PCK_HOME) )
	  {
		issue_event( EVENT_RESOLUTION_PREV );
	  } else {
		if( ispressed(PCK_END) )
		  issue_event( EVENT_SCALEY_PREV );
	  }
	}

#ifdef RETRO_PLATFORM
  if(RetroPlatformGetMode()) {
    if( pressed ( RetroPlatformGetEscapeKey() ))
      issue_event( EVENT_RPESCAPE_ACTIVE );
    if( released ( RetroPlatformGetEscapeKey() ))
      issue_event( EVENT_RPESCAPE_INACTIVE );
  }
#endif

#ifdef RETRO_PLATFORM
  if(!RetroPlatformGetMode())
#endif
	if( released( PCK_F11 ))
	  issue_event( EVENT_EXIT );
	
	if( released( PCK_F12 ))
	{
	  mouseDrvToggleFocus();
	  joyDrvToggleFocus();
    //spriteSetDebugging();
	  break;
	}
	
	if( ispressed(PCK_HOME) )
	{
	  if( released( PCK_F1 )) issue_event( EVENT_INSERT_DF0 );
	  if( released( PCK_F2 )) issue_event( EVENT_INSERT_DF1 );
	  if( released( PCK_F3 )) issue_event( EVENT_INSERT_DF2 );
	  if( released( PCK_F4 )) issue_event( EVENT_INSERT_DF3 );
	}
	else if( ispressed(PCK_END) )
	{
	  if( released( PCK_F1 )) issue_event( EVENT_EJECT_DF0 );
	  if( released( PCK_F2 )) issue_event( EVENT_EJECT_DF1 );
	  if( released( PCK_F3 )) issue_event( EVENT_EJECT_DF2 );
	  if( released( PCK_F4 )) issue_event( EVENT_EJECT_DF3 );
	}
	/*
	if( released( PCK_F11 ) && kbd_drv_home_pressed )
	issue_event( EVENT_BMP_DUMP );
	*/
	
	if( ispressed(PCK_HOME) )
	{
	  if( ispressed( PCK_NUMPAD_2 ))
		issue_event( EVENT_SCROLL_DOWN );
	  if( ispressed( PCK_NUMPAD_4 ))
		issue_event( EVENT_SCROLL_LEFT );
	  
	  if( ispressed( PCK_NUMPAD_6 ))
		issue_event( EVENT_SCROLL_RIGHT );
	  if( ispressed( PCK_NUMPAD_8 ))
		issue_event( EVENT_SCROLL_UP );
	}
	
	// Check joysticks replacements
	// New here: Must remember last value to decide if a change has happened
	
	for( port = 0; port < 2; port++ )
	  for( setting = 0; setting < 2; setting++ )
		if( kbd_drv_joykey_enabled[port][setting] )
		{
		  // Here the gameport is set up for input from the specified set of joykeys 
		  // Check each key for change
		  kbd_drv_joykey_directions direction;
		  for (direction = 0; direction < MAX_JOYKEY_VALUE; direction++)
			if (symbol_key == kbd_drv_joykey[setting][direction])
			  if( pressed( symbol_key ) || released( symbol_key ))
				kbdEventEOLAdd(kbd_drv_joykey_event[port][pressed(symbol_key)][direction]);
		}
		break;
  }
  
  return (eol_evpos != kbd_state.eventsEOL.inpos) ||
	(eof_evpos != kbd_state.eventsEOF.inpos);
}


/*===========================================================================*/
/* Handle one specific keycode change                                        */
/*===========================================================================*/
    
void kbdDrvKeypress(ULO keycode, BOOL pressed) {
	kbd_drv_pc_symbol symbolic_key = symbolickey(keycode); 
	BOOLE keycode_pressed = pressed;
	BOOLE keycode_was_pressed = prevkeys[keycode];

	/* DEBUG info, not needed now*/
	char szMsg[255];
	sprintf( szMsg, "Keypress %s %s\n"
	, kbdDrvKeyString( symbolic_key )
	, ( pressed ? "pressed" : "released" )
	);
	fellowAddLog( szMsg );

	keys[keycode] = pressed;

	if ((!keycode_pressed) && keycode_was_pressed) {
		// If key is not eaten by a Fellow "event", add it to Amiga kbd queue
		if (!kbdDrvEventChecker(symbolic_key))
		{
			UBY a_code = kbd_drv_pc_symbol_to_amiga_scancode[symbolic_key];
			kbdKeyAdd(kbd_drv_pc_symbol_to_amiga_scancode[symbolic_key] | 0x80);
		}
	}
	else if (keycode_pressed && !keycode_was_pressed) {
		// If key is not eaten by a Fellow "event", add it to Amiga kbd queue
		if (!kbdDrvEventChecker(symbolic_key))
		{
			UBY a_code = kbd_drv_pc_symbol_to_amiga_scancode[symbolic_key];
			kbdKeyAdd(a_code);
		}
	}
	prevkeys[keycode] = pressed;
}


/*===========================================================================*/
/* Keyboard keypress handler                                                 */
/*===========================================================================*/

void kbdDrvBufferOverflowHandler(void) {
}


/*===========================================================================*/
/* Keyboard keypress handler                                                 */
/*===========================================================================*/

void kbdDrvKeypressHandler(void)
{
  DIDEVICEOBJECTDATA rgod[DINPUT_BUFFERSIZE];
  DWORD itemcount = DINPUT_BUFFERSIZE;
  HRESULT res;
		
  if( !kbd_drv_active)
	return;
  
  do {
	res = IDirectInputDevice_GetDeviceData(kbd_drv_lpDID,
	  sizeof(DIDEVICEOBJECTDATA),
	  rgod,
	  &itemcount,
	  0);
	if (res == DIERR_INPUTLOST) kbdDrvDInputAcquire();
  }
  while (res == DIERR_INPUTLOST);
  
  if ((res != DI_OK) && (res != DI_BUFFEROVERFLOW))
		kbdDrvDInputFailure("kbdDrvKeypressHandler(): GetDeviceData() ", res );
  else {
	
		ULO i = 0;
		
		for (i = 0; i < itemcount; i++)
			kbdDrvKeypress( rgod[i].dwOfs, (rgod[i].dwData & 0x80));
  }
}    
    
/*===========================================================================*/
/* Return string describing the given symbolic key                           */
/*===========================================================================*/

STR *kbdDrvKeyString(ULO symbolickey) {
  if (symbolickey >= 106)
    symbolickey = PCK_NONE;
  return kbd_drv_pc_symbol_to_string[symbolickey];
}

STR *kbdDrvKeyPrettyString(ULO symbolickey) {
  if (symbolickey >= 106)
    symbolickey = PCK_NONE;
  return symbol_pretty_name[symbolickey];
}

STR *DikKeyString(int dikkey)
{
  int j;

	for( j = 0; j < PCK_LAST_KEY; j++ )	{
		if( dikkey == symbol_to_DIK_kbddrv[j] )
			return kbdDrvKeyString(j);
	}
	return "UNKNOWN";
}


/*===========================================================================*/
/* Set joystick replacement for a given joystick and direction               */
/*===========================================================================*/

void kbdDrvJoystickReplacementSet(kbd_event event, ULO symbolickey)
{
  switch (event)
  {
  case EVENT_JOY0_UP_ACTIVE:
    kbd_drv_joykey[0][JOYKEY_UP] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY0_DOWN_ACTIVE:
    kbd_drv_joykey[0][JOYKEY_DOWN] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY0_LEFT_ACTIVE:
    kbd_drv_joykey[0][JOYKEY_LEFT] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY0_RIGHT_ACTIVE:
    kbd_drv_joykey[0][JOYKEY_RIGHT] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY0_FIRE0_ACTIVE:
    kbd_drv_joykey[0][JOYKEY_FIRE0] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY0_FIRE1_ACTIVE:
    kbd_drv_joykey[0][JOYKEY_FIRE1] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY0_AUTOFIRE0_ACTIVE:
    kbd_drv_joykey[0][JOYKEY_AUTOFIRE0] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY0_AUTOFIRE1_ACTIVE:
    kbd_drv_joykey[0][JOYKEY_AUTOFIRE1] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY1_UP_ACTIVE:
    kbd_drv_joykey[1][JOYKEY_UP] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY1_DOWN_ACTIVE:
    kbd_drv_joykey[1][JOYKEY_DOWN] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY1_LEFT_ACTIVE:
    kbd_drv_joykey[1][JOYKEY_LEFT] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY1_RIGHT_ACTIVE:
    kbd_drv_joykey[1][JOYKEY_RIGHT] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY1_FIRE0_ACTIVE:
    kbd_drv_joykey[1][JOYKEY_FIRE0] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY1_FIRE1_ACTIVE:
    kbd_drv_joykey[1][JOYKEY_FIRE1] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY1_AUTOFIRE0_ACTIVE:
    kbd_drv_joykey[1][JOYKEY_AUTOFIRE0] = (kbd_drv_pc_symbol) symbolickey;
    break;
  case EVENT_JOY1_AUTOFIRE1_ACTIVE:
    kbd_drv_joykey[1][JOYKEY_AUTOFIRE1] = (kbd_drv_pc_symbol) symbolickey;
    break;
  }
}

/*===========================================================================*/
/* Get joystick replacement for a given joystick and direction               */
/*===========================================================================*/

ULO kbdDrvJoystickReplacementGet(kbd_event event) {
  switch (event) {
  case EVENT_JOY0_UP_ACTIVE:
    return kbd_drv_joykey[0][JOYKEY_UP];
  case EVENT_JOY0_DOWN_ACTIVE:
    return kbd_drv_joykey[0][JOYKEY_DOWN];
  case EVENT_JOY0_LEFT_ACTIVE:
    return kbd_drv_joykey[0][JOYKEY_LEFT];
  case EVENT_JOY0_RIGHT_ACTIVE:
    return kbd_drv_joykey[0][JOYKEY_RIGHT];
  case EVENT_JOY0_FIRE0_ACTIVE:
    return kbd_drv_joykey[0][JOYKEY_FIRE0];
  case EVENT_JOY0_FIRE1_ACTIVE:
    return kbd_drv_joykey[0][JOYKEY_FIRE1];
  case EVENT_JOY0_AUTOFIRE0_ACTIVE:
    return kbd_drv_joykey[0][JOYKEY_AUTOFIRE0];
  case EVENT_JOY0_AUTOFIRE1_ACTIVE:
    return kbd_drv_joykey[0][JOYKEY_AUTOFIRE1];
  case EVENT_JOY1_UP_ACTIVE:
    return kbd_drv_joykey[1][JOYKEY_UP];
  case EVENT_JOY1_DOWN_ACTIVE:
    return kbd_drv_joykey[1][JOYKEY_DOWN];
  case EVENT_JOY1_LEFT_ACTIVE:
    return kbd_drv_joykey[1][JOYKEY_LEFT];
  case EVENT_JOY1_RIGHT_ACTIVE:
    return kbd_drv_joykey[1][JOYKEY_RIGHT];
  case EVENT_JOY1_FIRE0_ACTIVE:
    return kbd_drv_joykey[1][JOYKEY_FIRE0];
  case EVENT_JOY1_FIRE1_ACTIVE:
    return kbd_drv_joykey[1][JOYKEY_FIRE1];
  case EVENT_JOY1_AUTOFIRE0_ACTIVE:
    return kbd_drv_joykey[1][JOYKEY_AUTOFIRE0];
  case EVENT_JOY1_AUTOFIRE1_ACTIVE:
    return kbd_drv_joykey[1][JOYKEY_AUTOFIRE1];
  }
  return PC_NONE;
}


/*===========================================================================*/
/* Hard Reset                                                                */
/*===========================================================================*/

void kbdDrvHardReset(void) {
}


/*===========================================================================*/
/* Emulation Starting                                                        */
/*===========================================================================*/

void kbdDrvEmulationStart(void) {
  ULO port;
  
  kbd_drv_home_pressed = FALSE;
  kbd_drv_end_pressed = FALSE;
  for (port = 0; port < 2; port++) {
    kbd_drv_joykey_enabled[port][0] = (gameport_input[port] == GP_JOYKEY0);
    kbd_drv_joykey_enabled[port][1] = (gameport_input[port] == GP_JOYKEY1);
  }
  
  memset( prevkeys, 0, sizeof( prevkeys ));
  memset( keys, 0, sizeof( keys ));
  
  kbdDrvDInputInitialize();
}


/*===========================================================================*/
/* Emulation Stopping                                                        */
/*===========================================================================*/

void kbdDrvEmulationStop(void) {
  kbdDrvDInputRelease();
}

/*===========================================================================*/
/* Emulation Startup                                                         */
/*===========================================================================*/

void kbdDrvStartup(void) {
  ULO port, setting;
  int i;
  
  kbd_drv_joykey_event[0][0][JOYKEY_UP] = EVENT_JOY0_UP_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_UP] = EVENT_JOY0_UP_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_UP] = EVENT_JOY1_UP_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_UP] = EVENT_JOY1_UP_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_DOWN] = EVENT_JOY0_DOWN_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_DOWN] = EVENT_JOY0_DOWN_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_DOWN] = EVENT_JOY1_DOWN_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_DOWN] = EVENT_JOY1_DOWN_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_LEFT] = EVENT_JOY0_LEFT_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_LEFT] = EVENT_JOY0_LEFT_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_LEFT] = EVENT_JOY1_LEFT_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_LEFT] = EVENT_JOY1_LEFT_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_RIGHT] = EVENT_JOY0_RIGHT_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_RIGHT] = EVENT_JOY0_RIGHT_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_RIGHT] = EVENT_JOY1_RIGHT_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_RIGHT] = EVENT_JOY1_RIGHT_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_FIRE0] = EVENT_JOY0_FIRE0_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_FIRE0] = EVENT_JOY0_FIRE0_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_FIRE0] = EVENT_JOY1_FIRE0_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_FIRE0] = EVENT_JOY1_FIRE0_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_FIRE1] = EVENT_JOY0_FIRE1_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_FIRE1] = EVENT_JOY0_FIRE1_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_FIRE1] = EVENT_JOY1_FIRE1_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_FIRE1] = EVENT_JOY1_FIRE1_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_AUTOFIRE0] = EVENT_JOY0_AUTOFIRE0_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_AUTOFIRE0] = EVENT_JOY0_AUTOFIRE0_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_AUTOFIRE0] = EVENT_JOY1_AUTOFIRE0_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_AUTOFIRE0] = EVENT_JOY1_AUTOFIRE0_ACTIVE;
  kbd_drv_joykey_event[0][0][JOYKEY_AUTOFIRE1] = EVENT_JOY0_AUTOFIRE1_INACTIVE;
  kbd_drv_joykey_event[0][1][JOYKEY_AUTOFIRE1] = EVENT_JOY0_AUTOFIRE1_ACTIVE;
  kbd_drv_joykey_event[1][0][JOYKEY_AUTOFIRE1] = EVENT_JOY1_AUTOFIRE1_INACTIVE;
  kbd_drv_joykey_event[1][1][JOYKEY_AUTOFIRE1] = EVENT_JOY1_AUTOFIRE1_ACTIVE;
  
  kbd_drv_joykey[0][JOYKEY_UP]    = DIK_UP;
  kbd_drv_joykey[0][JOYKEY_DOWN]  = DIK_DOWN;
  kbd_drv_joykey[0][JOYKEY_LEFT]  = DIK_LEFT;
  kbd_drv_joykey[0][JOYKEY_RIGHT] = DIK_RIGHT;
  kbd_drv_joykey[0][JOYKEY_FIRE0] = DIK_RCONTROL;
  kbd_drv_joykey[0][JOYKEY_FIRE1] = DIK_RMENU;
  kbd_drv_joykey[0][JOYKEY_AUTOFIRE0] = DIK_O;
  kbd_drv_joykey[0][JOYKEY_AUTOFIRE1] = DIK_P;
  kbd_drv_joykey[1][JOYKEY_UP]    = DIK_R;
  kbd_drv_joykey[1][JOYKEY_DOWN]  = DIK_F;
  kbd_drv_joykey[1][JOYKEY_LEFT]  = DIK_D;
  kbd_drv_joykey[1][JOYKEY_RIGHT] = DIK_G;
  kbd_drv_joykey[1][JOYKEY_FIRE0] = DIK_LCONTROL;
  kbd_drv_joykey[1][JOYKEY_FIRE1] = DIK_LMENU;
  kbd_drv_joykey[1][JOYKEY_AUTOFIRE0] = DIK_A;
  kbd_drv_joykey[1][JOYKEY_AUTOFIRE1] = DIK_S;
  
  for (port = 0; port < 2; port++)
    for (setting = 0; setting < 2; setting++)
      kbd_drv_joykey_enabled[port][setting] = FALSE;
    
  kbd_drv_capture = FALSE;
  kbd_drv_captured_key = FALSE;
  
  for( i = 0; i < PCK_LAST_KEY; i++ )
    kbddrv_DIK_to_symbol[ i ] = PCK_NONE;
  
  kbddrv_DIK_to_symbol[ DIK_ESCAPE          ] = PCK_ESCAPE; /* First row */
  kbddrv_DIK_to_symbol[ DIK_F1              ] = PCK_F1;
  kbddrv_DIK_to_symbol[ DIK_F2              ] = PCK_F2;
  kbddrv_DIK_to_symbol[ DIK_F3              ] = PCK_F3;
  kbddrv_DIK_to_symbol[ DIK_F4              ] = PCK_F4;
  kbddrv_DIK_to_symbol[ DIK_F5              ] = PCK_F5;
  kbddrv_DIK_to_symbol[ DIK_F6              ] = PCK_F6;
  kbddrv_DIK_to_symbol[ DIK_F7              ] = PCK_F7;
  kbddrv_DIK_to_symbol[ DIK_F8              ] = PCK_F8;
  kbddrv_DIK_to_symbol[ DIK_F9              ] = PCK_F9;
  kbddrv_DIK_to_symbol[ DIK_F10             ] = PCK_F10;
  kbddrv_DIK_to_symbol[ DIK_F11             ] = PCK_F11;
  kbddrv_DIK_to_symbol[ DIK_F12             ] = PCK_F12;
  kbddrv_DIK_to_symbol[ DIK_SYSRQ           ] = PCK_PRINT_SCREEN;
  kbddrv_DIK_to_symbol[ DIK_SCROLL          ] = PCK_SCROLL_LOCK;
  /*kbddrv_DIK_to_symbol[ DIK_PAUSE           ] = PCK_PAUSE; */
  kbddrv_DIK_to_symbol[ DIK_GRAVE           ] = PCK_GRAVE;  /* Second row */
  kbddrv_DIK_to_symbol[ DIK_1               ] = PCK_1;
  kbddrv_DIK_to_symbol[ DIK_2               ] = PCK_2;
  kbddrv_DIK_to_symbol[ DIK_3               ] = PCK_3;
  kbddrv_DIK_to_symbol[ DIK_4               ] = PCK_4;
  kbddrv_DIK_to_symbol[ DIK_5               ] = PCK_5;
  kbddrv_DIK_to_symbol[ DIK_6               ] = PCK_6;
  kbddrv_DIK_to_symbol[ DIK_7               ] = PCK_7;
  kbddrv_DIK_to_symbol[ DIK_8               ] = PCK_8;
  kbddrv_DIK_to_symbol[ DIK_9               ] = PCK_9;
  kbddrv_DIK_to_symbol[ DIK_0               ] = PCK_0;
  kbddrv_DIK_to_symbol[ DIK_MINUS           ] = PCK_MINUS;
  kbddrv_DIK_to_symbol[ DIK_EQUALS          ] = PCK_EQUALS;
  kbddrv_DIK_to_symbol[ DIK_BACK            ] = PCK_BACKSPACE;
  kbddrv_DIK_to_symbol[ DIK_INSERT          ] = PCK_INSERT;
  kbddrv_DIK_to_symbol[ DIK_HOME            ] = PCK_HOME;
  kbddrv_DIK_to_symbol[ DIK_PRIOR           ] = PCK_PAGE_UP;
  kbddrv_DIK_to_symbol[ DIK_NUMLOCK         ] = PCK_NUM_LOCK;
  kbddrv_DIK_to_symbol[ DIK_DIVIDE          ] = PCK_NUMPAD_DIVIDE;
  kbddrv_DIK_to_symbol[ DIK_MULTIPLY        ] = PCK_NUMPAD_MULTIPLY;
  kbddrv_DIK_to_symbol[ DIK_SUBTRACT        ] = PCK_NUMPAD_MINUS;
  kbddrv_DIK_to_symbol[ DIK_TAB             ] = PCK_TAB;  /* Third row */
  kbddrv_DIK_to_symbol[ DIK_Q               ] = PCK_Q;
  kbddrv_DIK_to_symbol[ DIK_W               ] = PCK_W;
  kbddrv_DIK_to_symbol[ DIK_E               ] = PCK_E;
  kbddrv_DIK_to_symbol[ DIK_R               ] = PCK_R;
  kbddrv_DIK_to_symbol[ DIK_T               ] = PCK_T;
  kbddrv_DIK_to_symbol[ DIK_Y               ] = PCK_Y;
  kbddrv_DIK_to_symbol[ DIK_U               ] = PCK_U;
  kbddrv_DIK_to_symbol[ DIK_I               ] = PCK_I;
  kbddrv_DIK_to_symbol[ DIK_O               ] = PCK_O;
  kbddrv_DIK_to_symbol[ DIK_P               ] = PCK_P;
  kbddrv_DIK_to_symbol[ DIK_LBRACKET        ] = PCK_LBRACKET;
  kbddrv_DIK_to_symbol[ DIK_RBRACKET        ] = PCK_RBRACKET;
  kbddrv_DIK_to_symbol[ DIK_RETURN          ] = PCK_RETURN;
  kbddrv_DIK_to_symbol[ DIK_DELETE          ] = PCK_DELETE;
  kbddrv_DIK_to_symbol[ DIK_END             ] = PCK_END;
  kbddrv_DIK_to_symbol[ DIK_NEXT            ] = PCK_PAGE_DOWN;
  kbddrv_DIK_to_symbol[ DIK_NUMPAD7         ] = PCK_NUMPAD_7;
  kbddrv_DIK_to_symbol[ DIK_NUMPAD8         ] = PCK_NUMPAD_8;
  kbddrv_DIK_to_symbol[ DIK_NUMPAD9         ] = PCK_NUMPAD_9;
  kbddrv_DIK_to_symbol[ DIK_ADD             ] = PCK_NUMPAD_PLUS;
  
  kbddrv_DIK_to_symbol[ DIK_CAPITAL         ] = PCK_CAPS_LOCK; /* Fourth row */
  kbddrv_DIK_to_symbol[ DIK_A               ] = PCK_A;
  kbddrv_DIK_to_symbol[ DIK_S               ] = PCK_S;
  kbddrv_DIK_to_symbol[ DIK_D               ] = PCK_D;
  kbddrv_DIK_to_symbol[ DIK_F               ] = PCK_F;
  kbddrv_DIK_to_symbol[ DIK_G               ] = PCK_G;
  kbddrv_DIK_to_symbol[ DIK_H               ] = PCK_H;
  kbddrv_DIK_to_symbol[ DIK_J               ] = PCK_J;
  kbddrv_DIK_to_symbol[ DIK_K               ] = PCK_K;
  kbddrv_DIK_to_symbol[ DIK_L               ] = PCK_L;
  kbddrv_DIK_to_symbol[ DIK_SEMICOLON       ] = PCK_SEMICOLON;
  kbddrv_DIK_to_symbol[ DIK_APOSTROPHE      ] = PCK_APOSTROPHE;
  kbddrv_DIK_to_symbol[ DIK_BACKSLASH       ] = PCK_BACKSLASH;
  kbddrv_DIK_to_symbol[ DIK_NUMPAD4         ] = PCK_NUMPAD_4;
  kbddrv_DIK_to_symbol[ DIK_NUMPAD5         ] = PCK_NUMPAD_5;
  kbddrv_DIK_to_symbol[ DIK_NUMPAD6         ] = PCK_NUMPAD_6;
  
  kbddrv_DIK_to_symbol[ DIK_LSHIFT          ] = PCK_LEFT_SHIFT; /* Fifth row */
  kbddrv_DIK_to_symbol[ 0x56                ] = PCK_NONAME1;
  kbddrv_DIK_to_symbol[ DIK_Z               ] = PCK_Z;
  kbddrv_DIK_to_symbol[ DIK_X               ] = PCK_X;
  kbddrv_DIK_to_symbol[ DIK_C               ] = PCK_C;
  kbddrv_DIK_to_symbol[ DIK_V               ] = PCK_V;
  kbddrv_DIK_to_symbol[ DIK_B               ] = PCK_B;
  kbddrv_DIK_to_symbol[ DIK_N               ] = PCK_N;
  kbddrv_DIK_to_symbol[ DIK_M               ] = PCK_M;
  kbddrv_DIK_to_symbol[ DIK_COMMA           ] = PCK_COMMA;
  kbddrv_DIK_to_symbol[ DIK_PERIOD          ] = PCK_PERIOD;
  kbddrv_DIK_to_symbol[ DIK_SLASH           ] = PCK_SLASH;
  kbddrv_DIK_to_symbol[ DIK_RSHIFT          ] = PCK_RIGHT_SHIFT;
  kbddrv_DIK_to_symbol[ DIK_UP              ] = PCK_UP;
  kbddrv_DIK_to_symbol[ DIK_NUMPAD1         ] = PCK_NUMPAD_1;
  kbddrv_DIK_to_symbol[ DIK_NUMPAD2         ] = PCK_NUMPAD_2;
  kbddrv_DIK_to_symbol[ DIK_NUMPAD3         ] = PCK_NUMPAD_3;
  kbddrv_DIK_to_symbol[ DIK_NUMPADENTER     ] = PCK_NUMPAD_ENTER;
  
  kbddrv_DIK_to_symbol[ DIK_LCONTROL        ] = PCK_LEFT_CTRL; /* Sixth row */
  kbddrv_DIK_to_symbol[ DIK_LWIN            ] = PCK_LEFT_WINDOWS;
  kbddrv_DIK_to_symbol[ DIK_LMENU           ] = PCK_LEFT_ALT;
  kbddrv_DIK_to_symbol[ DIK_SPACE           ] = PCK_SPACE;
  kbddrv_DIK_to_symbol[ DIK_RMENU           ] = PCK_RIGHT_ALT;
  kbddrv_DIK_to_symbol[ DIK_RWIN            ] = PCK_RIGHT_WINDOWS;
  kbddrv_DIK_to_symbol[ DIK_APPS            ] = PCK_START_MENU;
  kbddrv_DIK_to_symbol[ DIK_RCONTROL        ] = PCK_RIGHT_CTRL;
  kbddrv_DIK_to_symbol[ DIK_LEFT            ] = PCK_LEFT;
  kbddrv_DIK_to_symbol[ DIK_DOWN            ] = PCK_DOWN;
  kbddrv_DIK_to_symbol[ DIK_RIGHT           ] = PCK_RIGHT;
  kbddrv_DIK_to_symbol[ DIK_NUMPAD0         ] = PCK_NUMPAD_0;
  kbddrv_DIK_to_symbol[ DIK_DECIMAL         ] = PCK_NUMPAD_DOT;

  fileopsGetGenericFileName(kbd_drv_mapping_filename, "WinFellow", MAPPING_FILENAME);

  prs_rewrite_mapping_file = prsReadFile( kbd_drv_mapping_filename, kbd_drv_pc_symbol_to_amiga_scancode, kbd_drv_joykey );

  for (port = 0; port < 2; port++)
    for (setting = 0; setting < MAX_JOYKEY_VALUE; setting++) {

			kbd_drv_joykey[port][setting] = kbddrv_DIK_to_symbol[kbd_drv_joykey[port][setting]];

#ifdef _DEBUG
			fellowAddLog( "keyplacement port %d direction %d: %s\n", port, setting, kbdDrvKeyString( kbd_drv_joykey[port][setting] ));
#endif

		}

  kbd_drv_active = FALSE;
  kbd_drv_lpDI = NULL;
  kbd_drv_lpDID = NULL;
}

/*===========================================================================*/
/* Emulation Shutdown                                                        */
/*===========================================================================*/

void kbdDrvShutdown(void) {
  if( prs_rewrite_mapping_file )
    prsWriteFile( kbd_drv_mapping_filename, kbd_drv_pc_symbol_to_amiga_scancode, kbd_drv_joykey );
}
