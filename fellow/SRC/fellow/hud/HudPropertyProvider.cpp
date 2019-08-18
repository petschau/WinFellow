#include "fellow/hud/HudPropertyProvider.h"

#include "fellow/api/defs.h"
#include "CIA.H"
#include "FLOPPY.H"

unsigned int HudPropertyProvider::GetFps() const
{
  return _fps;
}

bool HudPropertyProvider::GetPowerLedOn() const
{
  return _powerLedOn;
}

const HudFloppyDriveStatus &HudPropertyProvider::GetFloppyDriveStatus(unsigned int floppyDriveIndex) const
{
  F_ASSERT(floppyDriveIndex < 4);
  return _floppyDriveStatus[floppyDriveIndex];
}

void HudPropertyProvider::UpdatePropertyValues()
{
  _powerLedOn = ciaIsPowerLEDEnabled();
  _fps = _fpsProvider.GetLast50Fps();

  for (unsigned int floppyDriveIndex = 0; floppyDriveIndex < 4; floppyDriveIndex++)
  {
    _floppyDriveStatus[floppyDriveIndex].DriveExists = floppyGetEnabled(floppyDriveIndex);
    _floppyDriveStatus[floppyDriveIndex].MotorOn = floppyGetMotor(floppyDriveIndex);
    _floppyDriveStatus[floppyDriveIndex].IsWriting = floppyGetIsWriting(floppyDriveIndex);
    _floppyDriveStatus[floppyDriveIndex].Track = floppyGetTrack(floppyDriveIndex);
  }
}

HudPropertyProvider::HudPropertyProvider(IFpsProvider &fpsProvider) : _fpsProvider(fpsProvider)
{
}