#pragma once

#include <string>
#include "Defs.h"

namespace fellow::hardfile::rdb
{
  class RDBFileReader
  {
  private:
    FILE *_F;

  public:
    std::string ReadString(off_t offset, size_t maxCount);
    uint8_t ReadUint8(off_t offset);
    uint32_t ReadUint32(off_t offset);
    int32_t ReadInt32(off_t offset);
    uint8_t *ReadData(off_t offset, size_t byteCount);

    RDBFileReader(FILE *F);
  };
}
