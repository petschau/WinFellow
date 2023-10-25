#pragma once

#include "hardfile/rdb/RDBFileReader.h"
#include "hardfile/rdb/RDBFileSystemHandler.h"

namespace fellow::hardfile::rdb
{
  class RDBFileSystemHeader
  {
  public:
    uint32_t SizeInLongs;
    int32_t CheckSum;
    uint32_t HostID;
    uint32_t Next;
    uint32_t Flags;
    uint32_t DOSType;
    uint32_t Version;
    uint32_t PatchFlags;

    // Device node
    uint32_t DnType;
    uint32_t DnTask;
    uint32_t DnLock;
    uint32_t DnHandler;
    uint32_t DnStackSize;
    uint32_t DnPriority;
    uint32_t DnStartup;
    uint32_t DnSegListBlock;
    uint32_t DnGlobalVec;
    uint32_t Reserved2[23];

    bool HasValidCheckSum;
    bool HasFileSystemDataErrors;

    RDBFileSystemHandler FileSystemHandler;

    void ReadFromFile(RDBFileReader &reader, uint32_t blockChainStart, uint32_t blockSize);
    void Log();

    RDBFileSystemHeader();
  };
}
