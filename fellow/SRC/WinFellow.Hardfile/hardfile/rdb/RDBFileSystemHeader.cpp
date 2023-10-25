#include "hardfile/rdb/RDBFileSystemHeader.h"
#include "hardfile/rdb/CheckSumCalculator.h"
#include "VirtualHost/Core.h"

namespace fellow::hardfile::rdb
{
  void RDBFileSystemHeader::ReadFromFile(RDBFileReader &reader, uint32_t blockChainStart, uint32_t blockSize)
  {
    uint32_t index = blockSize * blockChainStart;

    SizeInLongs = reader.ReadUint32(index + 4);
    CheckSum = reader.ReadInt32(index + 8);
    HostID = reader.ReadUint32(index + 12);
    Next = reader.ReadUint32(index + 16);
    Flags = reader.ReadUint32(index + 20);
    DOSType = reader.ReadUint32(index + 32);
    Version = reader.ReadUint32(index + 36);
    PatchFlags = reader.ReadUint32(index + 40);

    // Device node
    DnType = reader.ReadUint32(index + 44);
    DnTask = reader.ReadUint32(index + 48);
    DnLock = reader.ReadUint32(index + 52);
    DnHandler = reader.ReadUint32(index + 56);
    DnStackSize = reader.ReadUint32(index + 60);
    DnPriority = reader.ReadUint32(index + 64);
    DnStartup = reader.ReadUint32(index + 68);
    DnSegListBlock = reader.ReadUint32(index + 72);
    DnGlobalVec = reader.ReadUint32(index + 76);

    // Reserved for additional patchflags
    for (int i = 0; i < 23; i++)
    {
      Reserved2[i] = reader.ReadUint32(index + i + 80);
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
    _core.Log->AddLogDebug("Filesystem header block\n");
    _core.Log->AddLogDebug("-----------------------------------------\n");
    _core.Log->AddLogDebug("0  - id:                     FSHD\n");
    _core.Log->AddLogDebug("4  - size in longs:          %u\n", SizeInLongs);
    _core.Log->AddLogDebug("8  - checksum:               %d (%s)\n", CheckSum, HasValidCheckSum ? "Valid" : "Invalid");
    _core.Log->AddLogDebug("12 - host id:                %u\n", HostID);
    _core.Log->AddLogDebug("16 - next:                   %d\n", Next);
    _core.Log->AddLogDebug("20 - flags:                  %X\n", Flags);
    _core.Log->AddLogDebug("32 - dos type:               %.8X\n", DOSType);
    _core.Log->AddLogDebug("36 - version:                %.8X ie %d.%d\n", Version, (Version & 0xffff0000) >> 16, Version & 0xffff);
    _core.Log->AddLogDebug("40 - patch flags:            %.8X\n", PatchFlags);
    _core.Log->AddLogDebug("Device node:-----------------------------\n");
    _core.Log->AddLogDebug("44 - type:                   %u\n", DnType);
    _core.Log->AddLogDebug("48 - task:                   %u\n", DnTask);
    _core.Log->AddLogDebug("48 - task:                   %u\n", DnTask);
    _core.Log->AddLogDebug("52 - lock:                   %u\n", DnLock);
    _core.Log->AddLogDebug("56 - handler:                %u\n", DnHandler);
    _core.Log->AddLogDebug("60 - stack size:             %u\n", DnStackSize);
    _core.Log->AddLogDebug("64 - priority:               %u\n", DnPriority);
    _core.Log->AddLogDebug("68 - startup:                %u\n", DnStartup);
    _core.Log->AddLogDebug("72 - seg list block:         %u\n", DnSegListBlock);
    _core.Log->AddLogDebug("76 - global vec:             %d\n\n", DnGlobalVec);
  }

  RDBFileSystemHeader::RDBFileSystemHeader()
    : SizeInLongs(0),
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
