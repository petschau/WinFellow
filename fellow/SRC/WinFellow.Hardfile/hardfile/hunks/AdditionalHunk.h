#pragma once

#include "hardfile/hunks/HunkBase.h"

namespace fellow::hardfile::hunks
{
  class AdditionalHunk : public HunkBase
  {
  private:
    uint32_t _sourceHunkIndex;

  public:
    uint32_t GetSourceHunkIndex();
    void Parse(RawDataReader& rawReader) override = 0;

    AdditionalHunk(uint32_t sourceHunkIndex);
  };
}
