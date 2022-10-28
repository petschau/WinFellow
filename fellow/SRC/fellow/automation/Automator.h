#pragma once

#include "fellow/application/Gameport.h"
#include "fellow/automation/Script.h"

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

  void RecordKey(UBY keyCode);
  void RecordMouse(gameport_inputs mousedev, LON x, LON y, BOOLE button1, BOOLE button2, BOOLE button3);
  void RecordJoystick(gameport_inputs joydev, BOOLE left, BOOLE up, BOOLE right, BOOLE down, BOOLE button1, BOOLE button2);
  void RecordEmulatorAction(kbd_event action);

  void EndOfLine();
  void EndOfFrame();
  void Startup();
  void Shutdown();

  Automator();
};

extern Automator automator;
