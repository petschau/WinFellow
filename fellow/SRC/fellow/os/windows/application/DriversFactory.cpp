#include "fellow/os/windows/application/DriversFactory.h"
#include "fellow/os/windows/sound/SoundDriver.h"
#include "fellow/os/windows/graphics/GraphicsDriver.h"
#include "fellow/os/windows/io/JoystickDriver.h"
#include "fellow/os/windows/io/KeyboardDriver.h"
#include "fellow/os/windows/io/MouseDriver.h"
#include "fellow/os/windows/timer/TimerDriver.h"
#include "fellow/os/windows/gui/GuiDriver.h"
#include "fellow/os/windows/ini/IniDriver.h"

using namespace fellow::api;

Drivers *DriversFactory::Create()
{
  return new Drivers(
      new SoundDriver(),
      new GraphicsDriver(),
      new JoystickDriver(),
      new KeyboardDriver(),
      new MouseDriver(),
      new TimerDriver(),
      new GuiDriver(),
      new IniDriver(),
      nullptr);
}

void DriversFactory::Delete(Drivers *drivers)
{
  delete drivers->Sound;
  delete drivers->Graphics;
  delete drivers->Joystick;
  delete drivers->Keyboard;
  delete drivers->Mouse;
  delete drivers->Timer;
  delete drivers->Gui;
  delete drivers->Ini;
  delete drivers->ModRipGui;

  delete drivers;
}
