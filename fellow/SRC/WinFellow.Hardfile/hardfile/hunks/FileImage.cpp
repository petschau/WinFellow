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

  void FileImage::SetHeader(HeaderHunk* header)
  {
    _header.reset(header);
  }

  HeaderHunk* FileImage::GetHeader()
  {
    return _header.get();
  }

  InitialHunk* FileImage::GetInitialHunk(ULO hunkIndex)
  {
    return _initialHunks[hunkIndex].get();
  }

  void FileImage::AddInitialHunk(InitialHunk* hunk)
  {
    _initialHunks.push_back(unique_ptr<InitialHunk>(hunk));
  }

  ULO FileImage::GetInitialHunkCount()
  {
    return (ULO) _initialHunks.size();
  }

  AdditionalHunk* FileImage::GetAdditionalHunk(ULO hunkIndex)
  {
    return _additionalHunks[hunkIndex].get();
  }

  void FileImage::AddAdditionalHunk(AdditionalHunk* hunk)
  {
    _additionalHunks.push_back(unique_ptr<AdditionalHunk>(hunk));
  }

  ULO FileImage::GetAdditionalHunkCount()
  {
    return (ULO) _additionalHunks.size();
  }
}
