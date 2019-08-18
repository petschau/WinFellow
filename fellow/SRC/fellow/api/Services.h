#pragma once

#include "fellow/api/service/IHUD.h"
#include "fellow/api/service/IFSWrapper.h"
#include "fellow/api/service/ILog.h"
#include "fellow/api/service/IRetroPlatform.h"
#include "fellow/api/service/IPerformanceCounter.h"
#include "fellow/api/service/IFileops.h"

namespace fellow::api
{
  class Services
  {
  public:
    fellow::api::IHUD &HUD;
    fellow::api::IFSWrapper &FSWrapper;
    fellow::api::ILog &Log;
    fellow::api::IRetroPlatform &RP;
    fellow::api::IPerformanceCounterFactory &PerformanceCounterFactory;
    fellow::api::IFileops &Fileops;

    Services(fellow::api::IHUD &hud, fellow::api::IFSWrapper &fsWrapper, fellow::api::ILog &log, fellow::api::IRetroPlatform &retroPlatform,
             fellow::api::IPerformanceCounterFactory &performanceCounterFactory, fellow::api::IFileops &fileops)
        : HUD(hud), FSWrapper(fsWrapper), Log(log), RP(retroPlatform), PerformanceCounterFactory(performanceCounterFactory), Fileops(fileops)
    {
    }
  };

  extern Services *Service;
}
