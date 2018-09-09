#include "fellow/hardfile/rdb/RDBFileSystemHeader.h"
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

    FileSystemHandler.ReadFromFile(reader, DnSegListBlock, blockSize);
  }

  void RDBFileSystemHeader::Log()
  {
    Service->Log.AddLog("Filesystem header block\n");
    Service->Log.AddLog("-----------------------------------------\n");
    Service->Log.AddLog("0  - id:                     FSHD\n");
    Service->Log.AddLog("4  - size in longs:          %u\n", SizeInLongs);
    Service->Log.AddLog("8  - checksum:               %d\n", CheckSum);
    Service->Log.AddLog("12 - host id:                %u\n", HostID);
    Service->Log.AddLog("16 - next:                   %d\n", Next);
    Service->Log.AddLog("20 - flags:                  %X\n", Flags);
    Service->Log.AddLog("32 - dos type:               %.8X\n", DOSType);
    Service->Log.AddLog("36 - version:                %.8X ie %d.%d\n", Version, (Version & 0xffff0000) >> 16, Version & 0xffff);
    Service->Log.AddLog("40 - patch flags:            %.8X\n", PatchFlags);
    Service->Log.AddLog("Device node:-----------------------------\n");
    Service->Log.AddLog("44 - type:                   %u\n", DnType);
    Service->Log.AddLog("48 - task:                   %u\n", DnTask);
    Service->Log.AddLog("48 - task:                   %u\n", DnTask);
    Service->Log.AddLog("52 - lock:                   %u\n", DnLock);
    Service->Log.AddLog("56 - handler:                %u\n", DnHandler);
    Service->Log.AddLog("60 - stack size:             %u\n", DnStackSize);
    Service->Log.AddLog("64 - priority:               %u\n", DnPriority);
    Service->Log.AddLog("68 - startup:                %u\n", DnStartup);
    Service->Log.AddLog("72 - seg list block:         %u\n", DnSegListBlock);
    Service->Log.AddLog("76 - global vec:             %d\n\n", DnGlobalVec);
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
    DnGlobalVec(0)
  {
  }
}
