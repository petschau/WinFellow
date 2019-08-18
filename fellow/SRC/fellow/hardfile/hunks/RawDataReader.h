#pragma once

#include <string>
#include "fellow/api/defs.h"

namespace fellow::hardfile::hunks
{
  class RawDataReader
  {
  private:
    UBY *_rawData;
    ULO _rawDataLength;
    ULO _index;

    void AssertValidIndexAndLength(ULO length);
    char GetByteAsChar(ULO index);
    ULO GetByteAsLong(ULO index);
    char GetNextChar();

  public:
    ULO GetIndex();
    ULO GetNextByteswappedLong();
    std::string GetNextString(ULO lengthInLongwords);
    UBY *GetNextBytes(ULO lengthInLongwords);

    RawDataReader(UBY *rawData, ULO rawDataLength);
  };
}
