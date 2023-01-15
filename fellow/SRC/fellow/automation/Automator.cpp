#include <sstream>
#include <iomanip>

#include "fellow/api/defs.h"
#include "fellow/api/Drivers.h"
#include "fellow/automation/Automator.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/api/drivers/IGraphicsDriver.h"

using namespace std;
using namespace fellow::api;

void Automator::TakeSnapshot()
{
  if (SnapshotDirectory.empty())
  {
    return;
  }
  _snapshotCounter++;
  if (_snapshotCounter >= SnapshotFrequency)
  {
    _snapshotCounter = 0;
    _snapshotsTaken++;

    ostringstream filename;
    filename << SnapshotDirectory << '\\' << setfill('0') << setw(4) << _snapshotsTaken << '_' << _scheduler->GetRasterFrameCount() + 1 << ".bmp";

    Driver->Graphics->SaveScreenshot(false, filename.str().c_str());
  }
}

void Automator::RecordKey(UBY keyCode)
{
  if (RecordScript)
  {
    _script.RecordKey(keyCode);
  }
}

void Automator::RecordMouse(gameport_inputs mousedev, LON x, LON y, bool button1, bool button2, bool button3)
{
  if (RecordScript)
  {
    _script.RecordMouse(mousedev, x, y, button1, button2, button3);
  }
}

void Automator::RecordJoystick(gameport_inputs joydev, bool left, bool up, bool right, bool down, bool button1, bool button2)
{
  if (RecordScript)
  {
    _script.RecordJoystick(joydev, left, up, right, down, button1, button2);
  }
}

void Automator::RecordEmulatorAction(kbd_event action)
{
  if (RecordScript)
  {
    _script.RecordEmulatorAction(action);
  }
}

void Automator::EndOfLine()
{
  if (!RecordScript)
  {
    ULL frameNumber = _scheduler->GetRasterFrameCount();
    ULO line = _scheduler->GetRasterY();
    _script.ExecuteUntil(frameNumber, line);
  }
}

void Automator::EndOfFrame()
{
  if (SnapshotEnable)
  {
    TakeSnapshot();
  }
}

void Automator::Startup()
{
  if (!RecordScript)
  {
    _script.Load(ScriptFilename);
  }
}

void Automator::Shutdown()
{
  if (RecordScript)
  {
    _script.Save(ScriptFilename);
  }
}

Automator::Automator(Scheduler *scheduler, Keyboard *keyboard)
  : _scheduler(scheduler), _script(keyboard, scheduler), RecordScript(false), SnapshotFrequency(100), SnapshotEnable(false)
{
}
