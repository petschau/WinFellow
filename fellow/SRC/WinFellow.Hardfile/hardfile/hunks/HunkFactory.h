#pragma once

#include "hardfile/hunks/InitialHunk.h"
#include "hardfile/hunks/AdditionalHunk.h"

namespace fellow::hardfile::hunks
{
  class HunkFactory
  {
  public:
    static InitialHunk* CreateInitialHunk(uint32_t type, uint32_t allocateSizeInLongwords);
    static AdditionalHunk* CreateAdditionalHunk(uint32_t type, uint32_t sourceHunkIndex);
  };
}
