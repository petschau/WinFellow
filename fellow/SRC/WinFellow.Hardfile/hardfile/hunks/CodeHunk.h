#pragma once

#include "hardfile/hunks/InitialHunk.h"

namespace fellow::hardfile::hunks
{
  class CodeHunk : public InitialHunk
  {
  private:
    static const uint32_t ID = CodeHunkID;

  public:
    uint32_t GetID() override;
    void Parse(RawDataReader& rawDataReader) override;

    CodeHunk(uint32_t allocateSizeInLongwords);
  };
}
