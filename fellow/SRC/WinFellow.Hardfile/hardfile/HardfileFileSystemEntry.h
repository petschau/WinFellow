#pragma once

#include <cstdint>
#include "DebugApi/IMemorySystem.h"
#include "hardfile/rdb/RDB.h"

namespace fellow::hardfile
{
  struct HardfileFileSystemEntry
  {
    Debug::IMemorySystem &Memory;
    rdb::RDBFileSystemHeader *Header;
    uint32_t SegListAddress;

    bool IsOlderOrSameFileSystemVersion(uint32_t DOSType, uint32_t version);

    bool IsDOSType(uint32_t DOSType);
    bool IsOlderVersion(uint32_t version);
    bool IsOlderOrSameVersion(uint32_t version);

    uint32_t GetDOSType();
    uint32_t GetVersion();
    void CopyHunkToAddress(uint32_t destinationAddress, uint32_t hunkIndex);

    HardfileFileSystemEntry(Debug::IMemorySystem &memory, rdb::RDBFileSystemHeader *header, uint32_t segListAddress);
  };
}