#pragma once

#include "fellow/api/service/IHUD.h"

namespace fellow::service
{
  class HUD : public fellow::api::service::IHUD
  {
  public:
    void SetFloppyLED(int driveIndex, bool active, bool write) override;
    void SetHarddiskLED(int deviceIndex, bool active, bool write) override;

    HUD();
  };
}
