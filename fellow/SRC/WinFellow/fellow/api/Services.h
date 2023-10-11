#pragma once

#include "fellow/api/service/IHUD.h"
#include "fellow/api/service/IFSWrapper.h"
#include "fellow/api/service/IRetroPlatform.h"

namespace fellow::api
{
  class Services
  {
  public:
    fellow::api::service::IHUD& HUD;
    fellow::api::service::IFSWrapper& FSWrapper;
    fellow::api::service::IRetroPlatform& RP;

    // Must be inline to allow tests etc to create it with mocks without referencing the core
    Services(fellow::api::service::IHUD& hud, fellow::api::service::IFSWrapper& fsWrapper, fellow::api::service::IRetroPlatform& retroPlatform)
      : HUD(hud), FSWrapper(fsWrapper), RP(retroPlatform)
    {
    }
  };

  extern Services* Service;
}
