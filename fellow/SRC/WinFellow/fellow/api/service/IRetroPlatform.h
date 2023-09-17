#ifndef FELLOW_API_SERVICE_IRETROPLATFORM_H
#define FELLOW_API_SERVICE_IRETROPLATFORM_H

#include "fellow/api/defs.h"

namespace fellow::api::service
{
  class IRetroPlatform
  {
  public:
    virtual ~IRetroPlatform() = default;
    virtual bool SendHardDriveContent(const uint32_t lHardDriveNo, const char *szImageName, const bool bWriteProtected) = 0;
    virtual bool PostHardDriveLED(const uint32_t lHardDriveNo, const bool bActive, const bool bWriteActivity) = 0;
  };
}

#endif
