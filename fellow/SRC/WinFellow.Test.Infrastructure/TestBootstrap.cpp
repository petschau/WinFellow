#include "fellow/api/Services.h"
#include "VirtualHost/Core.h"

#include "mock/Core/Service/LogMock.h"

#include "mock/service/HUDMock.h"
#include "mock/service/FSWrapperMock.h"
#include "mock/service/RetroPlatformWrapperMock.h"


using namespace fellow::api;
using namespace test::mock::fellow::api::service;
using namespace mock::Core::Service;

namespace fellow::api
{
  Services *Service = nullptr;
}

HUDMock hudMock;
FSWrapperMock fsWrapperMock;
RetroPlatformWrapperMock rpMock;

void InitializeTestframework()
{
  fellow::api::Service = new Services(hudMock, fsWrapperMock, rpMock);

  _core.Log = new LogMock();

}

void ShutdownTestframework()
{
  delete fellow::api::Service;
  fellow::api::Service = nullptr;
}
