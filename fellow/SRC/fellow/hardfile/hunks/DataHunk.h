#pragma once

#include "fellow/hardfile/hunks/InitialHunk.h"

namespace fellow::hardfile::hunks
{
  class DataHunk : public InitialHunk
  {
  private:
    static const ULO ID = DataHunkID;

  public:
    ULO GetID() override;
    void Parse(RawDataReader &rawDataReader) override;

    DataHunk(ULO allocateSizeInLongwords);
  };
}
