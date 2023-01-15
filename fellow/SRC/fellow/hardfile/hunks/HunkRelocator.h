#pragma once

#include "fellow/hardfile/hunks/FileImage.h"
#include "fellow/hardfile/hunks/Reloc32Hunk.h"
#include "fellow/api/vm/IMemorySystem.h"

namespace fellow::hardfile::hunks
{
  class HunkRelocator
  {
  private:
    FileImage &_fileImage;
    fellow::api::vm::IMemorySystem *_vmMemory;

    void ProcessReloc32OffsetTable(Reloc32OffsetTable *offsetTable, ULO hunkBaseAddress);
    void ProcessReloc32Hunk(Reloc32Hunk *reloc32Hunk, ULO hunkBaseAddress);
    void RelocateHunk(ULO hunkIndex);

  public:
    void RelocateHunks();

    HunkRelocator(FileImage &fileImage, fellow::api::vm::IMemorySystem *vmMemory);
  };
}
