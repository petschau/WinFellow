#pragma once

#include "fellow/api/defs.h"
#include "fellow/chipset/Keycodes.h"
#include "fellow/chipset/Keyboard.h"

constexpr unsigned int MAX_JOYKEY_VALUE = 8;

class IKeyboardDriver
{
public:
  virtual kbd_drv_pc_symbol GetPCSymbolFromDescription(const char *pcSymbolDescription) = 0;
  virtual const char *GetPCSymbolDescription(kbd_drv_pc_symbol symbolickey) = 0;
  virtual const char *GetPCSymbolPrettyDescription(kbd_drv_pc_symbol symbolickey) = 0;
  virtual kbd_drv_pc_symbol GetPCSymbolFromDIK(int dikkey) = 0;

  virtual void JoystickReplacementSet(kbd_event event, kbd_drv_pc_symbol symbolickey) = 0;
  virtual kbd_drv_pc_symbol JoystickReplacementGet(kbd_event event) = 0;
  virtual void KeypressRaw(ULO lRawKeyCode, bool pressed) = 0;
  virtual void StateHasChanged(bool active) = 0;
  virtual void KeypressHandler() = 0;

#ifdef RETRO_PLATFORM
  virtual void EOFHandler() = 0;
  virtual void SetJoyKeyEnabled(ULO lGameport, ULO lSetting, bool bEnabled) = 0;
#endif

  virtual void HardReset() = 0;

  virtual void EmulationStart() = 0;
  virtual void EmulationStop() = 0;

  virtual bool GetInitializationFailed() = 0;
  virtual void Initialize(Keyboard *keyboard) = 0;
  virtual void Release() = 0;

  IKeyboardDriver() = default;
  virtual ~IKeyboardDriver() = default;
};
