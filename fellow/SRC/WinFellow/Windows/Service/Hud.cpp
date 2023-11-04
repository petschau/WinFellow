#include "Windows/Service/Hud.h"
#include "Defs.h"
#include "DRAW.H"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif

Hud::Hud()
{
}

void Hud::SetFloppyLED(int driveIndex, bool active, bool write)
{
  drawSetLED(driveIndex, active);
}

void Hud::SetHarddiskLED(int deviceIndex, bool active, bool write)
{
  drawSetLED(4, active);

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) RP.PostHardDriveLED(deviceIndex, active, write);
#endif
}
