#include "hardfile/hunks/DataHunk.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

namespace fellow::hardfile::hunks
{
  ULO DataHunk::GetID()
  {
    return ID;
  }

  void DataHunk::Parse(RawDataReader& rawDataReader)
  {
    _contentSizeInLongwords = rawDataReader.GetNextByteswappedLong();
    _rawData.reset(rawDataReader.GetNextBytes(GetContentSizeInLongwords()));

    Service->Log.AddLogDebug("fhfile: RDB filesystem - Data hunk (%u), content length in bytes %u, allocate size in bytes %u\n", ID, GetContentSizeInBytes(), GetAllocateSizeInBytes());
  }

  DataHunk::DataHunk(ULO allocateSizeInLongwords)
    : InitialHunk(allocateSizeInLongwords)
  {
  }
}
