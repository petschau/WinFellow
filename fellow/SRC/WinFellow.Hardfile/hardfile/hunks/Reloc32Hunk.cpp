#include "fellow/api/defs.h"
#include "hardfile/hunks/Reloc32Hunk.h"
#include "VirtualHost/Core.h"

using namespace std;

namespace fellow::hardfile::hunks
{
  uint32_t Reloc32Hunk::GetID()
  {
    return ID;
  }

  uint32_t Reloc32Hunk::GetOffsetTableCount()
  {
    return (uint32_t)_offsetTables.size();
  }

  Reloc32OffsetTable *Reloc32Hunk::GetOffsetTable(uint32_t index)
  {
    return _offsetTables[index].get();
  }

  void Reloc32Hunk::Parse(RawDataReader &rawDataReader)
  {
    uint32_t offsetCount = rawDataReader.GetNextByteswappedLong();
    while (offsetCount != 0)
    {
      uint32_t relatedHunk = rawDataReader.GetNextByteswappedLong();
      Reloc32OffsetTable *offsetTable = new Reloc32OffsetTable(relatedHunk);
      offsetTable->Parse(rawDataReader, offsetCount);
      _offsetTables.push_back(unique_ptr<Reloc32OffsetTable>(offsetTable));
      offsetCount = rawDataReader.GetNextByteswappedLong();

      _core.Log->AddLogDebug("fhfile: RDB filesystem - Reloc32 hunk (%u), entry %u for hunk %u offset count %u\n", ID, _offsetTables.size() - 1, relatedHunk, offsetCount);
    }
  }

  Reloc32Hunk::Reloc32Hunk(uint32_t sourceHunkIndex) : AdditionalHunk(sourceHunkIndex)
  {
  }
}
