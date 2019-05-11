#include "fellow/api/Services.h"
#include "test/mock/fellow/api/service/LogMock.h"
#include "test/mock/fellow/api/service/HUDMock.h"
#include "test/mock/fellow/api/service/FSWrapperMock.h"
#include "test/mock/fellow/api/service/RetroPlatformWrapperMock.h"

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
