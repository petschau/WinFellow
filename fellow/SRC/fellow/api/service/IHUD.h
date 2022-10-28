#pragma once

#include "fellow/api/configuration/HudConfiguration.h"

namespace fellow::api
{
  class IHUD
  {
  public:
    virtual HudConfiguration &GetHudConfiguration() = 0;
    virtual void NotifyFloppyLEDChanged(int driveIndex, bool active, bool write) = 0;
    virtual void NotifyHarddiskLEDChanged(int deviceIndex, bool active, bool write) = 0;

    virtual ~IHUD() = default;
  };
}
