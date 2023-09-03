#ifndef FELLOW_HARDFILE_HUNKS_HUNKPARSER_H
#define FELLOW_HARDFILE_HUNKS_HUNKPARSER_H

#include "hardfile/hunks/RawDataReader.h"
#include "hardfile/hunks/FileImage.h"

namespace fellow::hardfile::hunks
{
  class HunkParser
  {
  private:
    RawDataReader _rawDataReader;
    FileImage& _fileImage;

    HeaderHunk* ParseHeader();
    InitialHunk* ParseNextInitialHunk(ULO allocateSizeInLongwords);
    AdditionalHunk* ParseNextAdditionalHunk(ULO sourceHunkIndex);

  public:
    bool Parse();

    HunkParser(UBY *rawData, ULO rawDataLength, FileImage& fileImage);
  };
}

#endif
