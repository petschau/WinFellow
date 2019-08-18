#pragma once

#include "fellow/application/IFpsProvider.h"

struct HudFloppyDriveStatus
{
  bool DriveExists;
  bool MotorOn;
  bool IsWriting;
  unsigned int Track;
};

class HudPropertyProvider
{
private:
  IFpsProvider &_fpsProvider;

  unsigned int _fps;
  bool _powerLedOn;
  HudFloppyDriveStatus _floppyDriveStatus[4];

public:
  void UpdatePropertyValues();

  bool GetPowerLedOn() const;
  unsigned int GetFps() const;
  const HudFloppyDriveStatus &GetFloppyDriveStatus(unsigned int floppyDriveIndex) const;

  HudPropertyProvider(IFpsProvider &fpsProvider);
};
