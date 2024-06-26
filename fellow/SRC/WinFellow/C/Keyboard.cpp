/*=========================================================================*/
/* Fellow Emulator                                                         */
/* Keyboard emulation                                                      */
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
Tuesday, September 19, 2000: nova
- added autofire support
*/

#include "Defs.h"
#include "FellowMain.h"
#include "Keycode.h"
#include "Keyboard.h"
#include "KeyboardDriver.h"
#include "Gameports.h"
#include "GraphicsPipeline.h"
#include "ComplexInterfaceAdapter.h"
#include "Renderer.h"
#include "FloppyDisk.h"
#include "VirtualHost/Core.h"

#include "../automation/Automator.h"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif

/*===========================================================================*/
/* Data collected in struct, for easy locking in IRQs by kbddrv.c            */
/*===========================================================================*/

kbd_state_type kbd_state;
uint32_t kbd_time_to_wait;

/*===========================================================================*/
/* Stuff that later goes other places                                        */
/*===========================================================================*/

uint8_t insert_dfX[4]; /* 0 - nothing 1- insert 2-eject */

/* Add an EOL event to the core, used by kbddrv */

void kbdEventEOLAdd(uint8_t eventId)
{
  kbd_state.eventsEOL.buffer[kbd_state.eventsEOL.inpos & KBDBUFFERMASK] = eventId;
  kbd_state.eventsEOL.inpos++;
}

/* Add an EOF event to the core, used by kbddrv */

void kbdEventEOFAdd(uint8_t eventId)
{
  kbd_state.eventsEOF.buffer[kbd_state.eventsEOF.inpos & KBDBUFFERMASK] = eventId;
  kbd_state.eventsEOF.inpos++;
}

/* Add a key to the core, used by kbddrv */

void kbdKeyAdd(uint8_t keyCode)
{
  automator.RecordKey(keyCode);

  kbd_state.scancodes.buffer[(kbd_state.scancodes.inpos) & KBDBUFFERMASK] = keyCode;
  kbd_state.scancodes.inpos++;
}

void kbdEventDFxIntoDF0(uint32_t driveNumber)
{
  char tmp[CFG_FILENAME_LENGTH];
  cfg *currentConfig = cfgManagerGetCurrentConfig(&cfg_manager);
  strcpy(tmp, cfgGetDiskImage(currentConfig, 0));
  cfgSetDiskImage(currentConfig, 0, cfgGetDiskImage(currentConfig, driveNumber));
  cfgSetDiskImage(currentConfig, driveNumber, tmp);

  floppySetDiskImage(0, cfgGetDiskImage(currentConfig, 0));
  floppySetDiskImage(driveNumber, cfgGetDiskImage(currentConfig, driveNumber));
}

/*===========================================================================*/
/* Called from the main end of frame handler                                 */
/*===========================================================================*/

void kbdEventEOFHandler()
{
  while (kbd_state.eventsEOF.outpos < kbd_state.eventsEOF.inpos)
  {
    kbd_event thisev = (kbd_event)(kbd_state.eventsEOF.buffer[kbd_state.eventsEOF.outpos & KBDBUFFERMASK]);

    automator.RecordEmulatorAction(thisev);

    switch (thisev)
    {
      case EVENT_INSERT_DF0:
        insert_dfX[0] = 1;
        fellowRequestEmulationStop();
        break;
      case EVENT_INSERT_DF1:
        insert_dfX[1] = 1;
        fellowRequestEmulationStop();
        break;
      case EVENT_INSERT_DF2:
        insert_dfX[2] = 1;
        fellowRequestEmulationStop();
        break;
      case EVENT_INSERT_DF3:
        insert_dfX[3] = 1;
        fellowRequestEmulationStop();
        break;
      case EVENT_EJECT_DF0:
        insert_dfX[0] = 2;
        fellowRequestEmulationStop();
        break;
      case EVENT_EJECT_DF1:
        insert_dfX[1] = 2;
        fellowRequestEmulationStop();
        break;
      case EVENT_EJECT_DF2:
        insert_dfX[2] = 2;
        fellowRequestEmulationStop();
        break;
      case EVENT_EJECT_DF3:
        insert_dfX[3] = 2;
        fellowRequestEmulationStop();
        break;
      case EVENT_DF1_INTO_DF0: kbdEventDFxIntoDF0(1); break;
      case EVENT_DF2_INTO_DF0: kbdEventDFxIntoDF0(2); break;
      case EVENT_DF3_INTO_DF0: kbdEventDFxIntoDF0(3); break;
      case EVENT_EXIT:
#ifdef RETRO_PLATFORM
        if (RP.GetHeadlessMode())
          RP.SendClose();
        else
#endif
          fellowRequestEmulationStop();

        break;
      case EVENT_HARD_RESET:
        // a reset triggered by keyboard should perform a soft reset and in addition reset the CPU state
        // cpuIntegrationHardReset calls cpuHardReset, which in turn triggers a soft reset
        _core.Log->AddLog("kbd: keyboard-initiated reset triggered...\n");
        cpuIntegrationHardReset();
        break;
      case EVENT_BMP_DUMP: gfxDrvSaveScreenshot(true, ""); break;
    }
    kbd_state.eventsEOF.outpos++;
  }
}

/*===========================================================================*/
/* Called from the main end of line handler                                  */
/*===========================================================================*/

void kbdEventEOLHandler()
{
  BOOLE left[2], up[2], right[2], down[2], fire0[2], fire1[2];
  BOOLE left_changed[2], up_changed[2], right_changed[2], down_changed[2];
  BOOLE fire0_changed[2], fire1_changed[2];
  uint32_t i;

  for (i = 0; i < 2; i++)
    left_changed[i] = up_changed[i] = right_changed[i] = down_changed[i] = fire0_changed[i] = fire1_changed[i] = FALSE;

  while (kbd_state.eventsEOL.outpos < kbd_state.eventsEOL.inpos)
  {
    kbd_event thisev = (kbd_event)(kbd_state.eventsEOL.buffer[kbd_state.eventsEOL.outpos & KBDBUFFERMASK]);
    switch (thisev)
    {
      case EVENT_JOY0_UP_ACTIVE:
      case EVENT_JOY0_UP_INACTIVE:
        up[0] = (thisev == EVENT_JOY0_UP_ACTIVE);
        up_changed[0] = TRUE;
        //_core.Log->AddLog(up[0] ? "Up0 - pressed\n" : "Up0 - released\n");
        break;
      case EVENT_JOY0_DOWN_ACTIVE:
      case EVENT_JOY0_DOWN_INACTIVE:
        down[0] = (thisev == EVENT_JOY0_DOWN_ACTIVE);
        down_changed[0] = TRUE;
        //_core.Log->AddLog(down[0] ? "Down0 - pressed\n" : "Down0 - released\n");
        break;
      case EVENT_JOY0_LEFT_ACTIVE:
      case EVENT_JOY0_LEFT_INACTIVE:
        left[0] = (thisev == EVENT_JOY0_LEFT_ACTIVE);
        left_changed[0] = TRUE;
        //_core.Log->AddLog(left[0] ? "Left0 - pressed\n" : "Left0 - released\n");
        break;
      case EVENT_JOY0_RIGHT_ACTIVE:
      case EVENT_JOY0_RIGHT_INACTIVE:
        right[0] = (thisev == EVENT_JOY0_RIGHT_ACTIVE);
        right_changed[0] = TRUE;
        //_core.Log->AddLog(right[0] ? "Right0 - pressed\n" : "Right0 - released\n");
        break;
      case EVENT_JOY0_FIRE0_ACTIVE:
      case EVENT_JOY0_FIRE0_INACTIVE:
        fire0[0] = (thisev == EVENT_JOY0_FIRE0_ACTIVE);
        fire0_changed[0] = TRUE;
        //_core.Log->AddLog(fire0[0] ? "Fire00 - pressed\n" : "Fire00 - released\n");
        break;
      case EVENT_JOY0_FIRE1_ACTIVE:
      case EVENT_JOY0_FIRE1_INACTIVE:
        fire1[0] = (thisev == EVENT_JOY0_FIRE1_ACTIVE);
        fire1_changed[0] = TRUE;
        //_core.Log->AddLog(fire1[0] ? "Fire10 - pressed\n" : "Fire10 - released\n");
        break;
      case EVENT_JOY0_AUTOFIRE0_ACTIVE:
      case EVENT_JOY0_AUTOFIRE0_INACTIVE:
        gameport_autofire0[0] = (thisev == EVENT_JOY0_AUTOFIRE0_ACTIVE);
        //_core.Log->AddLog(gameport_autofire0[0] ? "AutoFire00 - pressed\n" : "AutoFire00 - released\n");
        break;
      case EVENT_JOY0_AUTOFIRE1_ACTIVE:
      case EVENT_JOY0_AUTOFIRE1_INACTIVE:
        gameport_autofire1[0] = (thisev == EVENT_JOY0_AUTOFIRE1_ACTIVE);
        //_core.Log->AddLog(gameport_autofire1[0] ? "AutoFire10 - pressed\n" : "AutoFire10 - released\n");
        break;
      case EVENT_JOY1_UP_ACTIVE:
      case EVENT_JOY1_UP_INACTIVE:
        up[1] = (thisev == EVENT_JOY1_UP_ACTIVE);
        up_changed[1] = TRUE;
        //_core.Log->AddLog(up[1] ? "Up1 - pressed\n" : "Up1 - released\n");
        break;
      case EVENT_JOY1_DOWN_ACTIVE:
      case EVENT_JOY1_DOWN_INACTIVE:
        down[1] = (thisev == EVENT_JOY1_DOWN_ACTIVE);
        down_changed[1] = TRUE;
        //_core.Log->AddLog(down[1] ? "Down1 - pressed\n" : "Down1 - released\n");
        break;
      case EVENT_JOY1_LEFT_ACTIVE:
      case EVENT_JOY1_LEFT_INACTIVE:
        left[1] = (thisev == EVENT_JOY1_LEFT_ACTIVE);
        left_changed[1] = TRUE;
        //_core.Log->AddLog(left[1] ? "Left1 - pressed\n" : "Left1 - released\n");
        break;
      case EVENT_JOY1_RIGHT_ACTIVE:
      case EVENT_JOY1_RIGHT_INACTIVE:
        right[1] = (thisev == EVENT_JOY1_RIGHT_ACTIVE);
        right_changed[1] = TRUE;
        //_core.Log->AddLog(right[1] ? "Right1 - pressed\n" : "Right1 - released\n");
        break;
      case EVENT_JOY1_FIRE0_ACTIVE:
      case EVENT_JOY1_FIRE0_INACTIVE:
        fire0[1] = (thisev == EVENT_JOY1_FIRE0_ACTIVE);
        fire0_changed[1] = TRUE;
        //_core.Log->AddLog(fire0[1] ? "Fire01 - pressed\n" : "Fire01 - released\n");
        break;
      case EVENT_JOY1_FIRE1_ACTIVE:
      case EVENT_JOY1_FIRE1_INACTIVE:
        fire1[1] = (thisev == EVENT_JOY1_FIRE1_ACTIVE);
        fire1_changed[1] = TRUE;
        //_core.Log->AddLog(fire1[1] ? "Fire11 - pressed\n" : "Fire11 - released\n");
        break;
      case EVENT_JOY1_AUTOFIRE0_ACTIVE:
      case EVENT_JOY1_AUTOFIRE0_INACTIVE:
        gameport_autofire0[1] = (thisev == EVENT_JOY1_AUTOFIRE0_ACTIVE);
        //_core.Log->AddLog(gameport_autofire0[1] ? "AutoFire01 - pressed\n" : "AutoFire01 - released\n");
        break;
      case EVENT_JOY1_AUTOFIRE1_ACTIVE:
      case EVENT_JOY1_AUTOFIRE1_INACTIVE:
        gameport_autofire1[1] = (thisev == EVENT_JOY1_AUTOFIRE1_ACTIVE);
        //_core.Log->AddLog(gameport_autofire1[1] ? "AutoFire11 - pressed\n" : "AutoFire11 - released\n");
        break;
    }
    kbd_state.eventsEOL.outpos++;
  }
  for (i = 0; i < 2; i++)
    if (left_changed[i] || up_changed[i] || right_changed[i] || down_changed[i] || fire0_changed[i] || fire1_changed[i])
    {
      if ((gameport_input[i] == GP_JOYKEY0) || (gameport_input[i] == GP_JOYKEY1))
        gameportJoystickHandler(
            gameport_input[i],
            (left_changed[i]) ? left[i] : gameport_left[i],
            (up_changed[i]) ? up[i] : gameport_up[i],
            (right_changed[i]) ? right[i] : gameport_right[i],
            (down_changed[i]) ? down[i] : gameport_down[i],
            (fire0_changed[i]) ? fire0[i] : gameport_fire0[i],
            (fire1_changed[i]) ? fire1[i] : gameport_fire1[i]);
    }
}

/*===========================================================================*/
/* Move a scancode to the CIA SP register                                    */
/* Called from end_of_line                                                   */
/*===========================================================================*/

void kbdQueueHandler()
{
  if (kbd_state.scancodes.outpos < kbd_state.scancodes.inpos)
  {
    if (--kbd_time_to_wait <= 0)
    {
      kbd_time_to_wait = 10;
      uint32_t scode = kbd_state.scancodes.buffer[kbd_state.scancodes.outpos & KBDBUFFERMASK];
      kbd_state.scancodes.outpos++;
      if (scode != A_NONE)
      {
#ifdef _DEBUG
        _core.Log->AddLog("   kbdQueueHandler(): writing scancode 0x%x to CIA and raising interrupt.\n", scode);
#endif
        ciaWritesp(0, (uint8_t) ~(((scode >> 7) & 1) | (scode << 1)));
        ciaRaiseIRQ(0, 8);
      }
    }
  }
}

/*===========================================================================*/
/* Fellow standard functions                                                 */
/*===========================================================================*/

void kbdHardReset()
{
  kbd_state.eventsEOL.inpos = 0;
  kbd_state.eventsEOL.outpos = 0;
  kbd_state.eventsEOF.inpos = 0;
  kbd_state.eventsEOF.outpos = 0;
  kbd_state.scancodes.inpos = 2;
  kbd_state.scancodes.outpos = 0;
  kbd_state.scancodes.buffer[0] = 0xfd; /* Start of keys pressed on reset */
  kbd_state.scancodes.buffer[1] = 0xfe; /* End of keys pressed during reset */
  kbd_time_to_wait = 10;
  kbdDrvHardReset();
}

void kbdEmulationStart()
{
  for (uint32_t i = 0; i < 4; i++)
    insert_dfX[i] = FALSE;
  kbdDrvEmulationStart();
}

void kbdEmulationStop()
{
  kbdDrvEmulationStop();
}

void kbdStartup()
{
  kbdDrvStartup();
}

void kbdShutdown()
{
  kbdDrvShutdown();
}
