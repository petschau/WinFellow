#include <cstring>
#include "fellow/api/defs.h"
#include "hardfile/hunks/BSSHunk.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

namespace fellow::hardfile::hunks
{
  ULO BSSHunk::GetID()
  {
    return ID;
  }

  void BSSHunk::Parse(RawDataReader& rawDataReader)
  {
    _contentSizeInLongwords = rawDataReader.GetNextByteswappedLong();
    ULO size = GetContentSizeInBytes();
    _rawData.reset(new UBY[size]);
    memset(_rawData.get(), 0, size);

    Service->Log.AddLogDebug("fhfile: RDB filesystem - BSS hunk (%u), content length in bytes %u, allocate length in bytes %u\n", ID, size, GetAllocateSizeInBytes());
  }

  BSSHunk::BSSHunk(ULO allocateSizeInLongwords)
    : InitialHunk(allocateSizeInLongwords)
  {
  }
}
