#include "fellow/hardfile/hunks/HunkFactory.h"
#include "fellow/hardfile/hunks/CodeHunk.h"
#include "fellow/hardfile/hunks/DataHunk.h"
#include "fellow/hardfile/hunks/BSSHunk.h"
#include "fellow/hardfile/hunks/EndHunk.h"
#include "fellow/hardfile/hunks/Reloc32Hunk.h"

#ifdef _DEBUG
  #define _CRTDBG_MAP_ALLOC
  #include <cstdlib>
  #include <crtdbg.h>
  #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
  #define DBG_NEW new
#endif

namespace fellow::hardfile::hunks
{
  InitialHunk* HunkFactory::CreateInitialHunk(ULO type, ULO allocateSizeInLongwords)
  {
    // The lower 29 bits are used, except on header
    switch (type & 0x1fffffff)
    {
    case CodeHunkID: return DBG_NEW CodeHunk(allocateSizeInLongwords);
    case DataHunkID: return DBG_NEW DataHunk(allocateSizeInLongwords);
    case BSSHunkID: return DBG_NEW BSSHunk(allocateSizeInLongwords);
    default: return nullptr;
    }
  }

  AdditionalHunk* HunkFactory::CreateAdditionalHunk(ULO type, ULO sourceHunkIndex)
  {
    // The lower 29 bits are used, except on header
    switch (type & 0x1fffffff)
    {
    case Reloc32HunkID: return DBG_NEW Reloc32Hunk(sourceHunkIndex);
    case EndHunkID: return DBG_NEW EndHunk();
    default: return nullptr;
    }
  }
}