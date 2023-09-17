#include "fellow/api/defs.h"
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

  UBY RDBFileReader::ReadUBY(off_t offset)
  {
    fseek(_F, offset, SEEK_SET);
    return static_cast<UBY>(fgetc(_F));
  }

  uint32_t RDBFileReader::ReadUint32(off_t offset)
  {
    UBY value[4];
    fseek(_F, offset, SEEK_SET);
    fread(&value, 1, 4, _F);
    return static_cast<uint32_t>(value[0]) << 24 | static_cast<uint32_t>(value[1]) << 16 | static_cast<uint32_t>(value[2]) << 8 | static_cast<uint32_t>(value[3]);
  }

  LON RDBFileReader::ReadLON(off_t offset)
  {
    return static_cast<LON>(ReadUint32(offset));
  }

  UBY *RDBFileReader::ReadData(off_t offset, size_t byteCount)
  {
    UBY *data = new UBY[byteCount];
    fseek(_F, offset, SEEK_SET);
    fread(data, 1, byteCount, _F);
    return data;
  }

  RDBFileReader::RDBFileReader(FILE *F)
    : _F(F)
  {
  }
}
