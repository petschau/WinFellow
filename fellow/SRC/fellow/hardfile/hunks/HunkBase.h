#ifndef FELLOW_HARDFILE_HUNKS_HUNKBASE_H
#define FELLOW_HARDFILE_HUNKS_HUNKBASE_H

#include "fellow/hardfile/hunks/RawDataReader.h"
#include "fellow/hardfile/hunks/HunkID.h"

namespace fellow::hardfile::hunks
{
  class HunkBase
  {
  public:
    virtual ULO GetID() = 0;
    virtual void Parse(RawDataReader& rawReader) = 0;

    virtual ~HunkBase() = default;
  };
}

#endif
