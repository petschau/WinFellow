#pragma once

#include "fellow/api/service/IRetroPlatform.h"

namespace fellow::service
{
  class RetroPlatformWrapper : public fellow::api::service::IRetroPlatform
  {
  public:
    bool SendHardDriveContent(const uint32_t lHardDriveNo, const char *szImageName, const bool bWriteProtected) override;
    bool PostHardDriveLED(const uint32_t lHardDriveNo, const bool bActive, const bool bWriteActivity) override;
  };
}
