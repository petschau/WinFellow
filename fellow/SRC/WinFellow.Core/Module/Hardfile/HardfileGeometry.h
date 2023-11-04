#pragma once

namespace Module::Hardfile
{
  struct HardfileGeometry
  {
    unsigned int LowCylinder;
    unsigned int HighCylinder;
    unsigned int BytesPerSector;
    unsigned int SectorsPerTrack;
    unsigned int Surfaces;
    unsigned int Tracks;
    unsigned int ReservedBlocks;

    void Clear();
    bool operator==(const HardfileGeometry &other) const;

    HardfileGeometry();
  };
}
