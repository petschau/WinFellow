#ifndef FELLOW_HARDFILE_HUNKS_INITIALHUNK_H
#define FELLOW_HARDFILE_HUNKS_INITIALHUNK_H

#include <memory>
#include "hardfile/hunks/HunkBase.h"

namespace fellow::hardfile::hunks
{
  class InitialHunk : public HunkBase
  {
  protected:
    uint32_t _allocateSizeInLongwords;
    uint32_t _contentSizeInLongwords;
    uint32_t _vmAddress;
    std::unique_ptr<UBY> _rawData;

  public:
    void Parse(RawDataReader& rawReader) override = 0;
    uint32_t GetAllocateSizeInLongwords();
    uint32_t GetAllocateSizeInBytes();
    uint32_t GetContentSizeInLongwords();
    uint32_t GetContentSizeInBytes();
    UBY *GetContent();
    void SetVMAddress(uint32_t vmAddress);
    uint32_t GetVMAddress();

    InitialHunk(uint32_t allocateSizeInLongwords);
  };
}

#endif
