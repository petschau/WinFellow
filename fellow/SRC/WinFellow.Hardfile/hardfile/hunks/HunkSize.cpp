#include "hardfile/hunks/HunkSize.h"

namespace fellow::hardfile::hunks
{
  const STR* HunkSize::GetMemoryFlagsToString()
  {
    switch (MemoryFlags)
    {
    case 0: return "Any memory";
    case 1: return "Chip memory";
    case 2: return "Fast memory";
    default: return "With additional memory flags";
    }
  }

  HunkSize::HunkSize(uint32_t sizeInLongwords, uint32_t memoryFlags, uint32_t additionalFlags)
    : SizeInLongwords(sizeInLongwords), MemoryFlags(memoryFlags), AdditionalFlags(additionalFlags)
  {
  }
}
