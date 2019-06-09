#include <vector>
#include "fellow/hardfile/rdb/RDBFileSystemHandler.h"
#include "fellow/hardfile/rdb/RDBLSegBlock.h"
#include "fellow/hardfile/hunks/HunkParser.h"
#include "fellow/api/Services.h"

using namespace std;
using namespace fellow::api;
using namespace fellow::hardfile::hunks;

namespace fellow::hardfile::rdb
{
  RDBFileSystemHandler::RDBFileSystemHandler()
    : Size(0)
  {
  }

  bool RDBFileSystemHandler::ReadFromFile(RDBFileReader& reader, ULO blockChainStart, ULO blockSize)
  {
    vector<RDBLSegBlock> blocks;
    LON nextBlock = blockChainStart;

    Service->Log.AddLogDebug("Reading filesystem handler from block-chain at %d\n", blockChainStart);

    Size = 0;
    while (nextBlock != -1)
    {
      LON index = nextBlock * blockSize;
      blocks.emplace_back();
      RDBLSegBlock &block = blocks.back();
      block.ReadFromFile(reader, index);
      block.Log();

      if (!block.HasValidCheckSum)
      {
        Service->Log.AddLog("Hardfile LSegBlock had an invalid checksum.");
        return false;
      }

      if (nextBlock == block.Next)
      {
        Service->Log.AddLog("Hardfile LSegBlock next-block points to itself.");
        return false;
      }

      Size += block.GetDataSize();
      nextBlock = block.Next;
    }

    Service->Log.AddLogDebug("%d LSegBlocks read\n", blocks.size());
    Service->Log.AddLogDebug("Total filesystem size was %d bytes\n", Size);

    RawData.reset(new UBY[Size]);
    ULO nextCopyPosition = 0;
    for (const RDBLSegBlock& block : blocks)
    {
      LON size = block.GetDataSize();
      memcpy(RawData.get() + nextCopyPosition, block.GetData(), size);
      nextCopyPosition += size;
    }

    blocks.clear();

    HunkParser hunkParser(RawData.get(), Size, FileImage);
    return hunkParser.Parse();
  }
}
