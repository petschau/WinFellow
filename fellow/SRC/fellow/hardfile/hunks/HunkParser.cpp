#include <memory>
#include "fellow/hardfile/hunks/HunkParser.h"
#include "fellow/hardfile/hunks/HunkFactory.h"
#include "fellow/api/Services.h"

#ifdef _DEBUG
  #define _CRTDBG_MAP_ALLOC
  #include <cstdlib>
  #include <crtdbg.h>
  #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
  #define DBG_NEW new
#endif

using namespace fellow::api;

namespace fellow::hardfile::hunks
{
  HeaderHunk* HunkParser::ParseHeader()
  {
    ULO type = _rawDataReader.GetNextByteswappedLong();
    if (type != HeaderHunkID)
    {
      Service->Log.AddLogDebug("fhfile: Header hunk in RDB Filesystem handler is not type %X - Found type %X\n", HeaderHunkID, type);
      return nullptr;
    }

    auto header = DBG_NEW HeaderHunk();
    header->Parse(_rawDataReader);
    return header;
  }

  InitialHunk* HunkParser::ParseNextInitialHunk(ULO allocateSizeInLongwords)
  {
    ULO type = _rawDataReader.GetNextByteswappedLong();
    auto hunk = HunkFactory::CreateInitialHunk(type, allocateSizeInLongwords);
    if (hunk == nullptr)
    {
      Service->Log.AddLogDebug("fhfile: Unknown initial hunk type in RDB Filesystem handler - Type %.X\n", type);
      return nullptr;
    }

    hunk->Parse(_rawDataReader);
    return hunk;
  }

  AdditionalHunk* HunkParser::ParseNextAdditionalHunk(ULO sourceHunkIndex)
  {
    ULO type = _rawDataReader.GetNextByteswappedLong();
    auto hunk = HunkFactory::CreateAdditionalHunk(type, sourceHunkIndex);
    if (hunk == nullptr)
    {
      Service->Log.AddLogDebug("fhfile: Unknown additional hunk type in RDB Filesystem handler - Type %.X\n", type);
      return nullptr;
    }

    hunk->Parse(_rawDataReader);
    return hunk;
  }

  bool HunkParser::Parse()
  {
    _fileImage.Clear();

    auto header = ParseHeader();
    if (header == nullptr)
    {
      return false;
    }

    _fileImage.SetHeader(header);

    ULO hunkCount = header->GetHunkSizeCount();
    for (ULO i = 0; i < hunkCount; i++)
    {
      auto initialHunk = ParseNextInitialHunk(header->GetHunkSize(i).SizeInLongwords);
      if (initialHunk == nullptr)
      {
        _fileImage.Clear();
        return false;
      }
      _fileImage.AddInitialHunk(initialHunk);

      auto additionalHunk = ParseNextAdditionalHunk(i);
      while (additionalHunk != nullptr && additionalHunk->GetID() != EndHunkID)
      {
        _fileImage.AddAdditionalHunk(additionalHunk);
        additionalHunk = ParseNextAdditionalHunk(i);
      }

      if (additionalHunk != nullptr && additionalHunk->GetID() == EndHunkID)
      {
        delete additionalHunk;
      }
    }
    return true;
  }

  HunkParser::HunkParser(UBY* rawData, ULO rawDataLength, FileImage& fileImage)
    : _rawDataReader(rawData, rawDataLength), _fileImage(fileImage)
  {
  }
}
