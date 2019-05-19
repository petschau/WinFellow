#ifndef FELLOW_API_MODULE_IHARDFILEHANDLER_H
#define FELLOW_API_MODULE_IHARDFILEHANDLER_H

#include <string>
#include <vector>
#include "fellow/api/defs.h"

#define FHFILE_MAX_DEVICES 20

namespace fellow::api::module
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

    HardfileGeometry()
    {
      Clear();
    }

    void Clear()
    {
      LowCylinder = 0;
      HighCylinder = 0;
      BytesPerSector = 0;
      SectorsPerTrack = 0;
      Surfaces = 0;
      Tracks = 0;
      ReservedBlocks = 0;
    }

    bool operator==(const HardfileGeometry& other) const
    {
      return BytesPerSector == other.BytesPerSector &&
        SectorsPerTrack == other.SectorsPerTrack &&
        Surfaces == other.Surfaces &&
        ReservedBlocks == other.ReservedBlocks;
    }
  };

  struct HardfilePartition
  {
    std::string PreferredName;
    HardfileGeometry Geometry;
  };

  struct HardfileConfiguration
  {
  public:
    std::string Filename;
    bool Readonly;
    HardfileGeometry Geometry;

    std::vector<HardfilePartition> Partitions;

    bool operator==(const HardfileConfiguration& other) const
    {
      return Filename == other.Filename && Geometry == other.Geometry && Readonly == other.Readonly;
    }

    void Clear()
    {
      Filename = "";
      Readonly = false;
      Geometry.Clear();
      Partitions.clear();
    }
  };

  // Abstract interface to HardfileHandler
  class IHardfileHandler
  {
  public:
    virtual ~IHardfileHandler() = default;

    // Autoconfig and ROM memory
    virtual void CardInit() = 0;
    virtual void CardMap(ULO mapping) = 0;
    virtual UBY ReadByte(ULO address) = 0;
    virtual UWO ReadWord(ULO address) = 0;
    virtual ULO ReadLong(ULO address) = 0;

    // Native callback
    virtual void Do(ULO data) = 0;

    // Configuration
    virtual void SetEnabled(bool enabled) = 0;
    virtual bool GetEnabled() = 0;
    virtual void Clear() = 0;
    virtual bool CompareHardfile(const HardfileConfiguration& hardfile, unsigned int index) = 0;
    virtual void SetHardfile(const HardfileConfiguration& hardfile, unsigned int index) = 0;
    virtual bool RemoveHardfile(unsigned int index) = 0;
    virtual unsigned int GetMaxHardfileCount() = 0;
    virtual void SetDeviceNameStartNumber(unsigned int unitNoStartNumber) = 0;

    // UI helper function
    virtual bool Create(const HardfileConfiguration& configuration, ULO size) = 0;
    virtual bool HasRDB(const std::string& filename) = 0;
    virtual HardfileConfiguration GetConfigurationFromRDBGeometry(const std::string& filename) = 0;
    
    // Global events
    virtual void EmulationStart() = 0;
    virtual void EmulationStop() = 0;
    virtual void HardReset() = 0;
    virtual void Startup() = 0;
    virtual void Shutdown() = 0;
  };

  extern IHardfileHandler *HardfileHandler;
}

#endif
