#pragma once

namespace Service
{
  class IHud
  {
  public:
    virtual ~IHud() = default;
    virtual void SetFloppyLED(int driveIndex, bool active, bool write) = 0;
    virtual void SetHarddiskLED(int deviceIndex, bool active, bool write) = 0;
  };
}
