#include "Defs.h"
#include "Automator.h"
#include "BusScheduler.h"
#include "GraphicsDriver.h"
#
Automator automator;

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
    char filename[CFG_FILENAME_LENGTH];
    sprintf(filename, "%s\\Snap%.4d_%I64d.bmp", SnapshotDirectory.c_str(), _snapshotsTaken, busGetRasterFrameCount() + 1);
    gfxDrvSaveScreenshot(false, filename);
  }
}

void Automator::RecordKey(uint8_t keyCode)
{
  if (RecordScript)
  {
    _script.RecordKey(keyCode);
  }
}

void Automator::RecordMouse(gameport_inputs mousedev, int32_t x, int32_t y, BOOLE button1, BOOLE button2, BOOLE button3)
{
  if (RecordScript)
  {
    _script.RecordMouse(mousedev, x, y, button1, button2, button3);
  }
}

void Automator::RecordJoystick(gameport_inputs joydev, BOOLE left, BOOLE up, BOOLE right, BOOLE down, BOOLE button1, BOOLE button2)
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
    uint64_t frameNumber = busGetRasterFrameCount();
    uint32_t line = busGetRasterY();
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

Automator::Automator() : RecordScript(false), SnapshotFrequency(100), SnapshotEnable(false)
{
}

Automator::~Automator()
{
}
