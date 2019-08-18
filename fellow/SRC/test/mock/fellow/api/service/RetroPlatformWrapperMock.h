#pragma once

#include "fellow/api/service/IRetroPlatform.h"

namespace test::mock::fellow::api
{
  class RetroPlatformWrapperMock : public ::fellow::api::IRetroPlatform
  {
  public:
    bool SendHardDriveContent(const ULO lHardDriveNo, const STR *szImageName, const bool bWriteProtected) override;
    bool PostHardDriveLED(const ULO lHardDriveNo, const bool bActive, const bool bWriteActivity) override;
    bool PostFloppyDriveLED(const ULO lDriveNo, const bool bActive, const bool bWriteActivity) override;
  };
}
