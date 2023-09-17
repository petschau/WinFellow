#include "hardfile/hunks/AdditionalHunk.h"

namespace fellow::hardfile::hunks
{
  uint32_t AdditionalHunk::GetSourceHunkIndex()
  {
    return _sourceHunkIndex;
  }

  AdditionalHunk::AdditionalHunk(uint32_t sourceHunkIndex)
    : _sourceHunkIndex(sourceHunkIndex)
  {
  }
}