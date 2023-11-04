#pragma once

#include "Module/Hardfile/IHardfileHandler.h"
#include "hardfile/rdb/RDB.h"

namespace fellow::hardfile
{
  enum fhfile_status
  {
    FHFILE_NONE = 0,
    FHFILE_HDF = 1
  };

  class HardfileDevice
  {
  public:
    // Filename and geometry as listed in the config or on the RDB
    Module::Hardfile::HardfileConfiguration Configuration;

    // Internal properties, actual runtime values used for the hardfile
    bool Readonly;
    unsigned int FileSize;     // Actual file size
    unsigned int GeometrySize; // Size taken by configured geometry
    fhfile_status Status;
    FILE *F;
    bool HasRDB;
    rdb::RDB *RDB;

    bool CloseFile();
    void DeleteRDB();
    bool Clear();

    HardfileDevice();
    ~HardfileDevice();
  };
}
