#include "hardfile/hunks/AdditionalHunk.h"

namespace fellow::hardfile::hunks
{
  ULO AdditionalHunk::GetSourceHunkIndex()
  {
    return _sourceHunkIndex;
  }

  AdditionalHunk::AdditionalHunk(ULO sourceHunkIndex)
    : _sourceHunkIndex(sourceHunkIndex)
  {
  }
}