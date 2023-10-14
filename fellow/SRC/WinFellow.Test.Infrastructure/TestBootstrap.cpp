#include "fellow/api/Services.h"
#include "VirtualHost/Core.h"

#include "mock/Core/Service/LogMock.h"
#include "mock/Core/Service/FSWrapperMock.h"

#include "mock/service/HUDMock.h"
#include "mock/service/RetroPlatformWrapperMock.h"

using namespace fellow::api;
using namespace test::mock::fellow::api::service;
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
  _core.FSWrapper = new FSWrapperMock();
}

void ShutdownTestframework()
{
  delete _core.FSWrapper;
  _core.FSWrapper = nullptr;

  delete _core.Log;
  _core.Log = nullptr;

  delete fellow::api::Service;
  fellow::api::Service = nullptr;
}
