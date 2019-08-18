#include "fellow/api/defs.h"
#include "fellow/hardfile/hunks/Reloc32Hunk.h"
#include "fellow/api/Services.h"

using namespace std;
using namespace fellow::api;

namespace fellow::hardfile::hunks
{
  ULO Reloc32Hunk::GetID()
  {
    return ID;
  }

  ULO Reloc32Hunk::GetOffsetTableCount()
  {
    return (ULO) _offsetTables.size();
  }

  Reloc32OffsetTable* Reloc32Hunk::GetOffsetTable(ULO index)
  {
    return _offsetTables[index].get();
  }

  void Reloc32Hunk::Parse(RawDataReader& rawDataReader)
  {
    ULO offsetCount = rawDataReader.GetNextByteswappedLong();
    while (offsetCount != 0)
    {
      ULO relatedHunk = rawDataReader.GetNextByteswappedLong();
      Reloc32OffsetTable* offsetTable = new Reloc32OffsetTable(relatedHunk);
      offsetTable->Parse(rawDataReader, offsetCount);
      _offsetTables.push_back(unique_ptr<Reloc32OffsetTable>(offsetTable));
      offsetCount = rawDataReader.GetNextByteswappedLong();

      Service->Log.AddLogDebug("fhfile: RDB filesystem - Reloc32 hunk (%u), entry %u for hunk %u offset count %u\n", ID, _offsetTables.size() - 1, relatedHunk, offsetCount);
    }
  }

  Reloc32Hunk::Reloc32Hunk(ULO sourceHunkIndex)
    : AdditionalHunk(sourceHunkIndex)
  {
  }
}
