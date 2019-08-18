#pragma once

#include "fellow/api/defs.h"

namespace fellow::hardfile::hunks
{
  struct HunkSize
  {
    ULO SizeInLongwords;
    ULO MemoryFlags;
    ULO AdditionalFlags;

    const STR *GetMemoryFlagsToString();

    HunkSize(ULO sizeInLongwords, ULO memoryFlags, ULO additionalFlags);
  };
}
