/*=========================================================================*/
/* Fellow                                                                  */
/* Joystick and Mouse                                                      */
/*                                                                         */
/* Author: Petter Schau                                                    */
/*         Marco Nova (novamarco@hotmail.com)                              */
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

#include "fellow/api/defs.h"
#include "fellow/api/Services.h"
#include "fellow/api/Drivers.h"
#include "fellow/memory/Memory.h"
#include "fellow/application/Gameport.h"
#include "fellow/automation/Automator.h"
#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#endif

using namespace fellow::api;

//=============
// IO-Registers
//=============

ULO potgor, potdat[2];

//===============
// Gameport state
//===============

bool gameport_fire0[2], gameport_fire1[2], gameport_autofire0[2], gameport_autofire1[2];
bool gameport_left[2], gameport_right[2], gameport_up[2], gameport_down[2];
LON gameport_x[2], gameport_y[2];
LON gameport_x_last_read[2], gameport_y_last_read[2];
bool gameport_mouse_first_time[2];

//==========================
// Configuration information
//==========================

gameport_inputs gameport_input[2];

void gameportSetInput(ULO index, gameport_inputs gameportinput)
{
  gameport_input[index] = gameportinput;
}

bool gameportGetAnalogJoystickInUse()
{
  for (unsigned int i = 0; i < 2; i++)
  {
    if (gameport_input[i] == gameport_inputs::GP_ANALOG0 || gameport_input[i] == gameport_inputs::GP_ANALOG1)
    {
      return true;      
    }
  }

  return false;
}

//==============================================================
// General joydat calculation
// This routine will try to avoid overflow in the mouse counters
//==============================================================

static ULO rjoydat(ULO i)
{
  if (gameport_input[i] == gameport_inputs::GP_MOUSE0)
  { // Gameport input is mouse
    LON diffx = gameport_x[i] - gameport_x_last_read[i];
    LON diffy = gameport_y[i] - gameport_y_last_read[i];
    
    if (diffx > 127)
    {
      diffx = 127; // Find relative movement
    }
    else if (diffx < -127)
    {
      diffx = -127;
    }

    if (diffy > 127)
    {
      diffy = 127;
    }
    else if (diffy < -127)
    {
      diffy = -127;
    }

    gameport_x_last_read[i] += diffx;
    gameport_y_last_read[i] += diffy;

    return ((gameport_y_last_read[i] & 0xff) << 8) | (gameport_x_last_read[i] & 0xff);
  }
  else
  { // Gameport input is of joystick type
    ULO retval = 0;

    if (gameport_right[i])
    {
      retval |= 3;
    }

    if (gameport_left[i])
    {
      retval |= 0x300;
    }

    if (gameport_up[i])
    {
      retval ^= 0x100;
    }

    if (gameport_down[i])
    {
      retval ^= 1;
    }

    return retval;
  }
}

// JOY0DATR - $A

UWO rjoy0dat(ULO address)
{
  return (UWO)rjoydat(0);
}

// JOY1DATR - $C

UWO rjoy1dat(ULO address)
{
  return (UWO)rjoydat(1);
}

// POT0DATR - $12

UWO rpot0dat(ULO address)
{
  return (UWO)potdat[0];
}

// POT1DATR - $14

UWO rpot1dat(ULO address)
{
  return (UWO)potdat[1];
}

// POTGOR - $16

UWO rpotgor(ULO address)
{
  UWO val = potgor & 0xbbff;

  if (gameport_autofire1[0])
  {
    gameport_fire1[0] = !gameport_fire1[0];
  }

  if (gameport_autofire1[1])
  {
    gameport_fire1[1] = !gameport_fire1[1];
  }

  if (!gameport_fire1[0])
  {
    val |= 0x400;
  }

  if (!gameport_fire1[1])
  {
    val |= 0x4000;
  }

  return val;
}

// JOYTEST $36

void wjoytest(UWO data, ULO address)
{
  for (unsigned int i = 0; i < 2; i++)
  {
    gameport_x[i] = gameport_x_last_read[i] = (BYT)(data & 0xff);
    gameport_y[i] = gameport_y_last_read[i] = (BYT)((data >> 8) & 0xff);
  }
}

//=========================================================================
// Mouse movement handler
// Called by MouseDriverwhenever a change occurs
// The input coordinates are used raw. There can be a granularity problem.
//
// Parameters:
// mouseno                   - mouse 0 or mouse 1
// x, y                      - New relative position of mouse
// button1, button2, button3 - State of the mouse buttons, button2 not used
//=========================================================================

void gameportMouseHandler(gameport_inputs mousedev, LON x, LON y, bool button1, bool button2, bool button3)
{
  automator.RecordMouse(mousedev, x, y, button1, button2, button3);

  for (unsigned int i = 0; i < 2; i++)
  {
    if (gameport_input[i] == mousedev)
    {
      if ((!gameport_fire1[i]) && button3) potdat[i] = (potdat[i] + 0x100) & 0xffff;
      gameport_fire0[i] = button1;
      gameport_fire1[i] = button3;
      gameport_x[i] += x;
      gameport_y[i] += y;
    }
  }
}

//==========================================================
// Joystick movement handler
// Called by joydrv.c or kbddrv.c whenever a change occurs
//
// Parameters:
// left, up, right, down     - New state of the joystick
// button1, button2, button3 - State of the joystick buttons
//==========================================================

void gameportJoystickHandler(gameport_inputs joydev, bool left, bool up, bool right, bool down, bool button1, bool button2)
{
  automator.RecordJoystick(joydev, left, up, right, down, button1, button2);

  for (unsigned int i = 0; i < 2; i++)
  {
    if (gameport_input[i] == joydev)
    {
      if ((!gameport_fire1[i]) && button2)
      {
        potdat[i] = (potdat[i] + 0x100) & 0xffff;
      }

      gameport_fire0[i] = button1;
      gameport_fire1[i] = button2;
      gameport_left[i] = left;
      gameport_up[i] = up;
      gameport_right[i] = right;
      gameport_down[i] = down;

#ifdef RETRO_PLATFORM
#ifndef FELLOW_SUPPORT_RP_API_VERSION_71
      if (RP.GetHeadlessMode())
      {
        ULO lMask = 0;
        if (button1) lMask |= RP_JOYSTICK_BUTTON1;
        if (button2) lMask |= RP_JOYSTICK_BUTTON2;
        if (left) lMask |= RP_JOYSTICK_LEFT;
        if (up) lMask |= RP_JOYSTICK_UP;
        if (right) lMask |= RP_JOYSTICK_RIGHT;
        if (down) lMask |= RP_JOYSTICK_DOWN;

        RP.PostGameportActivity(i, lMask);
      }
#endif
#endif
    }
  }
}

void gameportIOHandlersInstall()
{
  memorySetIoReadStub(0xa, rjoy0dat);
  memorySetIoReadStub(0xc, rjoy1dat);
  memorySetIoReadStub(0x12, rpot0dat);
  memorySetIoReadStub(0x14, rpot1dat);
  memorySetIoReadStub(0x16, rpotgor);
  memorySetIoWriteStub(0x36, wjoytest);
}

void gameportIORegistersClear(bool clear_pot)
{
  if (clear_pot)
  {
    potgor = 0xffff;
  }

  for (unsigned int i = 0; i < 2; i++)
  {
    if (clear_pot)
    {
      potdat[i] = 0;
    }

    gameport_autofire0[i] = false;
    gameport_autofire1[i] = false;
    gameport_fire0[i] = false;
    gameport_fire1[i] = false;
    gameport_left[i] = false;
    gameport_right[i] = false;
    gameport_up[i] = false;
    gameport_down[i] = false;
    gameport_x[i] = 0;
    gameport_y[i] = 0;
    gameport_x_last_read[i] = 0;
    gameport_y_last_read[i] = 0;
    gameport_mouse_first_time[i] = false;
  }
}

void gameportHardReset()
{
  gameportIORegistersClear(true);
  Driver->Mouse->HardReset();
  Driver->Joystick->HardReset();
}

void gameportEmulationStart()
{
  gameportIOHandlersInstall();
  Service->Log.AddLog("gameportEmulationStart()\n");
  Driver->Mouse->EmulationStart();
  Driver->Joystick->EmulationStart();
  gameportIORegistersClear(false);
}

void gameportEmulationStop()
{
  Driver->Joystick->EmulationStop();
  Driver->Mouse->EmulationStop();
}

void gameportStartup()
{
  gameportIORegistersClear(true);

  // -- nova --
  // this is only an initial settings, they will be overrided
  // by the Game port configuration section of cfgManagerConfigurationActivate

  gameportSetInput(0, gameport_inputs::GP_MOUSE0);
  gameportSetInput(1, gameport_inputs::GP_NONE);
}

void gameportShutdown()
{
}
