#ifndef FELLOW_HARDFILE_HARDFILESTRUCTS_H
#define FELLOW_HARDFILE_HARDFILESTRUCTS_H

#include "fellow/api/module/IHardfileHandler.h"
#include "fellow/hardfile/rdb/RDB.h"

namespace fellow::hardfile
{
  typedef enum
  {
    FHFILE_NONE = 0,
    FHFILE_HDF = 1
  } fhfile_status;

  struct HardfileMountListEntry
  {
    int DeviceIndex;
    int PartitionIndex;
    std::string Name;
    ULO NameAddress;

    HardfileMountListEntry(int deviceIndex, int partitionIndex, const std::string& name)
      : DeviceIndex(deviceIndex), PartitionIndex(partitionIndex), Name(name), NameAddress(0)
    {
    }
  };

  struct HardfileFileSystemEntry
  {
    fellow::hardfile::rdb::RDBFileSystemHeader* Header;
    ULO SegListAddress;

    bool IsOlderOrSameFileSystemVersion(ULO DOSType, ULO version);

    bool IsDOSType(ULO DOSType);
    bool IsOlderVersion(ULO version);
    bool IsOlderOrSameVersion(ULO version);

    ULO GetDOSType();
    ULO GetVersion();
    void CopyHunkToAddress(ULO destinationAddress, int hunkIndex);

    HardfileFileSystemEntry(fellow::hardfile::rdb::RDBFileSystemHeader* header, ULO segListAddress);
  };

  class HardfileDevice : public fellow::api::module::HardfileDevice
  {
  public:
    ULO tracks;            /* Used by the driver */
    bool readonly;
    ULO bytespersector;
    ULO reservedblocks;
    fhfile_status status;
    FILE *F;
    bool hasRDB;
    fellow::hardfile::rdb::RDB *rdb;
    ULO lowCylinder;
    ULO highCylinder;
  };
}

#endif
