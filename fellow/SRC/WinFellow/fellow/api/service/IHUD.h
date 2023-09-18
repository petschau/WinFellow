#pragma once
namespace fellow::api::service
{
  class IHUD
  {
  public:
    virtual ~IHUD() = default;
    virtual void SetFloppyLED(int driveIndex, bool active, bool write) = 0;
    virtual void SetHarddiskLED(int deviceIndex, bool active, bool write) = 0;
  };
}
