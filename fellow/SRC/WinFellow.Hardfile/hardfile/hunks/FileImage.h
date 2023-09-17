#ifndef FELLOW_HARDFILE_HUNKS_FILEIMAGE_H
#define FELLOW_HARDFILE_HUNKS_FILEIMAGE_H

#include <memory>
#include <vector>
#include "hardfile/hunks/HeaderHunk.h"
#include "hardfile/hunks/InitialHunk.h"
#include "hardfile/hunks/AdditionalHunk.h"

namespace fellow::hardfile::hunks
{
  class FileImage
  {
  private:
    std::unique_ptr<HeaderHunk> _header;
    std::vector<std::unique_ptr<InitialHunk>> _initialHunks;
    std::vector<std::unique_ptr<AdditionalHunk>> _additionalHunks;

  public:
    void SetHeader(HeaderHunk* header);
    HeaderHunk* GetHeader();

    InitialHunk* GetInitialHunk(uint32_t hunkIndex);
    void AddInitialHunk(InitialHunk* hunk);
    uint32_t GetInitialHunkCount();

    AdditionalHunk* GetAdditionalHunk(uint32_t hunkIndex);
    void AddAdditionalHunk(AdditionalHunk* hunk);
    uint32_t GetAdditionalHunkCount();

    void Clear();
  };
}

#endif
