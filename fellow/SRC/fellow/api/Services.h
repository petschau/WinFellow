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
    IHUD &HUD;
    IFSWrapper &FSWrapper;
    ILog &Log;
    IRetroPlatform &RP;
    IPerformanceCounterFactory &PerformanceCounterFactory;
    IFileops &Fileops;

    Services(
        IHUD &hud,
        IFSWrapper &fsWrapper,
        ILog &log,
        IRetroPlatform &retroPlatform,
        IPerformanceCounterFactory &performanceCounterFactory,
        IFileops &fileops)
      : HUD(hud), FSWrapper(fsWrapper), Log(log), RP(retroPlatform), PerformanceCounterFactory(performanceCounterFactory), Fileops(fileops)
    {
    }
  };

  extern Services *Service;
}
