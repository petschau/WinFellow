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
/* ---------------- CHANGE LOG ----------------- 
Friday, January 05, 2001: nova
- removed initialization of port 0 to GP_MOUSE0 in gameportEmulationStart

Tuesday, September 19, 2000
- added autofire support
*/

#include "defs.h"
#include "fellow.h"
#include "draw.h"
#include "fmem.h"
#include "gameport.h"
#include "mousedrv.h"
#include "joydrv.h"
#include "../automation/Automator.h"
#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif

/*===========================================================================*/
/* IO-Registers                                                              */
/*===========================================================================*/

ULO potgor, potdat[2];


/*===========================================================================*/
/* Gameport state                                                            */
/*===========================================================================*/

BOOLE gameport_fire0[2], gameport_fire1[2], gameport_autofire0[2], gameport_autofire1[2];
BOOLE gameport_left[2], gameport_right[2], gameport_up[2], gameport_down[2];
LON gameport_x[2], gameport_y[2];
LON gameport_x_last_read[2], gameport_y_last_read[2];
BOOLE gameport_mouse_first_time[2];


/*===========================================================================*/
/* Configuration information                                                 */
/*===========================================================================*/

gameport_inputs gameport_input[2];

void gameportSetInput(ULO index, gameport_inputs gameportinput) {
  gameport_input[index] = gameportinput;
}

BOOLE gameportGetAnalogJoystickInUse(void)
{
  BOOLE result = FALSE;
  ULO i;

  for(i = 0; i < 2; i++)
  {
    if (gameport_input[i] == GP_ANALOG0 || gameport_input[i] == GP_ANALOG1)
      result = TRUE;
  }

  return result;
}

/*===========================================================================*/
/* IO-Registers                                                              */
/*===========================================================================*/

/*===========================================================================*/
/* General joydat calculation                                                */
/* This routine will try to avoid overflow in the mouse counters             */
/*===========================================================================*/

static ULO rjoydat(ULO i) {
  if (gameport_input[i] == GP_MOUSE0) { /* Gameport input is mouse */
    LON diffx, diffy;

    diffx = gameport_x[i] - gameport_x_last_read[i];
    diffy = gameport_y[i] - gameport_y_last_read[i];
    if (diffx > 127)
      diffx = 127;  /* Find relative movement */
    else if (diffx < -127)
      diffx = -127;
    if (diffy > 127)
      diffy = 127;
    else if (diffy < -127)
      diffy = -127;
    gameport_x_last_read[i] += diffx;
    gameport_y_last_read[i] += diffy;

    return ((gameport_y_last_read[i] & 0xff)<<8) | (gameport_x_last_read[i] & 0xff);
  }
  else { /* Gameport input is of joystick type */
    ULO retval = 0;

    if (gameport_right[i])
      retval |= 3;

    if (gameport_left[i])
      retval |= 0x300;

    if (gameport_up[i])
      retval ^= 0x100;

    if (gameport_down[i])
      retval ^= 1;

    return retval;
  }
}

/* JOY0DATR - $A */

UWO rjoy0dat(ULO address)
{
  return (UWO) rjoydat(0);
}

/* JOY1DATR - $C */

UWO rjoy1dat(ULO address)
{
  return (UWO) rjoydat(1);
}

/* POT0DATR - $12 */

UWO rpot0dat(ULO address)
{
  return (UWO) potdat[0];
}

/* POT1DATR - $14 */

UWO rpot1dat(ULO address)
{
  return (UWO) potdat[1];
}

/* POTGOR - $16 */

UWO rpotgor(ULO address)
{
  UWO val = potgor & 0xbbff;

  if( gameport_autofire1[0] )
    gameport_fire1[0] = !gameport_fire1[0];

  if( gameport_autofire1[1] )
    gameport_fire1[1] = !gameport_fire1[1];

  if (!gameport_fire1[0])
    val |= 0x400;
  if (!gameport_fire1[1]) val |= 0x4000;
  return val;
}

/* JOYTEST $36 */

void wjoytest(UWO data, ULO address)
{
  ULO i;

  for (i = 0; i < 2; i++) {
    gameport_x[i] = gameport_x_last_read[i] = (BYT) (data & 0xff);
    gameport_y[i] = gameport_y_last_read[i] = (BYT) ((data>>8) & 0xff);
  }
}

/*===========================================================================*/
/* Mouse movement handler                                                    */
/* Called by mousedrv.c whenever a change occurs                             */
/* The input coordinates are used raw. There can be a granularity problem.   */
/*                                                                           */
/* Parameters:                                                               */
/* mouseno                   - mouse 0 or mouse 1                            */
/* x, y                      - New relative position of mouse                */
/* button1, button2, button3 - State of the mouse buttons, button2 not used  */
/*===========================================================================*/

void gameportMouseHandler(gameport_inputs mousedev,
			  LON x,
			  LON y,
			  BOOLE button1,
			  BOOLE button2,
			  BOOLE button3) {
  ULO i;

  automator.RecordMouse(mousedev, x, y, button1, button2, button3);

  for (i = 0; i < 2; i++) {
    if (gameport_input[i] == mousedev) {
      if ((!gameport_fire1[i]) && button3)
	potdat[i] = (potdat[i] + 0x100) & 0xffff; 
      gameport_fire0[i] = button1;
      gameport_fire1[i] = button3;
      gameport_x[i] += x;
      gameport_y[i] += y;
    }
  }
}

/*===========================================================================*/
/* Joystick movement handler                                                 */
/* Called by joydrv.c or kbddrv.c whenever a change occurs                   */
/*                                                                           */
/* Parameters:                                                               */
/* left, up, right, down     - New state of the joystick                     */
/* button1, button2, button3 - State of the joystick buttons                 */
/*===========================================================================*/

void gameportJoystickHandler(gameport_inputs joydev,
	BOOLE left,
	BOOLE up,
	BOOLE right,
	BOOLE down,
	BOOLE button1,
	BOOLE button2) {
  ULO i;

  automator.RecordJoystick(joydev, left, up, right, down, button1, button2);

  for (i = 0; i < 2; i++)
    if (gameport_input[i] == joydev)
    {
      if ((!gameport_fire1[i]) && button2)
	      potdat[i] = (potdat[i] + 0x100) & 0xffff; 

      gameport_fire0[i] = button1;
      gameport_fire1[i] = button2;
      gameport_left[i] = left;
      gameport_up[i] = up;
      gameport_right[i] = right;
      gameport_down[i] = down;

#ifdef RETRO_PLATFORM
#ifndef FELLOW_SUPPORT_RP_API_VERSION_71
      if(RP.GetHeadlessMode())
      {
        ULO lMask = 0;
        if(button1) lMask |= RP_JOYSTICK_BUTTON1;
        if(button2) lMask |= RP_JOYSTICK_BUTTON2;
        if(left)    lMask |= RP_JOYSTICK_LEFT;
        if(up)      lMask |= RP_JOYSTICK_UP;
        if(right)   lMask |= RP_JOYSTICK_RIGHT;
        if(down)    lMask |= RP_JOYSTICK_DOWN;

        RP.PostGameportActivity(i, lMask);
      }
#endif
#endif
    }
}

/*===========================================================================*/
/* Initialize the register stubs for the gameports                           */
/*===========================================================================*/

void gameportIOHandlersInstall(void)
{
  memorySetIoReadStub(0xa, rjoy0dat);
  memorySetIoReadStub(0xc, rjoy1dat);
  memorySetIoReadStub(0x12, rpot0dat);
  memorySetIoReadStub(0x14, rpot1dat);
  memorySetIoReadStub(0x16, rpotgor);
  memorySetIoWriteStub(0x36, wjoytest);
}

/*===========================================================================*/
/* Clear all gameport state                                                  */
/*===========================================================================*/

void gameportIORegistersClear(BOOLE clear_pot) {
  ULO i;

  if (clear_pot) potgor = 0xffff;
  for (i = 0; i < 2; i++) {
    if (clear_pot) potdat[i] = 0;
    gameport_autofire0[i] = FALSE;
    gameport_autofire1[i] = FALSE;
    gameport_fire0[i] = FALSE;
    gameport_fire1[i] = FALSE;
    gameport_left[i] = FALSE;
    gameport_right[i] = FALSE;
    gameport_up[i] = FALSE;
    gameport_down[i] = FALSE;
    gameport_x[i] = 0;
    gameport_y[i] = 0;
    gameport_x_last_read[i] = 0;
    gameport_y_last_read[i] = 0;
    gameport_mouse_first_time[i] = FALSE;
  }
}


/*===========================================================================*/
/* Standard Fellow module functions                                          */
/*===========================================================================*/

void gameportHardReset(void) {
  gameportIORegistersClear(TRUE);
  mouseDrvHardReset();
  joyDrvHardReset();
}

void gameportEmulationStart(void) {
  gameportIOHandlersInstall();
  fellowAddLog("gameportEmulationStart()\n");
  mouseDrvEmulationStart();
  joyDrvEmulationStart();
  gameportIORegistersClear(FALSE);
}

void gameportEmulationStop(void) {
  joyDrvEmulationStop();
  mouseDrvEmulationStop();
}

void gameportStartup(void) {
  gameportIORegistersClear(TRUE);
  mouseDrvStartup();
  joyDrvStartup();

  // -- nova --
  // this is only an initial settings, they will be overrided
  // by the Game port configuration section of cfgManagerConfigurationActivate

  gameportSetInput(0, GP_MOUSE0);
  gameportSetInput(1, GP_NONE);
}

void gameportShutdown(void) {
  joyDrvShutdown();
  mouseDrvShutdown();
}

