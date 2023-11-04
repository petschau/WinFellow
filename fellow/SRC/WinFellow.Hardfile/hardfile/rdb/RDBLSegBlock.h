#pragma once

#include <string>
#include <memory>
#include "Defs.h"
#include "hardfile/rdb/RDBFileReader.h"

namespace fellow::hardfile::rdb
{
  struct RDBLSegBlock
  {
    std::string ID;
    int32_t Blocknumber;
    int32_t SizeInLongs;
    int32_t CheckSum;
    int32_t HostID;
    int32_t Next;
    std::unique_ptr<const uint8_t> Data;

    bool HasValidCheckSum;

    int32_t GetDataSize() const;
    const uint8_t *GetData() const;
    void ReadFromFile(RDBFileReader &reader, uint32_t index);
    void Log();

    RDBLSegBlock();
  };
}
