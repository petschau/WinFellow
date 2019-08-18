#include "fellow/api/defs.h"
#include "fellow/hardfile/hunks/HunkFactory.h"
#include "fellow/hardfile/hunks/CodeHunk.h"
#include "fellow/hardfile/hunks/DataHunk.h"
#include "fellow/hardfile/hunks/BSSHunk.h"
#include "fellow/hardfile/hunks/EndHunk.h"
#include "fellow/hardfile/hunks/Reloc32Hunk.h"

namespace fellow::hardfile::hunks
{
  InitialHunk* HunkFactory::CreateInitialHunk(ULO type, ULO allocateSizeInLongwords)
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

  AdditionalHunk* HunkFactory::CreateAdditionalHunk(ULO type, ULO sourceHunkIndex)
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