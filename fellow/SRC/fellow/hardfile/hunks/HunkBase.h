#pragma once

#include "fellow/hardfile/hunks/RawDataReader.h"
#include "fellow/hardfile/hunks/HunkID.h"

namespace fellow::hardfile::hunks
{
  class HunkBase
  {
  public:
    virtual ULO GetID() = 0;
    virtual void Parse(RawDataReader &rawReader) = 0;

    virtual ~HunkBase() = default;
  };
}
