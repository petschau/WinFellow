#ifndef FELLOW_HARDFILE_RDB_RDB_H
#define FELLOW_HARDFILE_RDB_RDB_H

#include <vector>
#include <string>
#include <memory>
#include "fellow/api/defs.h"
#include "fellow/hardfile/rdb/RDBFileReader.h"
#include "fellow/hardfile/rdb/RDBPartition.h"
#include "fellow/hardfile/rdb/RDBFileSystemHeader.h"

namespace fellow::hardfile::rdb
{
  class RDB
  {
  public:
    std::string ID;
    ULO SizeInLongs;
    LON CheckSum;
    ULO HostID;
    ULO BlockSize;
    ULO Flags;
    ULO BadBlockList;
    ULO PartitionList;
    ULO FilesystemHeaderList;
    ULO DriveInitCode;

    ULO Cylinders;
    ULO SectorsPerTrack;
    ULO Heads;
    ULO Interleave;
    ULO ParkingZone;
    ULO WritePreComp;
    ULO ReducedWrite;
    ULO StepRate;

    ULO RDBBlockLow;
    ULO RDBBlockHigh;
    ULO LowCylinder;
    ULO HighCylinder;
    ULO CylinderBlocks;
    ULO AutoParkSeconds;
    ULO HighRDSKBlock;

    std::string DiskVendor;
    std::string DiskProduct;
    std::string DiskRevision;
    std::string ControllerVendor;
    std::string ControllerProduct;
    std::string ControllerRevision;

    std::vector<std::unique_ptr<RDBPartition>> Partitions;
    std::vector<std::unique_ptr<RDBFileSystemHeader>> FileSystemHeaders;

    void ReadFromFile(RDBFileReader& reader, bool geometryOnly = false);
    void Log();
  };
}

#endif
