#include "hardfile/hunks/HeaderHunk.h"
#include "VirtualHost/Core.h"

namespace fellow::hardfile::hunks
{
  uint32_t HeaderHunk::GetID()
  {
    return ID;
  }

  uint32_t HeaderHunk::GetHunkSizeCount()
  {
    return (uint32_t)_hunkSizes.size();
  }

  const HunkSize& HeaderHunk::GetHunkSize(uint32_t index)
  {
    return _hunkSizes[index];
  }

  uint32_t HeaderHunk::GetResidentLibraryCount()
  {
    return (uint32_t)_residentLibraries.size();
  }

  const std::string& HeaderHunk::GetResidentLibrary(uint32_t index)
  {
    return _residentLibraries[index];
  }

  uint32_t HeaderHunk::GetFirstLoadHunk()
  {
    return _firstLoadHunk;
  }

  uint32_t HeaderHunk::GetLastLoadHunk()
  {
    return _lastLoadHunk;
  }

  void HeaderHunk::Parse(RawDataReader& rawDataReader)
  {
    _core.Log->AddLogDebug("fhfile: RDB filesystem - Header hunk (%u)\n", ID);

    uint32_t stringLength = rawDataReader.GetNextByteswappedLong();
    while (stringLength != 0)
    {
      _residentLibraries.push_back(rawDataReader.GetNextString(stringLength));
      _core.Log->AddLogDebug("fhfile: RDB filesystem - Header hunk resident library entry '%s'\n", _residentLibraries.back().c_str());

      stringLength = rawDataReader.GetNextByteswappedLong();
    }

    uint32_t tableSize = rawDataReader.GetNextByteswappedLong();

    _core.Log->AddLogDebug("fhfile: RDB filesystem - Header hunk table size: %u\n", tableSize);

    _firstLoadHunk = rawDataReader.GetNextByteswappedLong();
    _lastLoadHunk = rawDataReader.GetNextByteswappedLong();

    _core.Log->AddLogDebug("fhfile: RDB filesystem - Header hunk first load %u last load %u\n", _firstLoadHunk, _lastLoadHunk);

    for (uint32_t i = _firstLoadHunk; i <= _lastLoadHunk; i++)
    {
      uint32_t hunkSize = rawDataReader.GetNextByteswappedLong();
      uint32_t memoryFlags = hunkSize >> 30;
      uint32_t additionalFlags = 0;

      hunkSize &= 0x3fffffff;

      if (memoryFlags == 3)
      {
        additionalFlags = rawDataReader.GetNextByteswappedLong();
      }
      _hunkSizes.emplace_back(hunkSize, memoryFlags, additionalFlags);

      _core.Log->AddLogDebug("fhfile: RDB filesystem - Header hunk table entry %u size: %u %s\n", i, _hunkSizes.back().SizeInLongwords * 4, _hunkSizes.back().GetMemoryFlagsToString());
    }
  }

  HeaderHunk::HeaderHunk()
    : _firstLoadHunk(0), _lastLoadHunk(0)
  {
  }
}
