#ifndef FELLOW_HARDFILE_HUNKS_DATAHUNK_H
#define FELLOW_HARDFILE_HUNKS_DATAHUNK_H

#include "hardfile/hunks/InitialHunk.h"

namespace fellow::hardfile::hunks
{
  class DataHunk : public InitialHunk
  {
  private:
    static const uint32_t ID = DataHunkID;

  public:
    uint32_t GetID() override;
    void Parse(RawDataReader& rawDataReader) override;

    DataHunk(uint32_t allocateSizeInLongwords);
  };
}

#endif
