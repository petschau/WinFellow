#include "VirtualHost/Core.h"
#include "VirtualHost/CoreFactory.h"

#include "Driver/Sound/DirectSoundDriver.h"

#include "Service/Log.h"
#include "Service/FileInformation.h"

#include "hardfile/HardfileHandler.h"

#include "Windows/Service/FileopsWin32.h"

using namespace Service;
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
}

void CoreFactory::DestroyServices()
{
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
  _core.HardfileHandler = new HardfileHandler();
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