#pragma once

#include "fellow/api/drivers/IKeyboardDriver.h"

class KeyboardDriver : public IKeyboardDriver
{
public:
  const char *KeyString(kbd_drv_pc_symbol symbolickey) = 0;
  const char *KeyPrettyString(kbd_drv_pc_symbol symbolickey) = 0;
  void JoystickReplacementSet(kbd_event event, kbd_drv_pc_symbol symbolickey) = 0;
  kbd_drv_pc_symbol JoystickReplacementGet(kbd_event event) = 0;
  void KeypressRaw(ULO lRawKeyCode, BOOLE pressed) = 0;
  void StateHasChanged(BOOLE active) = 0;
  void KeypressHandler() = 0;

#ifdef RETRO_PLATFORM
  void EOFHandler() = 0;
  void SetJoyKeyEnabled(ULO, ULO, BOOLE) = 0;
#endif

  void HardReset() = 0;
  void EmulationStart() = 0;
  void EmulationStop() = 0;
  void Startup() = 0;
  void Shutdown() = 0;
};
