#pragma once

#include <cstdint>

namespace Service
{
  class IRetroPlatform
  {
  public:
    virtual ~IRetroPlatform() = default;
    virtual bool SendHardDriveContent(const uint32_t lHardDriveNo, const char *szImageName, const bool bWriteProtected) = 0;
    virtual bool PostHardDriveLED(const uint32_t lHardDriveNo, const bool bActive, const bool bWriteActivity) = 0;
  };
}
