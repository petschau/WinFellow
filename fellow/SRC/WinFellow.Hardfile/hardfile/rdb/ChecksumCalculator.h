#pragma once

#include "hardfile/rdb/RDBFileReader.h"

namespace fellow::hardfile::rdb
{
  class CheckSumCalculator
  {
  public:
    static bool HasValidCheckSum(RDBFileReader& reader, unsigned int sizeInLongs, unsigned int index);
  };
}
