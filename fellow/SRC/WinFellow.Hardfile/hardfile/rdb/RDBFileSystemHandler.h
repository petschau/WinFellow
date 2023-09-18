#pragma once

#include "hardfile/hunks/FileImage.h"
#include "hardfile/rdb/RDBFileReader.h"

namespace fellow::hardfile::rdb
{
  struct RDBFileSystemHandler
  {
    uint32_t Size;
    std::unique_ptr<uint8_t> RawData;
    fellow::hardfile::hunks::FileImage FileImage;

    RDBFileSystemHandler();

    bool ReadFromFile(RDBFileReader& reader, uint32_t blockChainStart, uint32_t blockSize);
  };
}
