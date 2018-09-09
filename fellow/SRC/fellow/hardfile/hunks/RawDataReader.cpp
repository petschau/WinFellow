#include "fellow/hardfile/hunks/RawDataReader.h"

using namespace std;

namespace fellow::hardfile::hunks
{
  void RawDataReader::AssertValidIndexAndLength(ULO length)
  {
    if (_index + length > _rawDataLength)
    {
      throw out_of_range("RawDataReader index beyond data length");
    }
  }

  char RawDataReader::GetByteAsChar(ULO index)
  {
    return static_cast<char>(_rawData[index]);
  }

  ULO RawDataReader::GetByteAsLong(ULO index)
  {
    return static_cast<ULO>(_rawData[index]);
  }

  char RawDataReader::GetNextChar()
  {
    AssertValidIndexAndLength(1);
    char c = GetByteAsChar(_index);
    _index++;

    return c;
  }

  ULO RawDataReader::GetNextByteswappedLong()
  {
    AssertValidIndexAndLength(4);
    ULO value = (GetByteAsLong(_index) << 24) | (GetByteAsLong(_index + 1) << 16) | (GetByteAsLong(_index + 2) << 8) | GetByteAsLong(_index + 3);
    _index += 4;
    return value;
  }

  string RawDataReader::GetNextString(ULO lengthInLongwords)
  {
    string s;
    ULO lengthInBytes = lengthInLongwords * 4;
    bool endOfStringFound = false;

    for (ULO i = 0; i < lengthInBytes; i++)
    {
      char c = GetNextChar();
      if (c == '\0')
      {
        endOfStringFound = true;
      }

      if (!endOfStringFound)
      {
        s += c;
      }
    }

    return s;
  }

  UBY *RawDataReader::GetNextBytes(ULO lengthInLongwords)
  {
    ULO lengthInBytes = lengthInLongwords * 4;
    AssertValidIndexAndLength(lengthInBytes);
    UBY *bytes = new UBY[lengthInBytes];
    memcpy(bytes, _rawData + _index, lengthInBytes);
    _index += lengthInBytes;
    return bytes;
  }

  ULO RawDataReader::GetIndex()
  {
    return _index;
  }

  RawDataReader::RawDataReader(UBY *rawData, ULO rawDataLength)
    : _rawData(rawData), _rawDataLength(rawDataLength), _index(0)
  {
  }
}
