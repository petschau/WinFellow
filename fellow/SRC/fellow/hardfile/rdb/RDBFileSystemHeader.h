#pragma once

#include "fellow/api/defs.h"
#include "fellow/hardfile/rdb/RDBFileReader.h"
#include "fellow/hardfile/rdb/RDBFileSystemHandler.h"

namespace fellow::hardfile::rdb
{
  class RDBFileSystemHeader
  {
  public:
    ULO SizeInLongs;
    LON CheckSum;
    ULO HostID;
    ULO Next;
    ULO Flags;
    ULO DOSType;
    ULO Version;
    ULO PatchFlags;

    // Device node
    ULO DnType;
    ULO DnTask;
    ULO DnLock;
    ULO DnHandler;
    ULO DnStackSize;
    ULO DnPriority;
    ULO DnStartup;
    ULO DnSegListBlock;
    ULO DnGlobalVec;
    ULO Reserved2[23];

    bool HasValidCheckSum;
    bool HasFileSystemDataErrors;

    RDBFileSystemHandler FileSystemHandler;

    void ReadFromFile(RDBFileReader &reader, ULO blockChainStart, ULO blockSize);
    void Log();

    RDBFileSystemHeader();
  };
}
