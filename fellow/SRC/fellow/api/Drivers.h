#pragma once

#include "fellow/api/drivers/ISoundDriver.h"
#include "fellow/api/drivers/IGraphicsDriver.h"
#include "fellow/api/drivers/IJoystickDriver.h"
#include "fellow/api/drivers/IKeyboardDriver.h"
#include "fellow/api/drivers/IMouseDriver.h"
#include "fellow/api/drivers/ITimerDriver.h"
#include "fellow/api/drivers/IGuiDriver.h"

namespace fellow::api
{
  class Drivers
  {
  public:
    ISoundDriver &Sound;
    IGraphicsDriver &Graphics;
    IJoystickDriver &Joystick;
    IKeyboardDriver &Keyboard;
    IMouseDriver &Mouse;
    ITimerDriver &Timer;
    IGuiDriver &Gui;

    Drivers(ISoundDriver &sound, IGraphicsDriver &graphics, IJoystickDriver &joystick, IKeyboardDriver &keyboard, IMouseDriver &mouse, ITimerDriver &timer, IGuiDriver &gui)
      : Sound(sound), Graphics(graphics), Joystick(joystick), Keyboard(keyboard), Mouse(mouse), Timer(timer), Gui(gui)
    {
    }
  };

  extern Drivers *Driver;
}