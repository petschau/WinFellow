#ifndef FELLOW_SERVICE_RETROPLATFORMWRAPPER_H
#define FELLOW_SERVICE_RETROPLATFORMWRAPPER_H

#include "fellow/api/service/IRetroPlatform.h"

namespace fellow::service
{
  class RetroPlatformWrapper : public fellow::api::service::IRetroPlatform
  {
  public:
    bool SendHardDriveContent(const uint32_t lHardDriveNo, const STR *szImageName, const bool bWriteProtected) override;
    bool PostHardDriveLED(const uint32_t lHardDriveNo, const bool bActive, const bool bWriteActivity) override;
  };
}

#endif
