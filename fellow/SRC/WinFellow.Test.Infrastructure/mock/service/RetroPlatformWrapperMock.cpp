#include "mock/service/RetroPlatformWrapperMock.h"

namespace test::mock::fellow::api::service
{
  bool RetroPlatformWrapperMock::SendHardDriveContent(const uint32_t lHardDriveNo, const char *szImageName, const bool bWriteProtected)
  {
    return true;
  }

  bool RetroPlatformWrapperMock::PostHardDriveLED(const uint32_t lHardDriveNo, const bool bActive, const bool bWriteActivity)
  {
    return true;
  }
}
