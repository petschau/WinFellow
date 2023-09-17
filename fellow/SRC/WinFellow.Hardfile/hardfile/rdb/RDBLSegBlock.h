#ifndef FELLOW_HARDFILE_RDB_RDBLSEGBLOCK_H
#define FELLOW_HARDFILE_RDB_RDBLSEGBLOCK_H

#include <string>
#include <memory>
#include "fellow/api/defs.h"
#include "hardfile/rdb/RDBFileReader.h"

namespace fellow::hardfile::rdb
{
  struct RDBLSegBlock
  {
    std::string ID;
    LON Blocknumber;
    LON SizeInLongs;
    LON CheckSum;
    LON HostID;
    LON Next;
    std::unique_ptr<const uint8_t> Data;

    bool HasValidCheckSum;

    LON GetDataSize() const;
    const uint8_t* GetData() const;
    void ReadFromFile(RDBFileReader& reader, uint32_t index);
    void Log();

    RDBLSegBlock();
  };
}

#endif
