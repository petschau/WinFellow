#pragma once

#include "hardfile/hunks/AdditionalHunk.h"

namespace fellow::hardfile::hunks
{
  class EndHunk : public AdditionalHunk
  {
  private:
    static const uint32_t ID = EndHunkID;

  public:
    uint32_t GetID() override;
    void Parse(RawDataReader& rawDataReader) override;

    EndHunk();
  };
}
