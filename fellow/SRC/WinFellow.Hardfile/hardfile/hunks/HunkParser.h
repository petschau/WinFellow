#pragma once

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
    InitialHunk* ParseNextInitialHunk(uint32_t allocateSizeInLongwords);
    AdditionalHunk* ParseNextAdditionalHunk(uint32_t sourceHunkIndex);

  public:
    bool Parse();

    HunkParser(uint8_t *rawData, uint32_t rawDataLength, FileImage& fileImage);
  };
}
