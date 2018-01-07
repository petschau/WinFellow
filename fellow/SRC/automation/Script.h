#ifndef SCRIPT_H
#define SCRIPT_H

#include "DEFS.H"
#include "GAMEPORT.H"
#include "KBD.H"

#include <vector>
#include <string>

using namespace std;

struct ScriptLine
{
  ULL FrameNumber;
  ULO LineNumber;
  string Command;
  string Parameters;

  ScriptLine(ULL frameNumber, ULO lineNumber, const string& command, const string& parameters);
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
  UBY GetIdForAction(const string& action);

  void ExecuteMouseCommand(const string& parameters);
  void ExecuteKeyCommand(const string& parameters);
  void ExecuteJoystickCommand(const string& parameters);
  void ExecuteEmulatorActionCommand(const string& parameters);

  void Execute(const ScriptLine& line);

public:
  void RecordKey(UBY keyCode);
  void RecordMouse(gameport_inputs mousedev, LON x, LON y, BOOLE button1, BOOLE button2, BOOLE button3);
  void RecordJoystick(gameport_inputs joydev, BOOLE left, BOOLE up, BOOLE right, BOOLE down, BOOLE button1, BOOLE button2);
  void RecordEmulatorAction(kbd_event action);

  void ExecuteUntil(ULL frameNumber, ULO lineNumber);
  void Load(const string& filename);
  void Save(const string& filename);

  Script();
  ~Script();
};

#endif
