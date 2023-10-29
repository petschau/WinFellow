#include "VirtualHost/Core.h"

#include "mock/Core/Service/LogMock.h"
#include "mock/Core/Service/FileInformationMock.h"

#include "mock/Core/Service/HUDMock.h"
#include "mock/Core/Service/RetroPlatformWrapperMock.h"

using namespace fellow::api;
using namespace mock::Core::Service;

namespace fellow::api
{
  Services *Service = nullptr;
}

HUDMock hudMock;
RetroPlatformWrapperMock rpMock;

void InitializeTestframework()
{
  fellow::api::Service = new Services(hudMock, rpMock);

  _core.Log = new LogMock();
  _core.FileInformation = new FileInformationMock();
}

void ShutdownTestframework()
{
  delete _core.FileInformation;
  _core.FileInformation = nullptr;

  delete _core.Log;
  _core.Log = nullptr;

  delete fellow::api::Service;
  fellow::api::Service = nullptr;
}
