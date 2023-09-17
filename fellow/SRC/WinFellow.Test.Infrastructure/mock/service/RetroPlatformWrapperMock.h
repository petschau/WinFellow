#pragma once

#include "fellow/api/service/IRetroPlatform.h"

namespace test::mock::fellow::api::service
{
  class RetroPlatformWrapperMock : public ::fellow::api::service::IRetroPlatform
  {
  public:
    bool SendHardDriveContent(const uint32_t lHardDriveNo, const STR *szImageName, const bool bWriteProtected) override;
    bool PostHardDriveLED(const uint32_t lHardDriveNo, const bool bActive, const bool bWriteActivity) override;
  };
}
