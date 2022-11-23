#include "fellow/os/null/KeyboardDriver.h"

const char *KeyboardDriver::KeyString(kbd_drv_pc_symbol symbolickey)
{
  return "Unimplemented key string";
}

const char *KeyboardDriver::KeyPrettyString(kbd_drv_pc_symbol symbolickey)
{
  return "Unimplemented pretty key string";
}

void KeyboardDriver::JoystickReplacementSet(kbd_event event, kbd_drv_pc_symbol symbolickey)
{
}

kbd_drv_pc_symbol KeyboardDriver::JoystickReplacementGet(kbd_event event)
{
  return kbd_drv_pc_symbol::PCK_NONE;
}

void KeyboardDriver::KeypressRaw(ULO lRawKeyCode, BOOLE pressed)
{
}

void KeyboardDriver::StateHasChanged(BOOLE active)
{
}

void KeyboardDriver::KeypressHandler()
{
}

#ifdef RETRO_PLATFORM
void KeyboardDriver::EOFHandler()
{
}

void KeyboardDriver::SetJoyKeyEnabled(ULO, ULO, BOOLE)
{
}

#endif

void KeyboardDriver::HardReset()
{
}

void KeyboardDriver::EmulationStart()
{
}

void KeyboardDriver::EmulationStop()
{
}

void KeyboardDriver::Startup()
{
}

void KeyboardDriver::Shutdown()
{
}
