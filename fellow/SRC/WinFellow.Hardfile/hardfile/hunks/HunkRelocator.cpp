#include "hardfile/hunks/HunkRelocator.h"
#include "fellow/api/VM.h"

using namespace fellow::api;

namespace fellow::hardfile::hunks
{
  void HunkRelocator::ProcessReloc32OffsetTable(Reloc32OffsetTable *offsetTable, ULO hunkBaseAddress)
  {
    InitialHunk *relatedHunk = _fileImage.GetInitialHunk(offsetTable->GetRelatedHunkIndex());
    ULO relatedHunkBaseAddress = relatedHunk->GetVMAddress();
    ULO offsetCount = offsetTable->GetOffsetCount();
    for (ULO i = 0; i < offsetCount; i++)
    {
      ULO offset = offsetTable->GetOffset(i);
      ULO addressToOffset = hunkBaseAddress + offset;
      ULO originalOffset = VM->Memory.ReadLong(addressToOffset);
      VM->Memory.WriteLong(relatedHunkBaseAddress + originalOffset, addressToOffset);
    }
  }

  void HunkRelocator::ProcessReloc32Hunk(Reloc32Hunk *reloc32Hunk, ULO hunkBaseAddress)
  {
    ULO offsetTableCount = reloc32Hunk->GetOffsetTableCount();
    for (ULO j = 0; j < offsetTableCount; j++)
    {
      Reloc32OffsetTable *offsetTable = reloc32Hunk->GetOffsetTable(j);
      ProcessReloc32OffsetTable(offsetTable, hunkBaseAddress);
    }
  }

  void HunkRelocator::RelocateHunk(ULO hunkIndex)
  {
    InitialHunk* hunk = _fileImage.GetInitialHunk(hunkIndex);
    ULO hunkBaseAddress = hunk->GetVMAddress();
    ULO additionalHunkCount = _fileImage.GetAdditionalHunkCount();
    for (ULO i = 0; i < additionalHunkCount; i++)
    {
      AdditionalHunk *additionalHunk = _fileImage.GetAdditionalHunk(i);
      if (additionalHunk->GetSourceHunkIndex() == hunkIndex && additionalHunk->GetID() == Reloc32HunkID)
      {
        ProcessReloc32Hunk((Reloc32Hunk*)additionalHunk, hunkBaseAddress);
      }
    }
  }

  void HunkRelocator::RelocateHunks()
  {
    ULO initialHunkCount = _fileImage.GetInitialHunkCount();
    for (ULO i = 0; i < initialHunkCount; i++)
    {
      RelocateHunk(i);
    }
  }

  HunkRelocator::HunkRelocator(FileImage& fileImage)
    : _fileImage(fileImage)
  {
  }
}
