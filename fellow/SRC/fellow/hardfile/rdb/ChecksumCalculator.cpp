#include "fellow/hardfile/rdb/ChecksumCalculator.h"

namespace fellow::hardfile::rdb
{
  bool CheckSumCalculator::HasValidCheckSum(RDBFileReader &reader, unsigned int sizeInLongs, unsigned int index)
  {
    ULO checksum = 0;
    for (unsigned int i = 0; i < sizeInLongs; i++)
    {
      checksum += reader.ReadULO(index + i * 4);
    }
    return checksum == 0;
  }
}
