#include "fellow/hardfile/HardfileStructs.h"
#include "fellow/api/VM.h"

using namespace fellow::api;
using namespace fellow::hardfile::rdb;
using namespace fellow::hardfile::hunks;

namespace fellow::hardfile
{
  bool HardfileFileSystemEntry::IsOlderOrSameFileSystemVersion(ULO DOSType, ULO version)
  {
    return IsDOSType(DOSType) && IsOlderOrSameVersion(version);
  }

  bool HardfileFileSystemEntry::IsDOSType(ULO DOSType)
  {
    return Header->DOSType == DOSType;
  }

  bool HardfileFileSystemEntry::IsOlderVersion(ULO version)
  {
    return Header->Version < version;
  }

  bool HardfileFileSystemEntry::IsOlderOrSameVersion(ULO version)
  {
    return Header->Version <= version;
  }

  ULO HardfileFileSystemEntry::GetDOSType()
  {
    return Header->DOSType;
  }

  ULO HardfileFileSystemEntry::GetVersion()
  {
    return Header->Version;
  }

  void HardfileFileSystemEntry::CopyHunkToAddress(ULO destinationAddress, ULO hunkIndex)
  {
    InitialHunk* hunk = Header->FileSystemHandler.FileImage.GetInitialHunk(hunkIndex);
    hunk->SetVMAddress(destinationAddress);
    memcpy(VM->Memory.AddressToPtr(destinationAddress), hunk->GetContent(), hunk->GetContentSizeInBytes());
    if (hunk->GetAllocateSizeInBytes() > hunk->GetContentSizeInBytes())
    {
      ULO overflow = hunk->GetAllocateSizeInBytes() - hunk->GetContentSizeInBytes();
      memset(VM->Memory.AddressToPtr(destinationAddress), 0, overflow);
    }
  }

  HardfileFileSystemEntry::HardfileFileSystemEntry(RDBFileSystemHeader* header, ULO segListAddress)
    : Header(header), SegListAddress(segListAddress)
  {
  }
}
