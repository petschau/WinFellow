#ifndef FELLOW_HARDFILE_RDB_RDBLSEGBLOCK_H
#define FELLOW_HARDFILE_RDB_RDBLSEGBLOCK_H

#include <string>
#include "fellow/api/defs.h"
#include "fellow/hardfile/rdb/RDBFileReader.h"

namespace fellow::hardfile::rdb
{
  struct RDBLSegBlock
  {
    std::string ID;
    LON SizeInLongs;
    LON CheckSum;
    LON HostID;
    LON Next;
    std::unique_ptr<const UBY> Data;

    RDBLSegBlock();

    LON GetDataSize() const;
    const UBY* GetData() const;
    void ReadFromFile(RDBFileReader& reader, ULO index);
    void Log();
  };
}

#endif
