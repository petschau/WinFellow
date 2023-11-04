#include "Defs.h"
#include "hardfile/rdb/RDBFileReader.h"

using namespace std;

namespace fellow::hardfile::rdb
{
  string RDBFileReader::ReadString(off_t offset, size_t maxCount)
  {
    string s;
    bool moreCharacters = true;
    fseek(_F, offset, SEEK_SET);
    while (maxCount-- != 0 && moreCharacters)
    {
      int c = fgetc(_F);
      if (c != -1)
      {
        s.push_back(static_cast<char>(c));
      }
      else
      {
        moreCharacters = false;
      }
    }
    return s;
  }

  uint8_t RDBFileReader::ReadUint8(off_t offset)
  {
    fseek(_F, offset, SEEK_SET);
    return static_cast<uint8_t>(fgetc(_F));
  }

  uint32_t RDBFileReader::ReadUint32(off_t offset)
  {
    uint8_t value[4];
    fseek(_F, offset, SEEK_SET);
    fread(&value, 1, 4, _F);
    return static_cast<uint32_t>(value[0]) << 24 | static_cast<uint32_t>(value[1]) << 16 | static_cast<uint32_t>(value[2]) << 8 | static_cast<uint32_t>(value[3]);
  }

  int32_t RDBFileReader::ReadInt32(off_t offset)
  {
    return static_cast<int32_t>(ReadUint32(offset));
  }

  uint8_t *RDBFileReader::ReadData(off_t offset, size_t byteCount)
  {
    uint8_t *data = new uint8_t[byteCount];
    fseek(_F, offset, SEEK_SET);
    fread(data, 1, byteCount, _F);
    return data;
  }

  RDBFileReader::RDBFileReader(FILE *F) : _F(F)
  {
  }
}
