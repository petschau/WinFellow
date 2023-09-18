#ifndef FELLOW_HARDFILE_HUNKS_HUNKID_H
#define FELLOW_HARDFILE_HUNKS_HUNKID_H

#include "fellow/api/defs.h"

namespace fellow::hardfile::hunks
{
  static const uint32_t CodeHunkID = 0x3e9;
  static const uint32_t DataHunkID = 0x3ea;
  static const uint32_t BSSHunkID = 0x3eb;
  static const uint32_t Reloc32HunkID = 0x3ec;
  static const uint32_t EndHunkID = 0x3f2;
  static const uint32_t HeaderHunkID = 0x3f3;
}

#endif
