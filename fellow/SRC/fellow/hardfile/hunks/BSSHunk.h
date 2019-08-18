#pragma once

#include "fellow/hardfile/hunks/InitialHunk.h"

namespace fellow::hardfile::hunks
{
  class BSSHunk : public InitialHunk
  {
  private:
    static const ULO ID = BSSHunkID;

  public:
    ULO GetID() override;
    void Parse(RawDataReader &rawDataReader) override;

    BSSHunk(ULO allocateSizeInLongwords);
  };
}
