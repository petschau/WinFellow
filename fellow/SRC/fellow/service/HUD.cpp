#include "fellow/service/HUD.h"
#include "fellow/api/Services.h"
#include "fellow/api/defs.h"
#include "fellow/application/HostRenderer.h"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif

using namespace fellow::api;

namespace fellow::service
{
  HUD::HUD()
  {
  }

  HudConfiguration &HUD::GetHudConfiguration()
  {
    return _hudConfiguration;
  }

  void HUD::NotifyFloppyLEDChanged(int driveIndex, bool active, bool write)
  {
    Service->RP.PostFloppyDriveLED(driveIndex, active, write);
  }

  void HUD::NotifyHarddiskLEDChanged(int deviceIndex, bool active, bool write)
  {
    Service->RP.PostHardDriveLED(deviceIndex, active, write);
  }
}
