#pragma once

#include <vector>
#include "fellow/api/defs.h"
#include "hardfile/hunks/RawDataReader.h"

namespace fellow::hardfile::hunks
{
  class Reloc32OffsetTable
  {
  private:
    uint32_t _relatedHunkIndex;
    std::vector<uint32_t> _offsets;

  public:
    uint32_t GetRelatedHunkIndex();
    uint32_t GetOffsetCount();
    uint32_t GetOffset(uint32_t index);

    void Parse(RawDataReader &rawDataReader, uint32_t offsetCount);

    Reloc32OffsetTable(uint32_t relatedHunkIndex);
  };
}
