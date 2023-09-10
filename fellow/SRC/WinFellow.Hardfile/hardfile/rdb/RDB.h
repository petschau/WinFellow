#ifndef FELLOW_HARDFILE_RDB_RDB_H
#define FELLOW_HARDFILE_RDB_RDB_H

#include <vector>
#include <string>
#include <memory>
#include "fellow/api/defs.h"
#include "hardfile/rdb/RDBFileReader.h"
#include "hardfile/rdb/RDBPartition.h"
#include "hardfile/rdb/RDBFileSystemHeader.h"

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

    bool HasValidCheckSum;
    bool HasPartitionErrors;
    bool HasFileSystemHandlerErrors;

    void ReadFromFile(RDBFileReader& reader, unsigned int index, bool geometryOnly = false);
    void Log();

    RDB();
  };
}

#endif
