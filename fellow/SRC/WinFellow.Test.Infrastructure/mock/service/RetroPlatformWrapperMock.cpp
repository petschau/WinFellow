#include "mock/Service/RetroPlatformWrapperMock.h"

namespace mock::Service
{
  bool RetroPlatformWrapperMock::SendHardDriveContent(const uint32_t lHardDriveNo, const char *szImageName, const bool bWriteProtected)
  {
    return true;
  }
}
