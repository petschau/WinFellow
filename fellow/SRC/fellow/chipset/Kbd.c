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

#include "fellow/api/defs.h"
#include "fellow/application/Fellow.h"
#include "fellow/api/Services.h"
#include "KEYCODES.H"
#include "KBD.H"
#include "fellow/application/KeyboardDriver.h"
#include "fellow/application/Gameport.h"
#include "fellow/chipset/Graphics.h"
#include "fellow/chipset/Cia.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/chipset/Floppy.h"
#include "fellow/application/GraphicsDriver.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/debug/log/DebugLogHandler.h"

#include "fellow/automation/Automator.h"

#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#endif

using namespace fellow::api;

/*===========================================================================*/
/* Data collected in struct, for easy locking in IRQs by kbddrv.c            */
/*===========================================================================*/

kbd_state_type kbd_state;
ULO kbd_time_to_wait;

/*===========================================================================*/
/* Stuff that later goes other places                                        */
/*===========================================================================*/

UBY insert_dfX[4]; /* 0 - nothing 1- insert 2-eject */

/* Add an EOL event to the core, used by kbddrv */

void kbdEventEOLAdd(kbd_event eventId)
{
  kbd_state.eventsEOL.buffer[kbd_state.eventsEOL.inpos & KBDBUFFERMASK] = eventId;
  kbd_state.eventsEOL.inpos++;
}

/* Add an EOF event to the core, used by kbddrv */

void kbdEventEOFAdd(kbd_event eventId)
{
  kbd_state.eventsEOF.buffer[kbd_state.eventsEOF.inpos & KBDBUFFERMASK] = eventId;
  kbd_state.eventsEOF.inpos++;
}

/* Add a key to the core, used by kbddrv */

void kbdKeyAdd(UBY keyCode)
{
  automator.RecordKey(keyCode);

  kbd_state.scancodes.buffer[(kbd_state.scancodes.inpos) & KBDBUFFERMASK] = keyCode;
  kbd_state.scancodes.inpos++;
}

void kbdEventDFxIntoDF0(ULO driveNumber)
{
  char tmp[CFG_FILENAME_LENGTH];
  cfg *currentConfig = cfgManagerGetCurrentConfig(&cfg_manager);
  strcpy(tmp, cfgGetDiskImage(currentConfig, 0));
  cfgSetDiskImage(currentConfig, 0, cfgGetDiskImage(currentConfig, driveNumber));
  cfgSetDiskImage(currentConfig, driveNumber, tmp);

  floppySetDiskImage(0, cfgGetDiskImage(currentConfig, 0));
  floppySetDiskImage(driveNumber, cfgGetDiskImage(currentConfig, driveNumber));
}

void kbdEventDebugToggleRenderer()
{
  chipset_info.GfxDebugImmediateRenderingFromConfig = !chipset_info.GfxDebugImmediateRenderingFromConfig;
}

void kbdEventDebugToggleLogging()
{
  DebugLog.Enabled = !DebugLog.Enabled;
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
      case kbd_event::EVENT_INSERT_DF0:
        insert_dfX[0] = 1;
        fellowRequestEmulationStop();
        break;
      case kbd_event::EVENT_INSERT_DF1:
        insert_dfX[1] = 1;
        fellowRequestEmulationStop();
        break;
      case kbd_event::EVENT_INSERT_DF2:
        insert_dfX[2] = 1;
        fellowRequestEmulationStop();
        break;
      case kbd_event::EVENT_INSERT_DF3:
        insert_dfX[3] = 1;
        fellowRequestEmulationStop();
        break;
      case kbd_event::EVENT_EJECT_DF0:
        insert_dfX[0] = 2;
        fellowRequestEmulationStop();
        break;
      case kbd_event::EVENT_EJECT_DF1:
        insert_dfX[1] = 2;
        fellowRequestEmulationStop();
        break;
      case kbd_event::EVENT_EJECT_DF2:
        insert_dfX[2] = 2;
        fellowRequestEmulationStop();
        break;
      case kbd_event::EVENT_EJECT_DF3:
        insert_dfX[3] = 2;
        fellowRequestEmulationStop();
        break;
      case kbd_event::EVENT_DF1_INTO_DF0: kbdEventDFxIntoDF0(1); break;
      case kbd_event::EVENT_DF2_INTO_DF0: kbdEventDFxIntoDF0(2); break;
      case kbd_event::EVENT_DF3_INTO_DF0: kbdEventDFxIntoDF0(3); break;
      case kbd_event::EVENT_DEBUG_TOGGLE_RENDERER: kbdEventDebugToggleRenderer(); break;
      case kbd_event::EVENT_DEBUG_TOGGLE_LOGGING: kbdEventDebugToggleLogging(); break;
      case kbd_event::EVENT_EXIT:
#ifdef RETRO_PLATFORM
        if (RP.GetHeadlessMode())
          RP.SendClose();
        else
#endif
          fellowRequestEmulationStop();

        break;
      case kbd_event::EVENT_HARD_RESET:
        // a reset triggered by keyboard should perform a soft reset and in addition reset the CPU state
        // cpuIntegrationHardReset calls cpuHardReset, which in turn triggers a soft reset
        Service->Log.AddLog("kbd: keyboard-initiated reset triggered...\n");
        cpuIntegrationHardReset();
        break;
      case kbd_event::EVENT_BMP_DUMP: gfxDrvSaveScreenshot(true, ""); break;
    }
    kbd_state.eventsEOF.outpos++;
  }
}

/*===========================================================================*/
/* Called from the main end of line handler                                  */
/*===========================================================================*/

void kbdEventEOLHandler()
{
  kbd_event thisev;
  BOOLE left[2], up[2], right[2], down[2], fire0[2], fire1[2];
  BOOLE left_changed[2], up_changed[2], right_changed[2], down_changed[2];
  BOOLE fire0_changed[2], fire1_changed[2];
  ULO i;

  for (i = 0; i < 2; i++)
    left_changed[i] = up_changed[i] = right_changed[i] = down_changed[i] = fire0_changed[i] = fire1_changed[i] = FALSE;

  while (kbd_state.eventsEOL.outpos < kbd_state.eventsEOL.inpos)
  {
    thisev = (kbd_event)(kbd_state.eventsEOL.buffer[kbd_state.eventsEOL.outpos & KBDBUFFERMASK]);
    switch (thisev)
    {
      case kbd_event::EVENT_JOY0_UP_ACTIVE:
      case kbd_event::EVENT_JOY0_UP_INACTIVE:
        up[0] = (thisev == kbd_event::EVENT_JOY0_UP_ACTIVE);
        up_changed[0] = TRUE;
        break;
      case kbd_event::EVENT_JOY0_DOWN_ACTIVE:
      case kbd_event::EVENT_JOY0_DOWN_INACTIVE:
        down[0] = (thisev == kbd_event::EVENT_JOY0_DOWN_ACTIVE);
        down_changed[0] = TRUE;
        break;
      case kbd_event::EVENT_JOY0_LEFT_ACTIVE:
      case kbd_event::EVENT_JOY0_LEFT_INACTIVE:
        left[0] = (thisev == kbd_event::EVENT_JOY0_LEFT_ACTIVE);
        left_changed[0] = TRUE;
        break;
      case kbd_event::EVENT_JOY0_RIGHT_ACTIVE:
      case kbd_event::EVENT_JOY0_RIGHT_INACTIVE:
        right[0] = (thisev == kbd_event::EVENT_JOY0_RIGHT_ACTIVE);
        right_changed[0] = TRUE;
        break;
      case kbd_event::EVENT_JOY0_FIRE0_ACTIVE:
      case kbd_event::EVENT_JOY0_FIRE0_INACTIVE:
        fire0[0] = (thisev == kbd_event::EVENT_JOY0_FIRE0_ACTIVE);
        fire0_changed[0] = TRUE;
        break;
      case kbd_event::EVENT_JOY0_FIRE1_ACTIVE:
      case kbd_event::EVENT_JOY0_FIRE1_INACTIVE:
        fire1[0] = (thisev == kbd_event::EVENT_JOY0_FIRE1_ACTIVE);
        fire1_changed[0] = TRUE;
        break;
      case kbd_event::EVENT_JOY0_AUTOFIRE0_ACTIVE:
      case kbd_event::EVENT_JOY0_AUTOFIRE0_INACTIVE: gameport_autofire0[0] = (thisev == kbd_event::EVENT_JOY0_AUTOFIRE0_ACTIVE); break;
      case kbd_event::EVENT_JOY0_AUTOFIRE1_ACTIVE:
      case kbd_event::EVENT_JOY0_AUTOFIRE1_INACTIVE: gameport_autofire1[0] = (thisev == kbd_event::EVENT_JOY0_AUTOFIRE1_ACTIVE); break;
      case kbd_event::EVENT_JOY1_UP_ACTIVE:
      case kbd_event::EVENT_JOY1_UP_INACTIVE:
        up[1] = (thisev == kbd_event::EVENT_JOY1_UP_ACTIVE);
        up_changed[1] = TRUE;
        break;
      case kbd_event::EVENT_JOY1_DOWN_ACTIVE:
      case kbd_event::EVENT_JOY1_DOWN_INACTIVE:
        down[1] = (thisev == kbd_event::EVENT_JOY1_DOWN_ACTIVE);
        down_changed[1] = TRUE;
        break;
      case kbd_event::EVENT_JOY1_LEFT_ACTIVE:
      case kbd_event::EVENT_JOY1_LEFT_INACTIVE:
        left[1] = (thisev == kbd_event::EVENT_JOY1_LEFT_ACTIVE);
        left_changed[1] = TRUE;
        break;
      case kbd_event::EVENT_JOY1_RIGHT_ACTIVE:
      case kbd_event::EVENT_JOY1_RIGHT_INACTIVE:
        right[1] = (thisev == kbd_event::EVENT_JOY1_RIGHT_ACTIVE);
        right_changed[1] = TRUE;
        break;
      case kbd_event::EVENT_JOY1_FIRE0_ACTIVE:
      case kbd_event::EVENT_JOY1_FIRE0_INACTIVE:
        fire0[1] = (thisev == kbd_event::EVENT_JOY1_FIRE0_ACTIVE);
        fire0_changed[1] = TRUE;
        break;
      case kbd_event::EVENT_JOY1_FIRE1_ACTIVE:
      case kbd_event::EVENT_JOY1_FIRE1_INACTIVE:
        fire1[1] = (thisev == kbd_event::EVENT_JOY1_FIRE1_ACTIVE);
        fire1_changed[1] = TRUE;
        break;
      case kbd_event::EVENT_JOY1_AUTOFIRE0_ACTIVE:
      case kbd_event::EVENT_JOY1_AUTOFIRE0_INACTIVE: gameport_autofire0[1] = (thisev == kbd_event::EVENT_JOY1_AUTOFIRE0_ACTIVE); break;
      case kbd_event::EVENT_JOY1_AUTOFIRE1_ACTIVE:
      case kbd_event::EVENT_JOY1_AUTOFIRE1_INACTIVE: gameport_autofire1[1] = (thisev == kbd_event::EVENT_JOY1_AUTOFIRE1_ACTIVE); break;
    }
    kbd_state.eventsEOL.outpos++;
  }
  for (i = 0; i < 2; i++)
    if (left_changed[i] || up_changed[i] || right_changed[i] || down_changed[i] || fire0_changed[i] || fire1_changed[i])
    {
      if ((gameport_input[i] == gameport_inputs::GP_JOYKEY0) || (gameport_input[i] == gameport_inputs::GP_JOYKEY1))
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
      ULO scode = kbd_state.scancodes.buffer[kbd_state.scancodes.outpos & KBDBUFFERMASK];
      kbd_state.scancodes.outpos++;
      if (scode != A_NONE)
      {
#ifdef _DEBUG
        Service->Log.AddLog("kbdQueueHandler(): writing scancode 0x%x to CIA and raising interrupt.\n", scode);
#endif
        ciaWritesp(0, (UBY) ~(((scode >> 7) & 1) | (scode << 1)));
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
  ULO i;

  for (i = 0; i < 4; i++)
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
