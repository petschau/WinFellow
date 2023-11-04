#include <cstring>
#include "Defs.h"
#include "hardfile/hunks/BSSHunk.h"
#include "VirtualHost/Core.h"

namespace fellow::hardfile::hunks
{
  uint32_t BSSHunk::GetID()
  {
    return ID;
  }

  void BSSHunk::Parse(RawDataReader &rawDataReader)
  {
    _contentSizeInLongwords = rawDataReader.GetNextByteswappedLong();
    uint32_t size = GetContentSizeInBytes();
    _rawData.reset(new uint8_t[size]);
    memset(_rawData.get(), 0, size);

    _core.Log->AddLogDebug("fhfile: RDB filesystem - BSS hunk (%u), content length in bytes %u, allocate length in bytes %u\n", ID, size, GetAllocateSizeInBytes());
  }

  BSSHunk::BSSHunk(uint32_t allocateSizeInLongwords) : InitialHunk(allocateSizeInLongwords)
  {
  }
}
