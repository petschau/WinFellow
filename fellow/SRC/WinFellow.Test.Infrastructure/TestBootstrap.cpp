#include "fellow/api/Services.h"
#include "mock/service/LogMock.h"
#include "mock/service/HUDMock.h"
#include "mock/service/FSWrapperMock.h"
#include "mock/service/RetroPlatformWrapperMock.h"

using namespace fellow::api;
using namespace test::mock::fellow::api::service;

namespace fellow::api
{
  Services *Service = nullptr;
}

HUDMock hudMock;
FSWrapperMock fsWrapperMock;
LogMock logMock;
RetroPlatformWrapperMock rpMock;

void InitializeTestframework()
{
  Service = new Services(hudMock, fsWrapperMock, logMock, rpMock);
}

void ShutdownTestframework()
{
  delete Service;
  Service = nullptr;
}
