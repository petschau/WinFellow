#pragma once

#include <vector>
#include <string>
#include "hardfile/hunks/HunkBase.h"
#include "hardfile/hunks/HunkSize.h"

namespace fellow::hardfile::hunks
{
  class HeaderHunk : public HunkBase
  {
  private:
    static const uint32_t ID = HeaderHunkID;
    std::vector<std::string> _residentLibraries;
    std::vector<HunkSize> _hunkSizes;
    uint32_t _firstLoadHunk;
    uint32_t _lastLoadHunk;

  public:
    uint32_t GetID() override;
    uint32_t GetHunkSizeCount();
    const HunkSize& GetHunkSize(uint32_t index);
    uint32_t GetResidentLibraryCount();
    const std::string& GetResidentLibrary(uint32_t index);
    uint32_t GetFirstLoadHunk();
    uint32_t GetLastLoadHunk();

    void Parse(RawDataReader& rawDataReader) override;

    HeaderHunk();
  };
}
