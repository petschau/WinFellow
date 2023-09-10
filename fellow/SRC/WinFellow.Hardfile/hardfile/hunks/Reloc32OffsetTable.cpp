#include "hardfile/hunks/Reloc32OffsetTable.h"

using namespace std;

namespace fellow::hardfile::hunks
{
  Reloc32OffsetTable::Reloc32OffsetTable(ULO relatedHunkIndex)
    : _relatedHunkIndex(relatedHunkIndex)
  {
  }

  ULO Reloc32OffsetTable::GetRelatedHunkIndex()
  {
    return _relatedHunkIndex;
  }

  ULO Reloc32OffsetTable::GetOffsetCount()
  {
    return (ULO) _offsets.size();
  }

  ULO Reloc32OffsetTable::GetOffset(ULO index)
  {
    return _offsets[index];
  }

  void Reloc32OffsetTable::Parse(RawDataReader& rawDataReader, ULO offsetCount)
  {
    for (ULO i = 0; i < offsetCount; i++)
    {
      _offsets.push_back(rawDataReader.GetNextByteswappedLong());
    }
  }
}
