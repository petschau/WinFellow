#pragma once

#include <memory>
#include <vector>
#include "fellow/hardfile/hunks/HeaderHunk.h"
#include "fellow/hardfile/hunks/InitialHunk.h"
#include "fellow/hardfile/hunks/AdditionalHunk.h"

namespace fellow::hardfile::hunks
{
  class FileImage
  {
  private:
    std::unique_ptr<HeaderHunk> _header;
    std::vector<std::unique_ptr<InitialHunk>> _initialHunks;
    std::vector<std::unique_ptr<AdditionalHunk>> _additionalHunks;

  public:
    void SetHeader(HeaderHunk *header);
    HeaderHunk *GetHeader();

    InitialHunk *GetInitialHunk(ULO hunkIndex);
    void AddInitialHunk(InitialHunk *hunk);
    ULO GetInitialHunkCount();

    AdditionalHunk *GetAdditionalHunk(ULO hunkIndex);
    void AddAdditionalHunk(AdditionalHunk *hunk);
    ULO GetAdditionalHunkCount();

    void Clear();
  };
}
