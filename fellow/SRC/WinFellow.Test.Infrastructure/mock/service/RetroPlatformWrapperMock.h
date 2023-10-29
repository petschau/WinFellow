#pragma once

#include "Service/IRetroPlatform.h"

namespace test::mock::fellow::api::service
{
  class RetroPlatformWrapperMock : public ::Service::IRetroPlatform
  {
  public:
    bool SendHardDriveContent(const uint32_t lHardDriveNo, const char *szImageName, const bool bWriteProtected) override;
    bool PostHardDriveLED(const uint32_t lHardDriveNo, const bool bActive, const bool bWriteActivity) override;
  };
}
