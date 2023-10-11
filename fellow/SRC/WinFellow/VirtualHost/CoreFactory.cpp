#include "VirtualHost/Core.h"
#include "VirtualHost/CoreFactory.h"
#include "Driver/Sound/DirectSoundDriver.h"
#include "Service/Log.h"

#include "Windows/Service/FileopsWin32.h"

using namespace Service;

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
  _core.Fileops = new FileopsWin32();
  _core.Log = new Log();
}

void CoreFactory::DestroyServices()
{
  delete _core.Log;
  _core.Log = nullptr;

  delete _core.Fileops;
  _core.Fileops = nullptr;
}

void CoreFactory::CreateModules()
{
  _core.Sound = new Sound();
  _core.Uart = new Uart();
}

void CoreFactory::DestroyModules()
{
  delete _core.Uart;
  _core.Uart = nullptr;

  delete _core.Sound;
  _core.Sound = nullptr;
}