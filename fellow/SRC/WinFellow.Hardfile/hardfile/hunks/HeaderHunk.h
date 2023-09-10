#ifndef FELLOW_HARDFILE_HUNKS_HEADERHUNK_H
#define FELLOW_HARDFILE_HUNKS_HEADERHUNK_H

#include <vector>
#include <string>
#include "hardfile/hunks/HunkBase.h"
#include "hardfile/hunks/HunkSize.h"

namespace fellow::hardfile::hunks
{
  class HeaderHunk : public HunkBase
  {
  private:
    static const ULO ID = HeaderHunkID;
    std::vector<std::string> _residentLibraries;
    std::vector<HunkSize> _hunkSizes;
    ULO _firstLoadHunk;
    ULO _lastLoadHunk;

  public:
    ULO GetID() override;
    ULO GetHunkSizeCount();
    const HunkSize& GetHunkSize(ULO index);
    ULO GetResidentLibraryCount();
    const std::string& GetResidentLibrary(ULO index);
    ULO GetFirstLoadHunk();
    ULO GetLastLoadHunk();

    void Parse(RawDataReader& rawDataReader) override;

    HeaderHunk();
  };
}

#endif
