#pragma once

#include <string>
#include "fellow/api/defs.h"

namespace fellow::hardfile::rdb
{
  class RDBFileReader
  {
  private:
    FILE *_F;

  public:
    std::string ReadString(off_t offset, size_t maxCount);
    UBY ReadUBY(off_t offset);
    ULO ReadULO(off_t offset);
    LON ReadLON(off_t offset);
    UBY *ReadData(off_t offset, size_t byteCount);

    RDBFileReader(FILE *F);
  };
}
