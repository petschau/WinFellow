#ifndef FELLOW_HARDFILE_HUNKS_ADDITIONALHUNK_H
#define FELLOW_HARDFILE_HUNKS_ADDITIONALHUNK_H

#include "fellow/hardfile/hunks/HunkBase.h"

namespace fellow::hardfile::hunks
{
  class AdditionalHunk : public HunkBase
  {
  private:
    ULO _sourceHunkIndex;

  public:
    ULO GetSourceHunkIndex();
    virtual void Parse(RawDataReader& rawReader) = 0;

    AdditionalHunk(ULO sourceHunk);
  };
}

#endif
