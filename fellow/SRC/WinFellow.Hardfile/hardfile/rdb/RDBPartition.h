#ifndef FELLOW_HARDFILE_RDB_RDBPARTITION_H
#define FELLOW_HARDFILE_RDB_RDBPARTITION_H

#include <string>
#include "fellow/api/defs.h"
#include "hardfile/rdb/RDBFileReader.h"

namespace fellow::hardfile::rdb
{
  struct RDBPartition
  {
    std::string ID;
    uint32_t SizeInLongs;
    uint32_t CheckSum;
    uint32_t HostID;
    uint32_t Next;
    uint32_t Flags;
    uint32_t BadBlockList;
    uint32_t DevFlags;
    char DriveNameLength;
    std::string DriveName;

    // DOS Environment Vector
    uint32_t SizeOfVector;
    uint32_t SizeBlock;
    uint32_t SecOrg;
    uint32_t Surfaces;
    uint32_t SectorsPerBlock;
    uint32_t BlocksPerTrack;
    uint32_t Reserved;
    uint32_t PreAlloc;
    uint32_t Interleave;

    uint32_t LowCylinder;
    uint32_t HighCylinder;
    uint32_t NumBuffer;
    uint32_t BufMemType;
    uint32_t MaxTransfer;
    uint32_t Mask;
    uint32_t BootPri;
    uint32_t DOSType;
    uint32_t Baud;
    uint32_t Control;
    uint32_t Bootblocks;

    bool HasValidCheckSum;

    bool IsAutomountable();
    bool IsBootable();

    void ReadFromFile(RDBFileReader& reader, uint32_t blockChainStart, uint32_t blockSize);
    void Log();

    RDBPartition();
  };
}

#endif
