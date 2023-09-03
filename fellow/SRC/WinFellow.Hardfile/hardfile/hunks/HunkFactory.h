#ifndef FELLOW_HARDFILE_HUNKS_HUNKFACTORY_H
#define FELLOW_HARDFILE_HUNKS_HUNKFACTORY_H

#include "hardfile/hunks/InitialHunk.h"
#include "hardfile/hunks/AdditionalHunk.h"

namespace fellow::hardfile::hunks
{
  class HunkFactory
  {
  public:
    static InitialHunk* CreateInitialHunk(ULO type, ULO allocateSizeInLongwords);
    static AdditionalHunk* CreateAdditionalHunk(ULO type, ULO sourceHunkIndex);
  };
}

#endif
