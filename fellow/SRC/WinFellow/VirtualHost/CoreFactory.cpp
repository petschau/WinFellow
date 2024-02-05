#include "VirtualHost/Core.h"
#include "VirtualHost/CoreFactory.h"
#include "Driver/Sound/DirectSoundDriver.h"
#include "Service/Log.h"
#include "Service/FileInformation.h"
#include "Windows/Service/Hud.h"
#include "Windows/Service/RetroPlatformWrapper.h"
#include "hardfile/HardfileHandler.h"
#include "Windows/Service/FileopsWin32.h"
#include "DebugApi/M68K.h"
#include "DebugApi/MemorySystem.h"
#include "Cpu.h"
#include "Cia.h"
#include "graphics/Agnus.h"
#include "graphics/CycleExactCopper.h"
#include "LineExactCopper.h"
#include "CustomChipset/Sprite/CycleExactSprites.h"
#include "CustomChipset/Sprite/LineExactSprites.h"
#include "Blitter.h"
#include "PaulaInterrupt.h"
#include "MemoryInterface.h"

using namespace Service;
using namespace Debug;
using namespace fellow::hardfile;

void CoreFactory::CreateDrivers()
{
  _core.Drivers.SoundDriver = new DirectSoundDriver();
}

void CoreFactory::DestroyDrivers()
{
  delete _core.Drivers.SoundDriver;
  _core.Drivers.SoundDriver = nullptr;
}

void CoreFactory::CreateServices()
{
  _core.Log = new Log();
  _core.Fileops = new FileopsWin32(_core.Log);
  _core.FileInformation = new FileInformation();
  _core.Hud = new Hud();
  _core.RP = new RetroPlatformWrapper();
}

void CoreFactory::DestroyServices()
{
  delete _core.RP;
  _core.RP = nullptr;

  delete _core.Hud;
  _core.Hud = nullptr;

  delete _core.Fileops;
  _core.Fileops = nullptr;

  delete _core.Log;
  _core.Log = nullptr;

  delete _core.FileInformation;
  _core.FileInformation = nullptr;
}

void CoreFactory::CreateModules()
{
  _core.Events = new SchedulerEvents();
  _core.Clocks = new Clocks();
  _core.CurrentFrameParameters = new FrameParameters();
  _core.DebugLog = new DebugLog(*_core.Clocks, *_core.Fileops);

  _core.Cia = new Cia(_core.Events->ciaEvent);
  _core.Agnus = new Agnus(_core.Events->eolEvent, _core.Events->eofEvent, *_core.Clocks, *_core.CurrentFrameParameters);
  _core.Cpu = new Cpu(_core.Events->cpuEvent, *_core.Clocks);
  _core.Blitter = new Blitter(_core.Events->blitterEvent);
  _core.Paula = new Paula(_core.Events->interruptEvent);
  _core.Memory = new Memory();

  _core.Scheduler = new Scheduler(*_core.Clocks, *_core.Events, *_core.CurrentFrameParameters);

  _core.CopperRegisters = new CopperRegisters(*_core.Memory);
  _core.LineExactCopper =
      new LineExactCopper(*_core.Scheduler, _core.Events->copperEvent, _core.Events->cpuEvent, *_core.CurrentFrameParameters, *_core.Clocks, *_core.CopperRegisters);
  _core.CycleExactCopper = new CycleExactCopper(
      *_core.Scheduler, _core.Events->copperEvent, _core.Events->cpuEvent, *_core.CurrentFrameParameters, *_core.Clocks, *_core.CopperRegisters, *_core.DebugLog);

  _core.SpriteRegisters = new SpriteRegisters(*_core.Memory);
  _core.LineExactSprites = new LineExactSprites(*_core.Clocks, *_core.SpriteRegisters);
  _core.CycleExactSprites = new CycleExactSprites(*_core.SpriteRegisters, _core.RegisterUtility);

  _core.Sound = new Sound();
  _core.Uart = new Uart();
  _core.RtcOkiMsm6242rs = new RtcOkiMsm6242rs(_core.Log);
  _core.HardfileHandler = new HardfileHandler(*_core.DebugVM.Memory, *_core.DebugVM.CPU, *_core.Log);
}

void CoreFactory::DestroyModules()
{
  delete _core.HardfileHandler;
  _core.HardfileHandler = nullptr;

  delete _core.RtcOkiMsm6242rs;
  _core.RtcOkiMsm6242rs = nullptr;

  delete _core.Uart;
  _core.Uart = nullptr;

  delete _core.Sound;
  _core.Sound = nullptr;

  delete _core.CycleExactSprites;
  _core.CycleExactSprites = nullptr;

  delete _core.LineExactSprites;
  _core.LineExactSprites = nullptr;

  _core.CurrentSprites = nullptr;

  delete _core.SpriteRegisters;
  _core.SpriteRegisters = nullptr;

  delete _core.CycleExactCopper;
  _core.CycleExactCopper = nullptr;

  delete _core.LineExactCopper;
  _core.LineExactCopper = nullptr;

  _core.CurrentCopper = nullptr;

  delete _core.Scheduler;
  _core.Scheduler = nullptr;

  delete _core.Paula;
  _core.Paula = nullptr;

  delete _core.Blitter;
  _core.Blitter = nullptr;

  delete _core.Cpu;
  _core.Cpu = nullptr;

  delete _core.Agnus;
  _core.Agnus = nullptr;

  delete _core.Cia;
  _core.Cia = nullptr;

  delete _core.DebugLog;
  _core.DebugLog = nullptr;

  delete _core.CurrentFrameParameters;
  _core.CurrentFrameParameters = nullptr;

  delete _core.Clocks;
  _core.Clocks = nullptr;

  delete _core.Events;
  _core.Events = nullptr;
}

void CoreFactory::CreateDebugVM()
{
  _core.DebugVM.CPU = new M68K();
  _core.DebugVM.Memory = new MemorySystem();
}

void CoreFactory::DestroyDebugVM()
{
  delete _core.DebugVM.Memory;
  _core.DebugVM.Memory = nullptr;

  delete _core.DebugVM.CPU;
  _core.DebugVM.CPU = nullptr;
}