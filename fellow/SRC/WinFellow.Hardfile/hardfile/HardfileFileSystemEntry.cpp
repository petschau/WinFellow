#include "hardfile/HardfileFileSystemEntry.h"
#include "DebugApi/IMemorySystem.h"

using namespace Debug;
using namespace fellow::hardfile::rdb;
using namespace fellow::hardfile::hunks;

namespace fellow::hardfile
{
  bool HardfileFileSystemEntry::IsOlderOrSameFileSystemVersion(uint32_t DOSType, uint32_t version)
  {
    return IsDOSType(DOSType) && IsOlderOrSameVersion(version);
  }

  bool HardfileFileSystemEntry::IsDOSType(uint32_t DOSType)
  {
    return Header->DOSType == DOSType;
  }

  bool HardfileFileSystemEntry::IsOlderVersion(uint32_t version)
  {
    return Header->Version < version;
  }

  bool HardfileFileSystemEntry::IsOlderOrSameVersion(uint32_t version)
  {
    return Header->Version <= version;
  }

  uint32_t HardfileFileSystemEntry::GetDOSType()
  {
    return Header->DOSType;
  }

  uint32_t HardfileFileSystemEntry::GetVersion()
  {
    return Header->Version;
  }

  void HardfileFileSystemEntry::CopyHunkToAddress(uint32_t destinationAddress, uint32_t hunkIndex)
  {
    InitialHunk *hunk = Header->FileSystemHandler.FileImage.GetInitialHunk(hunkIndex);
    hunk->SetVMAddress(destinationAddress);
    memcpy(Memory.AddressToPtr(destinationAddress), hunk->GetContent(), hunk->GetContentSizeInBytes());
    if (hunk->GetAllocateSizeInBytes() > hunk->GetContentSizeInBytes())
    {
      uint32_t overflow = hunk->GetAllocateSizeInBytes() - hunk->GetContentSizeInBytes();
      memset(Memory.AddressToPtr(destinationAddress), 0, overflow);
    }
  }

  HardfileFileSystemEntry::HardfileFileSystemEntry(IMemorySystem &memory, RDBFileSystemHeader *header, uint32_t segListAddress)
    : Memory(memory), Header(header), SegListAddress(segListAddress)
  {
  }
}
