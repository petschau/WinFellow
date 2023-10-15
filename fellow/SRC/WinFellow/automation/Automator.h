#pragma once

#include "KBD.H"
#include "GAMEPORT.H"
#include "Script.h"

class Automator
{
private:
  Script _script;
  int _snapshotsTaken = 0;
  int _snapshotCounter = 0;

  void TakeSnapshot();

public:
  std::string ScriptFilename;
  bool RecordScript;
  std::string SnapshotDirectory;
  int SnapshotFrequency;
  bool SnapshotEnable;

  void RecordKey(uint8_t keyCode);
  void RecordMouse(gameport_inputs mousedev, int32_t x, int32_t y, BOOLE button1, BOOLE button2, BOOLE button3);
  void RecordJoystick(gameport_inputs joydev, BOOLE left, BOOLE up, BOOLE right, BOOLE down, BOOLE button1, BOOLE button2);
  void RecordEmulatorAction(kbd_event action);

  void EndOfLine();
  void EndOfFrame();
  void Startup();
  void Shutdown();

  Automator();
  ~Automator();
};

extern Automator automator;
