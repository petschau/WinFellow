#pragma once

#include "fellow/hardfile/hunks/AdditionalHunk.h"

namespace fellow::hardfile::hunks
{
  class EndHunk : public AdditionalHunk
  {
  private:
    static const ULO ID = EndHunkID;

  public:
    ULO GetID() override;
    void Parse(RawDataReader &rawDataReader) override;

    EndHunk();
  };
}
