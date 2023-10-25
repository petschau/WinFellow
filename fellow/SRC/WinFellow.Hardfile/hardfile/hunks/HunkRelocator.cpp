#include "hardfile/hunks/HunkRelocator.h"
#include "fellow/api/VM.h"

using namespace fellow::api;

namespace fellow::hardfile::hunks
{
  void HunkRelocator::ProcessReloc32OffsetTable(Reloc32OffsetTable *offsetTable, uint32_t hunkBaseAddress)
  {
    InitialHunk *relatedHunk = _fileImage.GetInitialHunk(offsetTable->GetRelatedHunkIndex());
    uint32_t relatedHunkBaseAddress = relatedHunk->GetVMAddress();
    uint32_t offsetCount = offsetTable->GetOffsetCount();
    for (uint32_t i = 0; i < offsetCount; i++)
    {
      uint32_t offset = offsetTable->GetOffset(i);
      uint32_t addressToOffset = hunkBaseAddress + offset;
      uint32_t originalOffset = VM->Memory.ReadLong(addressToOffset);
      VM->Memory.WriteLong(relatedHunkBaseAddress + originalOffset, addressToOffset);
    }
  }

  void HunkRelocator::ProcessReloc32Hunk(Reloc32Hunk *reloc32Hunk, uint32_t hunkBaseAddress)
  {
    uint32_t offsetTableCount = reloc32Hunk->GetOffsetTableCount();
    for (uint32_t j = 0; j < offsetTableCount; j++)
    {
      Reloc32OffsetTable *offsetTable = reloc32Hunk->GetOffsetTable(j);
      ProcessReloc32OffsetTable(offsetTable, hunkBaseAddress);
    }
  }

  void HunkRelocator::RelocateHunk(uint32_t hunkIndex)
  {
    InitialHunk *hunk = _fileImage.GetInitialHunk(hunkIndex);
    uint32_t hunkBaseAddress = hunk->GetVMAddress();
    uint32_t additionalHunkCount = _fileImage.GetAdditionalHunkCount();
    for (uint32_t i = 0; i < additionalHunkCount; i++)
    {
      AdditionalHunk *additionalHunk = _fileImage.GetAdditionalHunk(i);
      if (additionalHunk->GetSourceHunkIndex() == hunkIndex && additionalHunk->GetID() == Reloc32HunkID)
      {
        ProcessReloc32Hunk((Reloc32Hunk *)additionalHunk, hunkBaseAddress);
      }
    }
  }

  void HunkRelocator::RelocateHunks()
  {
    uint32_t initialHunkCount = _fileImage.GetInitialHunkCount();
    for (uint32_t i = 0; i < initialHunkCount; i++)
    {
      RelocateHunk(i);
    }
  }

  HunkRelocator::HunkRelocator(FileImage &fileImage) : _fileImage(fileImage)
  {
  }
}
