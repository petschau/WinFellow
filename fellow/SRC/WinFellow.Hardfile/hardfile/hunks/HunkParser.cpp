#include <memory>
#include "Defs.h"
#include "hardfile/hunks/HunkParser.h"
#include "hardfile/hunks/HunkFactory.h"
#include "VirtualHost/Core.h"

namespace fellow::hardfile::hunks
{
  HeaderHunk *HunkParser::ParseHeader()
  {
    uint32_t type = _rawDataReader.GetNextByteswappedLong();
    if (type != HeaderHunkID)
    {
      _core.Log->AddLogDebug("fhfile: Header hunk in RDB Filesystem handler is not type %X - Found type %X\n", HeaderHunkID, type);
      return nullptr;
    }

    auto header = new HeaderHunk();
    header->Parse(_rawDataReader);
    return header;
  }

  InitialHunk *HunkParser::ParseNextInitialHunk(uint32_t allocateSizeInLongwords)
  {
    uint32_t type = _rawDataReader.GetNextByteswappedLong();
    auto hunk = HunkFactory::CreateInitialHunk(type, allocateSizeInLongwords);
    if (hunk == nullptr)
    {
      _core.Log->AddLogDebug("fhfile: Unknown initial hunk type in RDB Filesystem handler - Type %.X\n", type);
      return nullptr;
    }

    hunk->Parse(_rawDataReader);
    return hunk;
  }

  AdditionalHunk *HunkParser::ParseNextAdditionalHunk(uint32_t sourceHunkIndex)
  {
    uint32_t type = _rawDataReader.GetNextByteswappedLong();
    auto hunk = HunkFactory::CreateAdditionalHunk(type, sourceHunkIndex);
    if (hunk == nullptr)
    {
      _core.Log->AddLogDebug("fhfile: Unknown additional hunk type in RDB Filesystem handler - Type %.X\n", type);
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

    uint32_t hunkCount = header->GetHunkSizeCount();
    for (uint32_t i = 0; i < hunkCount; i++)
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

  HunkParser::HunkParser(uint8_t *rawData, uint32_t rawDataLength, FileImage &fileImage) : _rawDataReader(rawData, rawDataLength), _fileImage(fileImage)
  {
  }
}
