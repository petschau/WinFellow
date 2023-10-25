#include "hardfile/hunks/Reloc32OffsetTable.h"

using namespace std;

namespace fellow::hardfile::hunks
{
  Reloc32OffsetTable::Reloc32OffsetTable(uint32_t relatedHunkIndex) : _relatedHunkIndex(relatedHunkIndex)
  {
  }

  uint32_t Reloc32OffsetTable::GetRelatedHunkIndex()
  {
    return _relatedHunkIndex;
  }

  uint32_t Reloc32OffsetTable::GetOffsetCount()
  {
    return (uint32_t)_offsets.size();
  }

  uint32_t Reloc32OffsetTable::GetOffset(uint32_t index)
  {
    return _offsets[index];
  }

  void Reloc32OffsetTable::Parse(RawDataReader &rawDataReader, uint32_t offsetCount)
  {
    for (uint32_t i = 0; i < offsetCount; i++)
    {
      _offsets.push_back(rawDataReader.GetNextByteswappedLong());
    }
  }
}
