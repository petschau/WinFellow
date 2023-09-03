#include "hardfile/hunks/HeaderHunk.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

namespace fellow::hardfile::hunks
{
  ULO HeaderHunk::GetID()
  {
    return ID;
  }

  ULO HeaderHunk::GetHunkSizeCount()
  {
    return (ULO) _hunkSizes.size();
  }

  const HunkSize& HeaderHunk::GetHunkSize(ULO index)
  {
    return _hunkSizes[index];
  }

  ULO HeaderHunk::GetResidentLibraryCount()
  {
    return (ULO) _residentLibraries.size();
  }

  const std::string& HeaderHunk::GetResidentLibrary(ULO index)
  {
    return _residentLibraries[index];
  }

  ULO HeaderHunk::GetFirstLoadHunk()
  {
    return _firstLoadHunk;
  }

  ULO HeaderHunk::GetLastLoadHunk()
  {
    return _lastLoadHunk;
  }

  void HeaderHunk::Parse(RawDataReader& rawDataReader)
  {
    Service->Log.AddLogDebug("fhfile: RDB filesystem - Header hunk (%u)\n", ID);

    ULO stringLength = rawDataReader.GetNextByteswappedLong();
    while (stringLength != 0)
    {
      _residentLibraries.push_back(rawDataReader.GetNextString(stringLength));
      Service->Log.AddLogDebug("fhfile: RDB filesystem - Header hunk resident library entry '%s'\n", _residentLibraries.back().c_str());

      stringLength = rawDataReader.GetNextByteswappedLong();
    }

    ULO tableSize = rawDataReader.GetNextByteswappedLong();

    Service->Log.AddLogDebug("fhfile: RDB filesystem - Header hunk table size: %u\n", tableSize);

    _firstLoadHunk = rawDataReader.GetNextByteswappedLong();
    _lastLoadHunk = rawDataReader.GetNextByteswappedLong();

    Service->Log.AddLogDebug("fhfile: RDB filesystem - Header hunk first load %u last load %u\n", _firstLoadHunk, _lastLoadHunk);

    for (ULO i = _firstLoadHunk; i <= _lastLoadHunk; i++)
    {
      ULO hunkSize = rawDataReader.GetNextByteswappedLong();
      ULO memoryFlags = hunkSize >> 30;
      ULO additionalFlags = 0;

      hunkSize &= 0x3fffffff;

      if (memoryFlags == 3)
      {
        additionalFlags = rawDataReader.GetNextByteswappedLong();
      }
      _hunkSizes.emplace_back(hunkSize, memoryFlags, additionalFlags);

      Service->Log.AddLogDebug("fhfile: RDB filesystem - Header hunk table entry %u size: %u %s\n", i, _hunkSizes.back().SizeInLongwords * 4, _hunkSizes.back().GetMemoryFlagsToString());
    }
  }

  HeaderHunk::HeaderHunk()
    : _firstLoadHunk(0), _lastLoadHunk(0)  
  {
  }
}
