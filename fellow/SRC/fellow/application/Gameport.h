#pragma once

extern bool gameport_left[2];
extern bool gameport_up[2];
extern bool gameport_right[2];
extern bool gameport_down[2];
extern bool gameport_fire0[2];
extern bool gameport_fire1[2];
extern bool gameport_autofire0[2];
extern bool gameport_autofire1[2];

//=====================================
// Types of input devices to a gameport
//=====================================

enum class gameport_inputs
{
  GP_NONE,
  GP_JOYKEY0,
  GP_JOYKEY1,
  GP_ANALOG0,
  GP_ANALOG1,
  GP_MOUSE0,
  GP_MOUSE1,
  RP_JOYSTICK0, // dummy device used to associate joystick input received from RetroPlatform host with gameport
  RP_JOYSTICK1  // dummy device used to associate joystick input received from RetroPlatform host with gameport
};

extern gameport_inputs gameport_input[2];

extern void gameportMouseHandler(gameport_inputs mousedev, LON x, LON y, bool button1, bool button2, bool button3);
extern void gameportJoystickHandler(gameport_inputs joydev, bool left, bool up, bool right, bool down, bool button1, bool button2);
extern void gameportSetInput(ULO index, gameport_inputs gameportinput);

extern bool gameportGetAnalogJoystickInUse();

extern void gameportHardReset();
extern void gameportEmulationStart();
extern void gameportEmulationStop();
extern void gameportStartup();
extern void gameportShutdown();
