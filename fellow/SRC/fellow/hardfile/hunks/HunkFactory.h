#pragma once

#include "fellow/hardfile/hunks/InitialHunk.h"
#include "fellow/hardfile/hunks/AdditionalHunk.h"

namespace fellow::hardfile::hunks
{
  class HunkFactory
  {
  public:
    static InitialHunk *CreateInitialHunk(ULO type, ULO allocateSizeInLongwords);
    static AdditionalHunk *CreateAdditionalHunk(ULO type, ULO sourceHunkIndex);
  };
}
