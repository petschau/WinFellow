#ifndef FELLOW_HARDFILE_HUNKS_HUNKBASE_H
#define FELLOW_HARDFILE_HUNKS_HUNKBASE_H

#include "hardfile/hunks/RawDataReader.h"
#include "hardfile/hunks/HunkID.h"

namespace fellow::hardfile::hunks
{
  class HunkBase
  {
  public:
    virtual uint32_t GetID() = 0;
    virtual void Parse(RawDataReader& rawReader) = 0;

    virtual ~HunkBase() = default;
  };
}

#endif
