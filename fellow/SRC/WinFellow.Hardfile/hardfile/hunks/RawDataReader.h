#ifndef FELLOW_HARDFILE_HUNKS_RAWDATAREADER_H
#define FELLOW_HARDFILE_HUNKS_RAWDATAREADER_H

#include <string>
#include "fellow/api/defs.h"

namespace fellow::hardfile::hunks
{
  class RawDataReader
  {
  private:
    uint8_t * _rawData;
    uint32_t _rawDataLength;
    uint32_t _index;

    void AssertValidIndexAndLength(uint32_t length);
    char GetByteAsChar(uint32_t index);
    uint32_t GetByteAsLong(uint32_t index);
    char GetNextChar();

  public:
    uint32_t GetIndex();
    uint32_t GetNextByteswappedLong();
    std::string GetNextString(uint32_t lengthInLongwords);
    uint8_t *GetNextBytes(uint32_t lengthInLongwords);

    RawDataReader(uint8_t *rawData, uint32_t rawDataLength);
  };
}

#endif
