#pragma once

#include "Defs.h"
#include "GAMEPORT.H"
#include "KBD.H"

#include <vector>
#include <string>

struct ScriptLine
{
  uint64_t FrameNumber;
  uint32_t LineNumber;
  std::string Command;
  std::string Parameters;

  ScriptLine(uint64_t frameNumber, uint32_t lineNumber, const std::string &command, const std::string &parameters);
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

  std::string GetStringForAction(kbd_event action);
  uint8_t GetIdForAction(const std::string &action);

  void ExecuteMouseCommand(const std::string &parameters);
  void ExecuteKeyCommand(const std::string &parameters);
  void ExecuteJoystickCommand(const std::string &parameters);
  void ExecuteEmulatorActionCommand(const std::string &parameters);

  void Execute(const ScriptLine &line);

public:
  void RecordKey(uint8_t keyCode);
  void RecordMouse(gameport_inputs mousedev, int32_t x, int32_t y, BOOLE button1, BOOLE button2, BOOLE button3);
  void RecordJoystick(gameport_inputs joydev, BOOLE left, BOOLE up, BOOLE right, BOOLE down, BOOLE button1, BOOLE button2);
  void RecordEmulatorAction(kbd_event action);

  void ExecuteUntil(uint64_t frameNumber, uint32_t lineNumber);
  void Load(const std::string &filename);
  void Save(const std::string &filename);

  Script();
  ~Script();
};
