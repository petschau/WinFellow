#ifndef FELLOW_HARDFILE_HUNKS_ENDHUNK_H
#define FELLOW_HARDFILE_HUNKS_ENDHUNK_H

#include "fellow/hardfile/hunks/AdditionalHunk.h"

namespace fellow::hardfile::hunks
{
  class EndHunk : public AdditionalHunk
  {
  private:
    static const ULO ID = EndHunkID;

  public:
    ULO GetID() override;
    void Parse(RawDataReader& rawDataReader) override;

    EndHunk();
  };
}

#endif
