#ifndef FELLOW_HARDFILE_HUNKS_HUNKRELOCATOR_H
#define FELLOW_HARDFILE_HUNKS_HUNKRELOCATOR_H

#include "hardfile/hunks/FileImage.h"
#include "hardfile/hunks/Reloc32Hunk.h"

namespace fellow::hardfile::hunks
{
  class HunkRelocator
  {
  private:
    FileImage& _fileImage;

    void ProcessReloc32OffsetTable(Reloc32OffsetTable *offsetTable, ULO hunkBaseAddress);
    void ProcessReloc32Hunk(Reloc32Hunk *reloc32Hunk, ULO hunkBaseAddress);
    void RelocateHunk(ULO hunkIndex);

  public:
    void RelocateHunks();

    HunkRelocator(FileImage& fileImage);
  };
}

#endif
