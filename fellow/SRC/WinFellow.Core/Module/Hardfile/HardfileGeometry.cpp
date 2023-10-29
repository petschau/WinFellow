#include "Module/Hardfile/HardfileGeometry.h"

namespace Module::Hardfile
{
  HardfileGeometry::HardfileGeometry()
  {
    Clear();
  }

  void HardfileGeometry::Clear()
  {
    LowCylinder = 0;
    HighCylinder = 0;
    BytesPerSector = 0;
    SectorsPerTrack = 0;
    Surfaces = 0;
    Tracks = 0;
    ReservedBlocks = 0;
  }

  bool HardfileGeometry::operator==(const HardfileGeometry &other) const
  {
    return BytesPerSector == other.BytesPerSector && SectorsPerTrack == other.SectorsPerTrack && Surfaces == other.Surfaces && ReservedBlocks == other.ReservedBlocks;
  }
}
