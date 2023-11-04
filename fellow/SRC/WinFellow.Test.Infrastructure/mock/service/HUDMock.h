#pragma once

#include "Service/IHud.h"

namespace mock::Service
{
  class HudMock : public ::Service::IHud
  {
  public:
    void SetFloppyLED(int driveIndex, bool active, bool write) override;
    void SetHarddiskLED(int deviceIndex, bool active, bool write) override;
  };
}
