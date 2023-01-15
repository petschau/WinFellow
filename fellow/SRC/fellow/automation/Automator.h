#pragma once

#include "fellow/application/Gameport.h"
#include "fellow/automation/Script.h"

class Automator
{
private:
  Scheduler *_scheduler;
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
  void RecordMouse(gameport_inputs mousedev, LON x, LON y, bool button1, bool button2, bool button3);
  void RecordJoystick(gameport_inputs joydev, bool left, bool up, bool right, bool down, bool button1, bool button2);
  void RecordEmulatorAction(kbd_event action);

  void EndOfLine();
  void EndOfFrame();
  void Startup();
  void Shutdown();

  Automator(Scheduler *scheduler, Keyboard *keyboard);
};
