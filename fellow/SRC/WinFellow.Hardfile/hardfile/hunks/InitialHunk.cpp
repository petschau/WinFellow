#include "hardfile/hunks/InitialHunk.h"

namespace fellow::hardfile::hunks
{
  uint32_t InitialHunk::GetAllocateSizeInLongwords()
  {
    return _allocateSizeInLongwords & 0x3fffffff;
  }

  uint32_t InitialHunk::GetAllocateSizeInBytes()
  {
    return GetAllocateSizeInLongwords() * 4;
  }

  uint32_t InitialHunk::GetContentSizeInLongwords()
  {
    return _contentSizeInLongwords;
  }

  uint32_t InitialHunk::GetContentSizeInBytes()
  {
    return GetContentSizeInLongwords() * 4;
  }

  uint8_t *InitialHunk::GetContent()
  {
    return _rawData.get();
  }

  void InitialHunk::SetVMAddress(uint32_t vmAddress)
  {
    _vmAddress = vmAddress;
  }

  uint32_t InitialHunk::GetVMAddress()
  {
    return _vmAddress;
  }

  InitialHunk::InitialHunk(uint32_t allocateSizeInLongwords) : _allocateSizeInLongwords(allocateSizeInLongwords), _contentSizeInLongwords(0), _vmAddress(0), _rawData(nullptr)
  {
  }
}
