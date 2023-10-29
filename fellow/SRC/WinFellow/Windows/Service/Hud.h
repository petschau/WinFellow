#pragma once

#include "Service/IHud.h"

class Hud : public Service::IHud
{
public:
  void SetFloppyLED(int driveIndex, bool active, bool write) override;
  void SetHarddiskLED(int deviceIndex, bool active, bool write) override;

  Hud();
};
