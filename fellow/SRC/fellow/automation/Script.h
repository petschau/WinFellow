#pragma once

#include "fellow/api/defs.h"
#include "fellow/application/Gameport.h"
#include "fellow/chipset/Kbd.h"

#include <vector>
#include <string>

struct ScriptLine
{
  ULL FrameNumber;
  ULO LineNumber;
  std::string Command;
  std::string Parameters;

  ScriptLine(ULL frameNumber, ULO lineNumber, const std::string &command, const std::string &parameters);
};

class Script
{
private:
  const char *KeyCommand = "Key";
  const char *MouseCommand = "Mouse";
  const char *JoystickCommand = "Joystick";
  const char *EmulatorActionCommand = "EmulatorAction";

  unsigned int _nextLine;
  std::vector<ScriptLine> _lines;
  bool _record;

  std::string GetStringForAction(kbd_event action) const;
  kbd_event GetKbdEventForAction(const std::string &action) const;

  void ExecuteMouseCommand(const std::string &parameters);
  void ExecuteKeyCommand(const std::string &parameters);
  void ExecuteJoystickCommand(const std::string &parameters);
  void ExecuteEmulatorActionCommand(const std::string &parameters);

  void Execute(const ScriptLine &line);

public:
  void RecordKey(UBY keyCode);
  void RecordMouse(gameport_inputs mousedev, LON x, LON y, BOOLE button1, BOOLE button2, BOOLE button3);
  void RecordJoystick(gameport_inputs joydev, BOOLE left, BOOLE up, BOOLE right, BOOLE down, BOOLE button1, BOOLE button2);
  void RecordEmulatorAction(kbd_event action);

  void ExecuteUntil(ULL frameNumber, ULO lineNumber);
  void Load(const std::string &filename);
  void Save(const std::string &filename);

  Script();
};
