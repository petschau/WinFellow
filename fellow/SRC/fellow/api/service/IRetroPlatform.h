#pragma once

#include "fellow/api/defs.h"

namespace fellow::api
{
  class IRetroPlatform
  {
  public:
    virtual ~IRetroPlatform() = default;
    virtual bool SendHardDriveContent(const ULO lHardDriveNo, const STR *szImageName, const bool bWriteProtected) = 0;
    virtual bool PostHardDriveLED(const ULO lHardDriveNo, const bool bActive, const bool bWriteActivity) = 0;
    virtual bool PostFloppyDriveLED(const ULO lDriveNo, const bool bActive, const bool bWriteActivity) = 0;
  };
}
