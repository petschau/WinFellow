#ifndef FELLOW_HARDFILE_HUNKS_RELOC32OFFSETTABLE_H
#define FELLOW_HARDFILE_HUNKS_RELOC32OFFSETTABLE_H

#include <vector>
#include "fellow/api/defs.h"
#include "hardfile/hunks/RawDataReader.h"

namespace fellow::hardfile::hunks
{
  class Reloc32OffsetTable
  {
  private:
    ULO _relatedHunkIndex;
    std::vector<ULO> _offsets;

  public:
    ULO GetRelatedHunkIndex();
    ULO GetOffsetCount();
    ULO GetOffset(ULO index);

    void Parse(RawDataReader& rawDataReader, ULO offsetCount);

    Reloc32OffsetTable(ULO relatedHunkIndex);
  };
}

#endif
