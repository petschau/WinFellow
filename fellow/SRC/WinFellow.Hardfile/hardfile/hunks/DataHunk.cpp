#include "hardfile/hunks/DataHunk.h"
#include "VirtualHost/Core.h"

namespace fellow::hardfile::hunks
{
  uint32_t DataHunk::GetID()
  {
    return ID;
  }

  void DataHunk::Parse(RawDataReader& rawDataReader)
  {
    _contentSizeInLongwords = rawDataReader.GetNextByteswappedLong();
    _rawData.reset(rawDataReader.GetNextBytes(GetContentSizeInLongwords()));

    _core.Log->AddLogDebug("fhfile: RDB filesystem - Data hunk (%u), content length in bytes %u, allocate size in bytes %u\n", ID, GetContentSizeInBytes(), GetAllocateSizeInBytes());
  }

  DataHunk::DataHunk(uint32_t allocateSizeInLongwords)
    : InitialHunk(allocateSizeInLongwords)
  {
  }
}
