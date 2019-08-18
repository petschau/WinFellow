#include "fellow/api/Services.h"
#include "test/mock/fellow/api/service/LogMock.h"
#include "test/mock/fellow/api/service/HUDMock.h"
#include "test/mock/fellow/api/service/FSWrapperMock.h"
#include "test/mock/fellow/api/service/RetroPlatformWrapperMock.h"
#include "test/mock/fellow/api/service/PerformanceCounterMock.h"
#include "test/mock/fellow/api/service/FileopsMock.h"

using namespace fellow::api;
using namespace test::mock::fellow::api;

namespace fellow::api
{
  Services *Service = nullptr;
}

HUDMock hudMock;
FSWrapperMock fsWrapperMock;
LogMock logMock;
RetroPlatformWrapperMock rpMock;
PerformanceCounterFactoryMock performanceCounterFactoryMock;
FileopsMock fileopsMock;

void InitializeTestframework()
{
  Service = new Services(hudMock, fsWrapperMock, logMock, rpMock, performanceCounterFactoryMock, fileopsMock);
}

void ShutdownTestframework()
{
  delete Service;
  Service = nullptr;
}
