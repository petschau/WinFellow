#ifndef FELLOW_HARDFILE_HUNKS_HUNKSIZE_H
#define FELLOW_HARDFILE_HUNKS_HUNKSIZE_H

#include "fellow/api/defs.h"

namespace fellow::hardfile::hunks
{
  struct HunkSize
  {
    ULO SizeInLongwords;
    ULO MemoryFlags;
    ULO AdditionalFlags;

    const STR* GetMemoryFlagsToString();

    HunkSize(ULO sizeInLongwords, ULO memoryFlags, ULO additionalFlags);
  };
}

#endif
