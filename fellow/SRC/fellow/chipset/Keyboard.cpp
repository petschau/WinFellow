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
#include "fellow/api/Services.h"
#include "fellow/api/Drivers.h"
#include "fellow/chipset/Keycodes.h"
#include "fellow/chipset/Keyboard.h"
#include "fellow/application/Gameport.h"
#include "fellow/chipset/Cia.h"
#include "fellow/chipset/Floppy.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/debug/log/DebugLogHandler.h"

#include "fellow/automation/Automator.h"

#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#endif

using namespace fellow::api;

// Add an EOL event to the core, used by KeyboardDriver
void Keyboard::EventEOLAdd(kbd_event eventId)
{
  _state.eventsEOL.buffer[_state.eventsEOL.inpos & KBDBUFFERMASK] = eventId;
  _state.eventsEOL.inpos++;
}

// Add an EOF event to the core, used by KeyboardDriver
void Keyboard::EventEOFAdd(kbd_event eventId)
{
  _state.eventsEOF.buffer[_state.eventsEOF.inpos & KBDBUFFERMASK] = eventId;
  _state.eventsEOF.inpos++;
}

// Add a key to the core, used by KeyboardDriver
void Keyboard::KeyAdd(UBY keyCode)
{
  automator.RecordKey(keyCode);

  _state.scancodes.buffer[(_state.scancodes.inpos) & KBDBUFFERMASK] = keyCode;
  _state.scancodes.inpos++;
}

void Keyboard::EventDFxIntoDF0(ULO driveNumber)
{
  char tmp[CFG_FILENAME_LENGTH];
  cfg *currentConfig = cfgManagerGetCurrentConfig(&cfg_manager);
  strcpy(tmp, cfgGetDiskImage(currentConfig, 0));
  cfgSetDiskImage(currentConfig, 0, cfgGetDiskImage(currentConfig, driveNumber));
  cfgSetDiskImage(currentConfig, driveNumber, tmp);

  floppySetDiskImage(0, cfgGetDiskImage(currentConfig, 0));
  floppySetDiskImage(driveNumber, cfgGetDiskImage(currentConfig, driveNumber));
}

void Keyboard::EventDebugToggleRenderer()
{
  chipset_info.GfxDebugImmediateRenderingFromConfig = !chipset_info.GfxDebugImmediateRenderingFromConfig;
}

void Keyboard::EventDebugToggleLogging()
{
  DebugLog.Enabled = !DebugLog.Enabled;
}

//==========================================
// Called from the main end of frame handler
//==========================================

void Keyboard::EventEOFHandler()
{
  while (_state.eventsEOF.outpos < _state.eventsEOF.inpos)
  {
    kbd_event thisev = (kbd_event)(_state.eventsEOF.buffer[_state.eventsEOF.outpos & KBDBUFFERMASK]);

    automator.RecordEmulatorAction(thisev);

    switch (thisev)
    {
      case kbd_event::EVENT_INSERT_DF0:
        _insert_dfX[0] = 1;
        _runtimeEnvironment->RequestEmulationStop();
        break;
      case kbd_event::EVENT_INSERT_DF1:
        _insert_dfX[1] = 1;
        _runtimeEnvironment->RequestEmulationStop();
        break;
      case kbd_event::EVENT_INSERT_DF2:
        _insert_dfX[2] = 1;
        _runtimeEnvironment->RequestEmulationStop();
        break;
      case kbd_event::EVENT_INSERT_DF3:
        _insert_dfX[3] = 1;
        _runtimeEnvironment->RequestEmulationStop();
        break;
      case kbd_event::EVENT_EJECT_DF0:
        _insert_dfX[0] = 2;
        _runtimeEnvironment->RequestEmulationStop();
        break;
      case kbd_event::EVENT_EJECT_DF1:
        _insert_dfX[1] = 2;
        _runtimeEnvironment->RequestEmulationStop();
        break;
      case kbd_event::EVENT_EJECT_DF2:
        _insert_dfX[2] = 2;
        _runtimeEnvironment->RequestEmulationStop();
        break;
      case kbd_event::EVENT_EJECT_DF3:
        _insert_dfX[3] = 2;
        _runtimeEnvironment->RequestEmulationStop();
        break;
      case kbd_event::EVENT_DF1_INTO_DF0: EventDFxIntoDF0(1); break;
      case kbd_event::EVENT_DF2_INTO_DF0: EventDFxIntoDF0(2); break;
      case kbd_event::EVENT_DF3_INTO_DF0: EventDFxIntoDF0(3); break;
      case kbd_event::EVENT_DEBUG_TOGGLE_RENDERER: EventDebugToggleRenderer(); break;
      case kbd_event::EVENT_DEBUG_TOGGLE_LOGGING: EventDebugToggleLogging(); break;
      case kbd_event::EVENT_EXIT:
#ifdef RETRO_PLATFORM
        if (RP.GetHeadlessMode())
          RP.SendClose();
        else
#endif
          _runtimeEnvironment->RequestEmulationStop();

        break;
      case kbd_event::EVENT_HARD_RESET:
        // a reset triggered by keyboard should perform a soft reset and in addition reset the CPU state
        // cpuIntegrationHardReset calls cpuHardReset, which in turn triggers a soft reset
        Service->Log.AddLog("kbd: keyboard-initiated reset triggered...\n");
        cpuIntegrationHardReset();
        break;
      case kbd_event::EVENT_BMP_DUMP: Driver->Graphics->SaveScreenshot(true, ""); break;
      default: break;
    }
    _state.eventsEOF.outpos++;
  }
}

//=========================================
// Called from the main end of line handler
//=========================================

void Keyboard::EventEOLHandler()
{
  bool left[2], up[2], right[2], down[2], fire0[2], fire1[2];
  bool left_changed[2], up_changed[2], right_changed[2], down_changed[2];
  bool fire0_changed[2], fire1_changed[2];

  for (unsigned int i = 0; i < 2; i++)
  {
    left_changed[i] = up_changed[i] = right_changed[i] = down_changed[i] = fire0_changed[i] = fire1_changed[i] = false;
  }

  while (_state.eventsEOL.outpos < _state.eventsEOL.inpos)
  {
    kbd_event thisev = (kbd_event)(_state.eventsEOL.buffer[_state.eventsEOL.outpos & KBDBUFFERMASK]);
    switch (thisev)
    {
      case kbd_event::EVENT_JOY0_UP_ACTIVE:
      case kbd_event::EVENT_JOY0_UP_INACTIVE:
        up[0] = (thisev == kbd_event::EVENT_JOY0_UP_ACTIVE);
        up_changed[0] = true;
        break;
      case kbd_event::EVENT_JOY0_DOWN_ACTIVE:
      case kbd_event::EVENT_JOY0_DOWN_INACTIVE:
        down[0] = (thisev == kbd_event::EVENT_JOY0_DOWN_ACTIVE);
        down_changed[0] = true;
        break;
      case kbd_event::EVENT_JOY0_LEFT_ACTIVE:
      case kbd_event::EVENT_JOY0_LEFT_INACTIVE:
        left[0] = (thisev == kbd_event::EVENT_JOY0_LEFT_ACTIVE);
        left_changed[0] = true;
        break;
      case kbd_event::EVENT_JOY0_RIGHT_ACTIVE:
      case kbd_event::EVENT_JOY0_RIGHT_INACTIVE:
        right[0] = (thisev == kbd_event::EVENT_JOY0_RIGHT_ACTIVE);
        right_changed[0] = true;
        break;
      case kbd_event::EVENT_JOY0_FIRE0_ACTIVE:
      case kbd_event::EVENT_JOY0_FIRE0_INACTIVE:
        fire0[0] = (thisev == kbd_event::EVENT_JOY0_FIRE0_ACTIVE);
        fire0_changed[0] = true;
        break;
      case kbd_event::EVENT_JOY0_FIRE1_ACTIVE:
      case kbd_event::EVENT_JOY0_FIRE1_INACTIVE:
        fire1[0] = (thisev == kbd_event::EVENT_JOY0_FIRE1_ACTIVE);
        fire1_changed[0] = true;
        break;
      case kbd_event::EVENT_JOY0_AUTOFIRE0_ACTIVE:
      case kbd_event::EVENT_JOY0_AUTOFIRE0_INACTIVE: gameport_autofire0[0] = (thisev == kbd_event::EVENT_JOY0_AUTOFIRE0_ACTIVE); break;
      case kbd_event::EVENT_JOY0_AUTOFIRE1_ACTIVE:
      case kbd_event::EVENT_JOY0_AUTOFIRE1_INACTIVE: gameport_autofire1[0] = (thisev == kbd_event::EVENT_JOY0_AUTOFIRE1_ACTIVE); break;
      case kbd_event::EVENT_JOY1_UP_ACTIVE:
      case kbd_event::EVENT_JOY1_UP_INACTIVE:
        up[1] = (thisev == kbd_event::EVENT_JOY1_UP_ACTIVE);
        up_changed[1] = true;
        break;
      case kbd_event::EVENT_JOY1_DOWN_ACTIVE:
      case kbd_event::EVENT_JOY1_DOWN_INACTIVE:
        down[1] = (thisev == kbd_event::EVENT_JOY1_DOWN_ACTIVE);
        down_changed[1] = true;
        break;
      case kbd_event::EVENT_JOY1_LEFT_ACTIVE:
      case kbd_event::EVENT_JOY1_LEFT_INACTIVE:
        left[1] = (thisev == kbd_event::EVENT_JOY1_LEFT_ACTIVE);
        left_changed[1] = true;
        break;
      case kbd_event::EVENT_JOY1_RIGHT_ACTIVE:
      case kbd_event::EVENT_JOY1_RIGHT_INACTIVE:
        right[1] = (thisev == kbd_event::EVENT_JOY1_RIGHT_ACTIVE);
        right_changed[1] = true;
        break;
      case kbd_event::EVENT_JOY1_FIRE0_ACTIVE:
      case kbd_event::EVENT_JOY1_FIRE0_INACTIVE:
        fire0[1] = (thisev == kbd_event::EVENT_JOY1_FIRE0_ACTIVE);
        fire0_changed[1] = true;
        break;
      case kbd_event::EVENT_JOY1_FIRE1_ACTIVE:
      case kbd_event::EVENT_JOY1_FIRE1_INACTIVE:
        fire1[1] = (thisev == kbd_event::EVENT_JOY1_FIRE1_ACTIVE);
        fire1_changed[1] = true;
        break;
      case kbd_event::EVENT_JOY1_AUTOFIRE0_ACTIVE:
      case kbd_event::EVENT_JOY1_AUTOFIRE0_INACTIVE: gameport_autofire0[1] = (thisev == kbd_event::EVENT_JOY1_AUTOFIRE0_ACTIVE); break;
      case kbd_event::EVENT_JOY1_AUTOFIRE1_ACTIVE:
      case kbd_event::EVENT_JOY1_AUTOFIRE1_INACTIVE: gameport_autofire1[1] = (thisev == kbd_event::EVENT_JOY1_AUTOFIRE1_ACTIVE); break;
      default: break;
    }

    _state.eventsEOL.outpos++;
  }

  for (unsigned int i = 0; i < 2; i++)
  {
    if (left_changed[i] || up_changed[i] || right_changed[i] || down_changed[i] || fire0_changed[i] || fire1_changed[i])
    {
      if ((gameport_input[i] == gameport_inputs::GP_JOYKEY0) || (gameport_input[i] == gameport_inputs::GP_JOYKEY1))
      {
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
  }
}

//=======================================
// Move a scancode to the CIA SP register
// Called from end_of_line
//=======================================

void Keyboard::QueueHandler()
{
  if (_state.scancodes.outpos < _state.scancodes.inpos)
  {
    if (--_time_to_wait <= 0)
    {
      _time_to_wait = 10;
      ULO scode = _state.scancodes.buffer[_state.scancodes.outpos & KBDBUFFERMASK];
      _state.scancodes.outpos++;
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

void Keyboard::HardReset()
{
  _state.eventsEOL.inpos = 0;
  _state.eventsEOL.outpos = 0;
  _state.eventsEOF.inpos = 0;
  _state.eventsEOF.outpos = 0;
  _state.scancodes.inpos = 2;
  _state.scancodes.outpos = 0;
  _state.scancodes.buffer[0] = 0xfd; // Start of keys pressed on reset
  _state.scancodes.buffer[1] = 0xfe; // End of keys pressed during reset
  _time_to_wait = 10;
  Driver->Keyboard->HardReset();
}

void Keyboard::StartEmulation()
{
  for (unsigned int i = 0; i < 4; i++)
  {
    _insert_dfX[i] = FALSE;
  }

  Driver->Keyboard->EmulationStart();
}

void Keyboard::StopEmulation()
{
  Driver->Keyboard->EmulationStop();
}

void Keyboard::Startup()
{
}

void Keyboard::Shutdown()
{
}

Keyboard::Keyboard(IRuntimeEnvironment *runtimeEnvironment) : _runtimeEnvironment(runtimeEnvironment)
{
}
