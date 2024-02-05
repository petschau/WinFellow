#include "VirtualHost/Core.h"

// For the time being this has to be a singleton
Core _core = Core();

Core::Core()
  : Registers(),
    CopperRegisters(nullptr),
    RegisterUtility(Registers),
    Sound(nullptr),
    Uart(nullptr),
    RtcOkiMsm6242rs(nullptr),
    Clocks(nullptr),
    Events(nullptr),
    Scheduler(nullptr),
    Cpu(nullptr),
    Agnus(nullptr),
    Cia(nullptr),
    Memory(nullptr),
    LineExactCopper(nullptr),
    CycleExactCopper(nullptr),
    CurrentCopper(nullptr),
    LineExactSprites(nullptr),
    CycleExactSprites(nullptr),
    CurrentSprites(nullptr),
    Blitter(nullptr),
    Paula(nullptr),
    HardfileHandler(nullptr),
    Drivers(),
    DebugLog(nullptr),
    Log(nullptr),
    Fileops(nullptr),
    FileInformation(nullptr),
    Hud(nullptr),
    RP(nullptr),
    DebugVM()
{
}

void Core::ConfigureForCycleAccuracy()
{
  CurrentSprites = CycleExactSprites;
  CurrentCopper = CycleExactCopper;
}

void Core::ConfigureForLineAccuracy()
{
  CurrentSprites = LineExactSprites;
  CurrentCopper = LineExactCopper;
}

void Core::ConfigureAccuracy(bool cycleAccuracy)
{
  if (cycleAccuracy)
  {
    ConfigureForCycleAccuracy();
  }
  else
  {
    ConfigureForLineAccuracy();
  }
}

Core::~Core()
{
}
