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
    static const uint32_t ID = Reloc32HunkID;
    std::vector<std::unique_ptr<Reloc32OffsetTable>> _offsetTables;

  public:
    uint32_t GetID() override;
    
    uint32_t GetOffsetTableCount();
    Reloc32OffsetTable* GetOffsetTable(uint32_t index);

    void Parse(RawDataReader& rawDataReader) override;

    Reloc32Hunk(uint32_t sourceHunkIndex);
  };
}

#endif
