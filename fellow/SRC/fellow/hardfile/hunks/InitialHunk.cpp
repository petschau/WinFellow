#include "fellow/hardfile/hunks/InitialHunk.h"

namespace fellow::hardfile::hunks
{
  ULO InitialHunk::GetAllocateSizeInLongwords()
  {
    return _allocateSizeInLongwords & 0x3fffffff;
  }

  ULO InitialHunk::GetAllocateSizeInBytes()
  {
    return GetAllocateSizeInLongwords() * 4;
  }

  ULO InitialHunk::GetContentSizeInLongwords()
  {
    return _contentSizeInLongwords;
  }

  ULO InitialHunk::GetContentSizeInBytes()
  {
    return GetContentSizeInLongwords() * 4;
  }

  UBY *InitialHunk::GetContent()
  {
    return _rawData.get();
  }

  void InitialHunk::SetVMAddress(ULO vmAddress)
  {
    _vmAddress = vmAddress;
  }

  ULO InitialHunk::GetVMAddress()
  {
    return _vmAddress;
  }

  InitialHunk::InitialHunk(ULO allocateSizeInLongwords)
    : _allocateSizeInLongwords(allocateSizeInLongwords), _contentSizeInLongwords(0), _vmAddress(0), _rawData(nullptr)
  {
  }
}
