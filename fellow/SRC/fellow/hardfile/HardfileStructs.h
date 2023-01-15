#pragma once

#include "fellow/api/modules/IHardfileHandler.h"
#include "fellow/hardfile/rdb/RDB.h"
#include "fellow/api/vm/IMemorySystem.h"

namespace fellow::hardfile
{
  enum class fhfile_status
  {
    FHFILE_NONE = 0,
    FHFILE_HDF = 1
  };

  struct HardfileMountListEntry
  {
    unsigned int DeviceIndex;
    int PartitionIndex;
    std::string Name;
    ULO NameAddress;

    HardfileMountListEntry(unsigned int deviceIndex, int partitionIndex, const std::string &name) : DeviceIndex(deviceIndex), PartitionIndex(partitionIndex), Name(name), NameAddress(0)
    {
    }
  };

  struct HardfileFileSystemEntry
  {
    rdb::RDBFileSystemHeader *Header;
    ULO SegListAddress;
    fellow::api::vm::IMemorySystem *_vmMemory;

    bool IsOlderOrSameFileSystemVersion(ULO DOSType, ULO version);

    bool IsDOSType(ULO DOSType);
    bool IsOlderVersion(ULO version);
    bool IsOlderOrSameVersion(ULO version);

    ULO GetDOSType();
    ULO GetVersion();
    void CopyHunkToAddress(ULO destinationAddress, ULO hunkIndex);

    HardfileFileSystemEntry(rdb::RDBFileSystemHeader *header, ULO segListAddress, fellow::api::vm::IMemorySystem *vmMemory);
  };

  class HardfileDevice
  {
  public:
    // Filename and geometry as listed in the config or on the RDB
    fellow::api::modules::HardfileConfiguration Configuration;

    // Internal properties, actual runtime values used for the hardfile
    bool Readonly;
    unsigned int FileSize;     // Actual file size
    unsigned int GeometrySize; // Size taken by configured geometry
    fhfile_status Status;
    FILE *F;
    bool HasRDB;
    rdb::RDB *RDB;

    bool CloseFile()
    {
      if (F != nullptr)
      {
        fflush(F);
        fclose(F);
        return true;
      }

      return false;
    }

    void DeleteRDB()
    {
      if (HasRDB)
      {
        delete RDB;
        RDB = nullptr;
        HasRDB = false;
      }
    }

    bool Clear()
    {
      bool result = CloseFile();
      DeleteRDB();
      FileSize = 0;
      GeometrySize = 0;
      Readonly = false;
      Status = fellow::hardfile::fhfile_status::FHFILE_NONE;
      Configuration.Clear();
      return result;
    }

    HardfileDevice() : Readonly(false), FileSize(0), GeometrySize(0), Status(fellow::hardfile::fhfile_status::FHFILE_NONE), F(nullptr), HasRDB(false), RDB(nullptr)
    {
    }

    virtual ~HardfileDevice()
    {
      CloseFile();
      DeleteRDB();
    }
  };
}
