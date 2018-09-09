#ifndef FELLOW_API_SERVICE_IRETROPLATFORM_H
#define FELLOW_API_SERVICE_IRETROPLATFORM_H

#include "fellow/api/defs.h"

namespace fellow::api::service
{
  class IRetroPlatform
  {
  public:
    virtual bool SendHardDriveContent(const ULO lHardDriveNo, const STR *szImageName, const bool bWriteProtected) = 0;
    virtual bool PostHardDriveLED(const ULO lHardDriveNo, const bool bActive, const bool bWriteActivity) = 0;
  };
}

#endif
