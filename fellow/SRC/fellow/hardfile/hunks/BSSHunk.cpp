#include <cstring>
#include "fellow/hardfile/hunks/BSSHunk.h"
#include "fellow/api/Services.h"

#ifdef _DEBUG
  #define _CRTDBG_MAP_ALLOC
  #include <cstdlib>
  #include <crtdbg.h>
  #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
  #define DBG_NEW new
#endif

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
    _rawData.reset(DBG_NEW UBY[size]);
    memset(_rawData.get(), 0, size);

    Service->Log.AddLogDebug("fhfile: RDB filesystem - BSS hunk (%u), content length in bytes %u, allocate length in bytes %u\n", ID, size, GetAllocateSizeInBytes());
  }

  BSSHunk::BSSHunk(ULO allocateSizeInLongwords)
    : InitialHunk(allocateSizeInLongwords)
  {
  }
}
