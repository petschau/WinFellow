#pragma once

#include <string>
#include "fellow/hardfile/hunks/FileImage.h"
#include "fellow/hardfile/rdb/RDBFileReader.h"

namespace fellow::hardfile::rdb
{
  struct RDBFileSystemHandler
  {
    ULO Size;
    std::unique_ptr<UBY> RawData;
    fellow::hardfile::hunks::FileImage FileImage;

    RDBFileSystemHandler();

    bool ReadFromFile(RDBFileReader &reader, ULO blockChainStart, ULO blockSize);
  };
}
