#include "fellow/api/defs.h"
#include "hardfile/hunks/HunkFactory.h"
#include "hardfile/hunks/CodeHunk.h"
#include "hardfile/hunks/DataHunk.h"
#include "hardfile/hunks/BSSHunk.h"
#include "hardfile/hunks/EndHunk.h"
#include "hardfile/hunks/Reloc32Hunk.h"

namespace fellow::hardfile::hunks
{
  InitialHunk *HunkFactory::CreateInitialHunk(uint32_t type, uint32_t allocateSizeInLongwords)
  {
    // The lower 29 bits are used, except on header
    switch (type & 0x1fffffff)
    {
      case CodeHunkID: return new CodeHunk(allocateSizeInLongwords);
      case DataHunkID: return new DataHunk(allocateSizeInLongwords);
      case BSSHunkID: return new BSSHunk(allocateSizeInLongwords);
      default: return nullptr;
    }
  }

  AdditionalHunk *HunkFactory::CreateAdditionalHunk(uint32_t type, uint32_t sourceHunkIndex)
  {
    // The lower 29 bits are used, except on header
    switch (type & 0x1fffffff)
    {
      case Reloc32HunkID: return new Reloc32Hunk(sourceHunkIndex);
      case EndHunkID: return new EndHunk();
      default: return nullptr;
    }
  }
}