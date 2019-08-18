#include "fellow/service/CoreServicesFactory.h"
#include "OSServicesFactory.h"

using namespace fellow::api;
using namespace fellow::os;

namespace fellow::service
{
  HUD CoreServicesFactory::_hud;
  FSWrapper CoreServicesFactory::_fsWrapper;
  Log CoreServicesFactory::_log;
  RetroPlatformWrapper CoreServicesFactory::_retroPlatformWrapper;

  Services *CoreServicesFactory::Create()
  {
    return new Services(_hud, _fsWrapper, _log, _retroPlatformWrapper, GetIPerformanceCounterFactoryImplementation(), GetIFileopsImplementation());
  }

  void CoreServicesFactory::Delete(Services *services)
  {
    delete services;
  }
}
