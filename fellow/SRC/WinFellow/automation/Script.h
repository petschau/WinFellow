#pragma once

#include "DEFS.H"
#include "GAMEPORT.H"
#include "KBD.H"

#include <vector>
#include <string>

using namespace std;

struct ScriptLine
{
  uint64_t FrameNumber;
  uint32_t LineNumber;
  string Command;
  string Parameters;

  ScriptLine(uint64_t frameNumber, uint32_t lineNumber, const string& command, const string& parameters);
};

class Script
{
private:
  const char *KeyCommand = "Key";
  const char *MouseCommand = "Mouse";
  const char *JoystickCommand = "Joystick";
  const char *EmulatorActionCommand = "EmulatorAction";

  unsigned int _nextLine;
  vector<ScriptLine> _lines;
  bool _record;

  string GetStringForAction(kbd_event action);
  uint8_t GetIdForAction(const string& action);

  void ExecuteMouseCommand(const string& parameters);
  void ExecuteKeyCommand(const string& parameters);
  void ExecuteJoystickCommand(const string& parameters);
  void ExecuteEmulatorActionCommand(const string& parameters);

  void Execute(const ScriptLine& line);

public:
  void RecordKey(uint8_t keyCode);
  void RecordMouse(gameport_inputs mousedev, int32_t x, int32_t y, BOOLE button1, BOOLE button2, BOOLE button3);
  void RecordJoystick(gameport_inputs joydev, BOOLE left, BOOLE up, BOOLE right, BOOLE down, BOOLE button1, BOOLE button2);
  void RecordEmulatorAction(kbd_event action);

  void ExecuteUntil(uint64_t frameNumber, uint32_t lineNumber);
  void Load(const string& filename);
  void Save(const string& filename);

  Script();
  ~Script();
};
