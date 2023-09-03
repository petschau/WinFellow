#include "hardfile/hunks/CodeHunk.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

namespace fellow::hardfile::hunks
{
  ULO CodeHunk::GetID()
  {
    return ID;
  }

  void CodeHunk::Parse(RawDataReader& rawDataReader)
  {
    _contentSizeInLongwords = rawDataReader.GetNextByteswappedLong();
    _rawData.reset(rawDataReader.GetNextBytes(GetContentSizeInLongwords()));

    Service->Log.AddLogDebug("fhfile: RDB Filesystem - Code hunk (%u), content length in bytes %u, allocate length in bytes %u\n", ID, GetContentSizeInBytes(), GetAllocateSizeInBytes());
  }

  CodeHunk::CodeHunk(ULO allocateSizeInLongwords)
    : InitialHunk(allocateSizeInLongwords)
  {
  }
}
