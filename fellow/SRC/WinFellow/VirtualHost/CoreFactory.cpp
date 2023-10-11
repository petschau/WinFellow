#include "VirtualHost/Core.h"
#include "VirtualHost/CoreFactory.h"
#include "Driver/Sound/DirectSoundDriver.h"

void CoreFactory::CreateDrivers()
{
  _core.Drivers.SoundDriver = new DirectSoundDriver();
}

void CoreFactory::DestroyDrivers()
{
  delete _core.Drivers.SoundDriver;
  _core.Drivers.SoundDriver = nullptr;
}

void CoreFactory::CreateModules()
{
  _core.Sound = new Sound();
}

void CoreFactory::DestroyModules()
{
  delete _core.Sound;
  _core.Sound = nullptr;
}