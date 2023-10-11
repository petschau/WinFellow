#include "hardfile/hunks/EndHunk.h"
#include "VirtualHost/Core.h"

namespace fellow::hardfile::hunks
{
  uint32_t EndHunk::GetID()
  {
    return ID;
  }

  void EndHunk::Parse(RawDataReader& rawDataReader)
  {
    _core.Log->AddLogDebug("fhfile: RDB filesystem - End hunk (%u)\n", ID);
  }

  EndHunk::EndHunk()
    : AdditionalHunk(0)
  {
  }
}
