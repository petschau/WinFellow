#ifndef FELLOW_HARDFILE_HUNKS_RELOC32HUNK_H
#define FELLOW_HARDFILE_HUNKS_RELOC32HUNK_H

#include <memory>
#include <vector>
#include "hardfile/hunks/AdditionalHunk.h"
#include "hardfile/hunks/Reloc32OffsetTable.h"

namespace fellow::hardfile::hunks
{
  class Reloc32Hunk : public AdditionalHunk
  {
  private:
    static const ULO ID = Reloc32HunkID;
    std::vector<std::unique_ptr<Reloc32OffsetTable>> _offsetTables;

  public:
    ULO GetID() override;
    
    ULO GetOffsetTableCount();
    Reloc32OffsetTable* GetOffsetTable(ULO index);

    void Parse(RawDataReader& rawDataReader) override;

    Reloc32Hunk(ULO sourceHunkIndex);
  };
}

#endif
