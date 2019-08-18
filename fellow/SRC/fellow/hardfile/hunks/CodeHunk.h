#pragma once

#include "fellow/hardfile/hunks/InitialHunk.h"

namespace fellow::hardfile::hunks
{
  class CodeHunk : public InitialHunk
  {
  private:
    static const ULO ID = CodeHunkID;

  public:
    ULO GetID() override;
    void Parse(RawDataReader &rawDataReader) override;

    CodeHunk(ULO allocateSizeInLongwords);
  };
}
