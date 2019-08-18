#include "test/mock/fellow/api/service/RetroPlatformWrapperMock.h"

namespace test::mock::fellow::api
{
  bool RetroPlatformWrapperMock::SendHardDriveContent(const ULO lHardDriveNo, const STR *szImageName, const bool bWriteProtected)
  {
    return true;
  }

  bool RetroPlatformWrapperMock::PostHardDriveLED(const ULO lHardDriveNo, const bool bActive, const bool bWriteActivity)
  {
    return true;
  }

  bool RetroPlatformWrapperMock::PostFloppyDriveLED(const ULO lDriveNo, const bool bActive, const bool bWriteActivity)
  {
    return true;
  }
}
