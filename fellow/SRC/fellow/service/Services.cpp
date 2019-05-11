#include "fellow/api/Services.h"
#include "fellow/service/HUD.h"
#include "fellow/service/FSWrapper.h"
#include "fellow/service/Log.h"
#include "fellow/service/RetroPlatformWrapper.h"

using namespace fellow::service;
using namespace fellow::api::service;

namespace fellow::api
{
  HUD hud;
  FSWrapper fsWrapper;
  Log log;
  RetroPlatformWrapper retroPlatformWrapper;

  Services *Service = new Services(hud, fsWrapper, log, retroPlatformWrapper);
}
