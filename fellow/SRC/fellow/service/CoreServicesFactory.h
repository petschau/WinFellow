#pragma once

#include "fellow/api/Services.h"
#include "fellow/service/HUD.h"
#include "fellow/service/FSWrapper.h"
#include "fellow/service/Log.h"
#include "fellow/service/RetroPlatformWrapper.h"

namespace fellow::service
{

  class CoreServicesFactory
  {
  private:
    static fellow::service::HUD _hud;
    static fellow::service::FSWrapper _fsWrapper;
    static fellow::service::Log _log;
    static fellow::service::RetroPlatformWrapper _retroPlatformWrapper;

  public:
    static fellow::api::Services *Create();
    static void Delete(fellow::api::Services *services);
  };

}
