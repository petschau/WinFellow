#include "hardfile/hunks/EndHunk.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

namespace fellow::hardfile::hunks
{
  uint32_t EndHunk::GetID()
  {
    return ID;
  }

  void EndHunk::Parse(RawDataReader& rawDataReader)
  {
    Service->Log.AddLogDebug("fhfile: RDB filesystem - End hunk (%u)\n", ID);
  }

  EndHunk::EndHunk()
    : AdditionalHunk(0)
  {
  }
}
