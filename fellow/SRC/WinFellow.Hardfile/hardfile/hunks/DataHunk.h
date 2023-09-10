#ifndef FELLOW_HARDFILE_HUNKS_DATAHUNK_H
#define FELLOW_HARDFILE_HUNKS_DATAHUNK_H

#include "hardfile/hunks/InitialHunk.h"

namespace fellow::hardfile::hunks
{
  class DataHunk : public InitialHunk
  {
  private:
    static const ULO ID = DataHunkID;

  public:
    ULO GetID() override;
    void Parse(RawDataReader& rawDataReader) override;

    DataHunk(ULO allocateSizeInLongwords);
  };
}

#endif
