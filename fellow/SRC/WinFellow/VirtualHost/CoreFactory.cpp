#include "VirtualHost/Core.h"
#include "VirtualHost/CoreFactory.h"

#include "Driver/Sound/DirectSoundDriver.h"

#include "Service/Log.h"
#include "Service/FileInformation.h"
#include "Windows/Service/Hud.h"
#include "Windows/Service/RetroPlatformWrapper.h"

#include "hardfile/HardfileHandler.h"

#include "Windows/Service/FileopsWin32.h"

#include "Debug/M68K.h"
#include "Debug/MemorySystem.h"

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
  _core.Sound = new Sound();
  _core.Uart = new Uart();
  _core.RtcOkiMsm6242rs = new RtcOkiMsm6242rs(_core.Log);
  _core.HardfileHandler = new HardfileHandler(*_core.DebugVM.Memory, *_core.DebugVM.CPU, *_core.Log);
}

void CoreFactory::DestroyModules()
{
  delete _core.HardfileHandler;
  _core.HardfileHandler = nullptr;

  delete _core.Uart;
  _core.Uart = nullptr;

  delete _core.Sound;
  _core.Sound = nullptr;

  delete _core.RtcOkiMsm6242rs;
  _core.RtcOkiMsm6242rs = nullptr;
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