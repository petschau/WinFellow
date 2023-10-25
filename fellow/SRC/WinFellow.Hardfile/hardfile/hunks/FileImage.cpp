#include "hardfile/hunks/FileImage.h"

using namespace std;

namespace fellow::hardfile::hunks
{
  void FileImage::Clear()
  {
    _header.reset();
    _initialHunks.clear();
    _additionalHunks.clear();
  }

  void FileImage::SetHeader(HeaderHunk *header)
  {
    _header.reset(header);
  }

  HeaderHunk *FileImage::GetHeader()
  {
    return _header.get();
  }

  InitialHunk *FileImage::GetInitialHunk(uint32_t hunkIndex)
  {
    return _initialHunks[hunkIndex].get();
  }

  void FileImage::AddInitialHunk(InitialHunk *hunk)
  {
    _initialHunks.push_back(unique_ptr<InitialHunk>(hunk));
  }

  uint32_t FileImage::GetInitialHunkCount()
  {
    return (uint32_t)_initialHunks.size();
  }

  AdditionalHunk *FileImage::GetAdditionalHunk(uint32_t hunkIndex)
  {
    return _additionalHunks[hunkIndex].get();
  }

  void FileImage::AddAdditionalHunk(AdditionalHunk *hunk)
  {
    _additionalHunks.push_back(unique_ptr<AdditionalHunk>(hunk));
  }

  uint32_t FileImage::GetAdditionalHunkCount()
  {
    return (uint32_t)_additionalHunks.size();
  }
}
