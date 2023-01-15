#pragma once

#include "fellow/api/defs.h"
#include "fellow/application/Gameport.h"
#include "fellow/chipset/Keyboard.h"
#include "fellow/automation/ScriptLine.h"

#include <vector>
#include <string>

#include "fellow/scheduler/Scheduler.h"

class Script
{
private:
  Keyboard *_keyboard;
  Scheduler *_scheduler;

  const char *KeyCommand = "Key";
  const char *MouseCommand = "Mouse";
  const char *JoystickCommand = "Joystick";
  const char *EmulatorActionCommand = "EmulatorAction";

  unsigned int _nextLine;
  std::vector<ScriptLine> _lines;

  std::string GetStringForAction(kbd_event action) const;
  kbd_event GetKbdEventForAction(const std::string &action) const;

  unsigned int BoolToUnsignedInt(bool value);
  bool BoolFromUnsignedInt(unsigned int value);

  void ExecuteMouseCommand(const std::string &parameters);
  void ExecuteKeyCommand(const std::string &parameters);
  void ExecuteJoystickCommand(const std::string &parameters);
  void ExecuteEmulatorActionCommand(const std::string &parameters);

  void Execute(const ScriptLine &line);

public:
  void RecordKey(UBY keyCode);
  void RecordMouse(gameport_inputs mousedev, LON x, LON y, bool button1, bool button2, bool button3);
  void RecordJoystick(gameport_inputs joydev, bool left, bool up, bool right, bool down, bool button1, bool button2);
  void RecordEmulatorAction(kbd_event action);

  void ExecuteUntil(ULL frameNumber, ULO lineNumber);
  void Load(const std::string &filename);
  void Save(const std::string &filename);

  Script(Keyboard *keyboard, Scheduler *scheduler);
};
