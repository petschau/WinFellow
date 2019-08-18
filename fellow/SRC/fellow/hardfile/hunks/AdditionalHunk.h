#pragma once

#include "fellow/hardfile/hunks/HunkBase.h"

namespace fellow::hardfile::hunks
{
  class AdditionalHunk : public HunkBase
  {
  private:
    ULO _sourceHunkIndex;

  public:
    ULO GetSourceHunkIndex();
    void Parse(RawDataReader &rawReader) override = 0;

    AdditionalHunk(ULO sourceHunkIndex);
  };
}
