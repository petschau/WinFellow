#pragma once

#include "Debug/IMemorySystem.h"
#include "hardfile/hunks/FileImage.h"
#include "hardfile/hunks/Reloc32Hunk.h"

namespace fellow::hardfile::hunks
{
  class HunkRelocator
  {
  private:
    Debug::IMemorySystem &_memory;
    FileImage &_fileImage;

    void ProcessReloc32OffsetTable(Reloc32OffsetTable *offsetTable, uint32_t hunkBaseAddress);
    void ProcessReloc32Hunk(Reloc32Hunk *reloc32Hunk, uint32_t hunkBaseAddress);
    void RelocateHunk(uint32_t hunkIndex);

  public:
    void RelocateHunks();

    HunkRelocator(Debug::IMemorySystem& memory, FileImage &fileImage);
  };
}
