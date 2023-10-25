#include "hardfile/hunks/CodeHunk.h"
#include "VirtualHost/Core.h"

namespace fellow::hardfile::hunks
{
  uint32_t CodeHunk::GetID()
  {
    return ID;
  }

  void CodeHunk::Parse(RawDataReader &rawDataReader)
  {
    _contentSizeInLongwords = rawDataReader.GetNextByteswappedLong();
    _rawData.reset(rawDataReader.GetNextBytes(GetContentSizeInLongwords()));

    _core.Log->AddLogDebug(
        "fhfile: RDB Filesystem - Code hunk (%u), content length in bytes %u, allocate length in bytes %u\n", ID, GetContentSizeInBytes(), GetAllocateSizeInBytes());
  }

  CodeHunk::CodeHunk(uint32_t allocateSizeInLongwords) : InitialHunk(allocateSizeInLongwords)
  {
  }
}
