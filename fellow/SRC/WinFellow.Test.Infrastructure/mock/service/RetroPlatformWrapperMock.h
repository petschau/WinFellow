#pragma once

#include "Service/IRetroPlatform.h"

namespace mock::Service
{
  class RetroPlatformWrapperMock : public ::Service::IRetroPlatform
  {
  public:
    bool SendHardDriveContent(const uint32_t lHardDriveNo, const char *szImageName, const bool bWriteProtected) override;
  };
}
