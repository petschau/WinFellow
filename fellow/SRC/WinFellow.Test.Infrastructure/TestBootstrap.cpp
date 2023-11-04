#include "VirtualHost/Core.h"

#include "mock/Service/LogMock.h"
#include "mock/Service/FileInformationMock.h"

#include "mock/Service/HUDMock.h"
#include "mock/Service/RetroPlatformWrapperMock.h"

using namespace mock::Service;

void InitializeTestframework()
{
  _core.Log = new LogMock();
  _core.FileInformation = new FileInformationMock();
  _core.Hud = new HudMock();
  _core.RP = new RetroPlatformWrapperMock();
}

void ShutdownTestframework()
{
  delete _core.FileInformation;
  _core.FileInformation = nullptr;

  delete _core.Log;
  _core.Log = nullptr;

  delete _core.Hud;
  _core.Hud = nullptr;

  delete _core.RP;
  _core.RP = nullptr;
}
