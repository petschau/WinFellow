#include "fellow/api/defs.h"
#include "hardfile/hunks/RawDataReader.h"
#include <stdexcept>

using namespace std;

namespace fellow::hardfile::hunks
{
  void RawDataReader::AssertValidIndexAndLength(uint32_t length)
  {
    if (_index + length > _rawDataLength)
    {
      throw out_of_range("RawDataReader index beyond data length");
    }
  }

  char RawDataReader::GetByteAsChar(uint32_t index)
  {
    return static_cast<char>(_rawData[index]);
  }

  uint32_t RawDataReader::GetByteAsLong(uint32_t index)
  {
    return static_cast<uint32_t>(_rawData[index]);
  }

  char RawDataReader::GetNextChar()
  {
    AssertValidIndexAndLength(1);
    char c = GetByteAsChar(_index);
    _index++;

    return c;
  }

  uint32_t RawDataReader::GetNextByteswappedLong()
  {
    AssertValidIndexAndLength(4);
    uint32_t value = (GetByteAsLong(_index) << 24) | (GetByteAsLong(_index + 1) << 16) | (GetByteAsLong(_index + 2) << 8) | GetByteAsLong(_index + 3);
    _index += 4;
    return value;
  }

  string RawDataReader::GetNextString(uint32_t lengthInLongwords)
  {
    string s;
    uint32_t lengthInBytes = lengthInLongwords * 4;
    bool endOfStringFound = false;

    for (uint32_t i = 0; i < lengthInBytes; i++)
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

  uint8_t *RawDataReader::GetNextBytes(uint32_t lengthInLongwords)
  {
    uint32_t lengthInBytes = lengthInLongwords * 4;
    AssertValidIndexAndLength(lengthInBytes);
    uint8_t *bytes = new uint8_t[lengthInBytes];
    memcpy(bytes, _rawData + _index, lengthInBytes);
    _index += lengthInBytes;
    return bytes;
  }

  uint32_t RawDataReader::GetIndex()
  {
    return _index;
  }

  RawDataReader::RawDataReader(uint8_t *rawData, uint32_t rawDataLength) : _rawData(rawData), _rawDataLength(rawDataLength), _index(0)
  {
  }
}
