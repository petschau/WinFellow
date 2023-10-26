#pragma once

#include <cstdint>

namespace fellow::hardfile::hunks
{
  struct HunkSize
  {
    uint32_t SizeInLongwords;
    uint32_t MemoryFlags;
    uint32_t AdditionalFlags;

    const char *GetMemoryFlagsToString();

    HunkSize(uint32_t sizeInLongwords, uint32_t memoryFlags, uint32_t additionalFlags);
  };
}
