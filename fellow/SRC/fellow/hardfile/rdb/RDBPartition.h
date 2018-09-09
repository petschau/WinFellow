#ifndef FELLOW_HARDFILE_RDB_RDBPARTITION_H
#define FELLOW_HARDFILE_RDB_RDBPARTITION_H

#include <string>
#include "fellow/api/defs.h"
#include "fellow/hardfile/rdb/RDBFileReader.h"

namespace fellow::hardfile::rdb
{
  struct RDBPartition
  {
    std::string ID;
    ULO SizeInLongs;
    ULO CheckSum;
    ULO HostID;
    ULO Next;
    ULO Flags;
    ULO BadBlockList;
    ULO DevFlags;
    char DriveNameLength;
    std::string DriveName;

    // DOS Environment Vector
    ULO SizeOfVector;
    ULO SizeBlock;
    ULO SecOrg;
    ULO Surfaces;
    ULO SectorsPerBlock;
    ULO BlocksPerTrack;
    ULO Reserved;
    ULO PreAlloc;
    ULO Interleave;

    ULO LowCylinder;
    ULO HighCylinder;
    ULO NumBuffer;
    ULO BufMemType;
    ULO MaxTransfer;
    ULO Mask;
    ULO BootPri;
    ULO DOSType;
    ULO Baud;
    ULO Control;
    ULO Bootblocks;

    bool IsAutomountable();
    bool IsBootable();

    void ReadFromFile(RDBFileReader& reader, ULO blockChainStart, ULO blockSize);
    void Log();
  };
}

#endif
