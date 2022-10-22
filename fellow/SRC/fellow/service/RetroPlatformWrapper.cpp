#include "fellow/service/RetroPlatformWrapper.h"

#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#endif

namespace fellow::service
{
  bool RetroPlatformWrapper::SendHardDriveContent(const ULO lHardDriveNo, const STR *szImageName, const bool bWriteProtected)
  {
#ifdef RETRO_PLATFORM
    if (RP.GetHeadlessMode()) return RP.SendHardDriveContent(lHardDriveNo, szImageName, bWriteProtected);
#endif
    return true;
  }

  bool RetroPlatformWrapper::PostHardDriveLED(const ULO lHardDriveNo, const bool bActive, const bool bWriteActivity)
  {

#ifdef RETRO_PLATFORM
    if (RP.GetHeadlessMode()) return RP.PostHardDriveLED(lHardDriveNo, bActive, bWriteActivity);
#endif
    return true;
  }

  bool RetroPlatformWrapper::PostFloppyDriveLED(const ULO lDriveNo, const bool bActive, const bool bWriteActivity)
  {
#ifdef RETRO_PLATFORM
    if (RP.GetHeadlessMode()) RP.PostFloppyDriveLED(lDriveNo, bActive, bWriteActivity);
#endif
    return true;
  }

}
