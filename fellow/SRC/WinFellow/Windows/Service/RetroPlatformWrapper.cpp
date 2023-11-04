#include "Windows/Service/RetroPlatformWrapper.h"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif

bool RetroPlatformWrapper::SendHardDriveContent(const uint32_t lHardDriveNo, const char *szImageName, const bool bWriteProtected)
{
#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) return RP.SendHardDriveContent(lHardDriveNo, szImageName, bWriteProtected);
#endif
  return true;
}
