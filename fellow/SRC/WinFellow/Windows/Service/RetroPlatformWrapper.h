#pragma once

#include "Service/IRetroPlatform.h"

class RetroPlatformWrapper : public Service::IRetroPlatform
{
public:
  bool SendHardDriveContent(const uint32_t lHardDriveNo, const char *szImageName, const bool bWriteProtected) override;
};
