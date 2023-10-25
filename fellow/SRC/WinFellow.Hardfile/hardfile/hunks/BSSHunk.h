#pragma once

#include "hardfile/hunks/InitialHunk.h"

namespace fellow::hardfile::hunks
{
  class BSSHunk : public InitialHunk
  {
  private:
    static const uint32_t ID = BSSHunkID;

  public:
    uint32_t GetID() override;
    void Parse(RawDataReader &rawDataReader) override;

    BSSHunk(uint32_t allocateSizeInLongwords);
  };
}
