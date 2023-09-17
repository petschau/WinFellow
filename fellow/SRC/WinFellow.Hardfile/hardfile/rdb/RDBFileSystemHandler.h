#ifndef FELLOW_HARDFILE_RDB_RDBFILESYSTEMHANDLER_H
#define FELLOW_HARDFILE_RDB_RDBFILESYSTEMHANDLER_H

#include <string>
#include "hardfile/hunks/FileImage.h"
#include "hardfile/rdb/RDBFileReader.h"

namespace fellow::hardfile::rdb
{
  struct RDBFileSystemHandler
  {
    uint32_t Size;
    std::unique_ptr<UBY> RawData;
    fellow::hardfile::hunks::FileImage FileImage;

    RDBFileSystemHandler();

    bool ReadFromFile(RDBFileReader& reader, uint32_t blockChainStart, uint32_t blockSize);
  };
}

#endif
