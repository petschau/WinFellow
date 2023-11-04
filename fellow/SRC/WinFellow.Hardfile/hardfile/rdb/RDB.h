#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Defs.h"
#include "hardfile/rdb/RDBFileReader.h"
#include "hardfile/rdb/RDBPartition.h"
#include "hardfile/rdb/RDBFileSystemHeader.h"

namespace fellow::hardfile::rdb
{
  class RDB
  {
  public:
    std::string ID;
    uint32_t SizeInLongs;
    int32_t CheckSum;
    uint32_t HostID;
    uint32_t BlockSize;
    uint32_t Flags;
    uint32_t BadBlockList;
    uint32_t PartitionList;
    uint32_t FilesystemHeaderList;
    uint32_t DriveInitCode;

    uint32_t Cylinders;
    uint32_t SectorsPerTrack;
    uint32_t Heads;
    uint32_t Interleave;
    uint32_t ParkingZone;
    uint32_t WritePreComp;
    uint32_t ReducedWrite;
    uint32_t StepRate;

    uint32_t RDBBlockLow;
    uint32_t RDBBlockHigh;
    uint32_t LowCylinder;
    uint32_t HighCylinder;
    uint32_t CylinderBlocks;
    uint32_t AutoParkSeconds;
    uint32_t HighRDSKBlock;

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

    void ReadFromFile(RDBFileReader &reader, unsigned int index, bool geometryOnly = false);
    void Log();

    RDB();
  };
}
