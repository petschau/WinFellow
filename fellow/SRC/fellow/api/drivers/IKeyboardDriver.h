#pragma once

#include "fellow/api/defs.h"
#include "fellow/chipset/Keycodes.h"
#include "fellow/chipset/Kbd.h"

constexpr unsigned int MAX_KEYS = 256;
constexpr unsigned int MAX_JOYKEY_VALUE = 8;

extern kbd_drv_pc_symbol kbddrv_DIK_to_symbol[MAX_KEYS];

extern const char *symbol_pretty_name[kbd_drv_pc_symbol::PCK_LAST_KEY];

class IKeyboardDriver
{
public:
  virtual const char *KeyString(kbd_drv_pc_symbol symbolickey) = 0;
  virtual const char *KeyPrettyString(kbd_drv_pc_symbol symbolickey) = 0;
  virtual void JoystickReplacementSet(kbd_event event, kbd_drv_pc_symbol symbolickey) = 0;
  virtual kbd_drv_pc_symbol JoystickReplacementGet(kbd_event event) = 0;
  virtual void KeypressRaw(ULO lRawKeyCode, BOOLE pressed) = 0;
  virtual void StateHasChanged(BOOLE active) = 0;
  virtual void KeypressHandler() = 0;

#ifdef RETRO_PLATFORM
  virtual void EOFHandler() = 0;
  virtual void SetJoyKeyEnabled(ULO, ULO, BOOLE) = 0;
#endif

  virtual void HardReset() = 0;
  virtual void EmulationStart() = 0;
  virtual void EmulationStop() = 0;
  virtual void Startup() = 0;
  virtual void Shutdown() = 0;
};
