#pragma once

#include "Service/IHud.h"

namespace test::mock::fellow::api::service
{
  class HUDMock : public ::Service::IHud
  {
  public:
    void SetFloppyLED(int driveIndex, bool active, bool write) override;
    void SetHarddiskLED(int deviceIndex, bool active, bool write) override;
  };
}
