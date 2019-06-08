#include "fellow/hardfile/rdb/RDBFileSystemHeader.h"
#include "fellow/hardfile/rdb/CheckSumCalculator.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

namespace fellow::hardfile::rdb
{
  void RDBFileSystemHeader::ReadFromFile(RDBFileReader& reader, ULO blockChainStart, ULO blockSize)
  {
    ULO index = blockSize * blockChainStart;

    SizeInLongs = reader.ReadULO(index + 4);
    CheckSum = reader.ReadLON(index + 8);
    HostID = reader.ReadULO(index + 12);
    Next = reader.ReadULO(index + 16);
    Flags = reader.ReadULO(index + 20);
    DOSType = reader.ReadULO(index + 32);
    Version = reader.ReadULO(index + 36);
    PatchFlags = reader.ReadULO(index + 40);

    // Device node
    DnType = reader.ReadULO(index + 44);
    DnTask = reader.ReadULO(index + 48);
    DnLock = reader.ReadULO(index + 52);
    DnHandler = reader.ReadULO(index + 56);
    DnStackSize = reader.ReadULO(index + 60);
    DnPriority = reader.ReadULO(index + 64);
    DnStartup = reader.ReadULO(index + 68);
    DnSegListBlock = reader.ReadULO(index + 72);
    DnGlobalVec = reader.ReadULO(index + 76);

    // Reserved for additional patchflags
    for (int i = 0; i < 23; i++)
    {
      Reserved2[i] = reader.ReadULO(index + i + 80);
    }

    HasValidCheckSum = (SizeInLongs == 64) && CheckSumCalculator::HasValidCheckSum(reader, 64, index);

    if (!HasValidCheckSum)
    {
      return;
    }

    bool fileSystemHandlerLoadSuccessful = FileSystemHandler.ReadFromFile(reader, DnSegListBlock, blockSize);
    if (!fileSystemHandlerLoadSuccessful)
    {
      HasFileSystemDataErrors = true;
      return;
    }
  }

  void RDBFileSystemHeader::Log()
  {
    Service->Log.AddLogDebug("Filesystem header block\n");
    Service->Log.AddLogDebug("-----------------------------------------\n");
    Service->Log.AddLogDebug("0  - id:                     FSHD\n");
    Service->Log.AddLogDebug("4  - size in longs:          %u\n", SizeInLongs);
    Service->Log.AddLogDebug("8  - checksum:               %d (%s)\n", CheckSum, HasValidCheckSum ? "Valid" : "Invalid");
    Service->Log.AddLogDebug("12 - host id:                %u\n", HostID);
    Service->Log.AddLogDebug("16 - next:                   %d\n", Next);
    Service->Log.AddLogDebug("20 - flags:                  %X\n", Flags);
    Service->Log.AddLogDebug("32 - dos type:               %.8X\n", DOSType);
    Service->Log.AddLogDebug("36 - version:                %.8X ie %d.%d\n", Version, (Version & 0xffff0000) >> 16, Version & 0xffff);
    Service->Log.AddLogDebug("40 - patch flags:            %.8X\n", PatchFlags);
    Service->Log.AddLogDebug("Device node:-----------------------------\n");
    Service->Log.AddLogDebug("44 - type:                   %u\n", DnType);
    Service->Log.AddLogDebug("48 - task:                   %u\n", DnTask);
    Service->Log.AddLogDebug("48 - task:                   %u\n", DnTask);
    Service->Log.AddLogDebug("52 - lock:                   %u\n", DnLock);
    Service->Log.AddLogDebug("56 - handler:                %u\n", DnHandler);
    Service->Log.AddLogDebug("60 - stack size:             %u\n", DnStackSize);
    Service->Log.AddLogDebug("64 - priority:               %u\n", DnPriority);
    Service->Log.AddLogDebug("68 - startup:                %u\n", DnStartup);
    Service->Log.AddLogDebug("72 - seg list block:         %u\n", DnSegListBlock);
    Service->Log.AddLogDebug("76 - global vec:             %d\n\n", DnGlobalVec);
  }

  RDBFileSystemHeader::RDBFileSystemHeader() :
    SizeInLongs(0),
    CheckSum(0),
    HostID(0),
    Next(0),
    Flags(0),
    DOSType(0),
    Version(0),
    PatchFlags(0),
    DnType(0),
    DnTask(0),
    DnLock(0),
    DnHandler(0),
    DnStackSize(0),
    DnPriority(0),
    DnStartup(0),
    DnSegListBlock(0),
    DnGlobalVec(0),
    HasValidCheckSum(false),
    HasFileSystemDataErrors(false)
  {
    memset(Reserved2, 0, sizeof Reserved2);
  }
}
