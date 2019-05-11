#include "fellow/service/HUD.h"
#include "fellow/api/Services.h"
#include "fellow/api/defs.h"
#include "DRAW.H"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif

using namespace fellow::api;

namespace fellow::service
{
  HUD::HUD()
  {
  }

  void HUD::SetFloppyLED(int driveIndex, bool active, bool write)
  {
    drawSetLED(driveIndex, active);
  }

  void HUD::SetHarddiskLED(int deviceIndex, bool active, bool write)
  {
    drawSetLED(4, active);
    Service->RP.PostHardDriveLED(deviceIndex, active, write);
  }
}
