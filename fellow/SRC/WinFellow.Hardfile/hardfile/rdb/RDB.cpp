#include "fellow/api/defs.h"
#include "hardfile/rdb/RDB.h"
#include "hardfile/rdb/CheckSumCalculator.h"
#include "VirtualHost/Core.h"

using namespace std;

namespace fellow::hardfile::rdb
{
  void RDB::ReadFromFile(RDBFileReader& reader, unsigned int index, bool geometryOnly)
  {
    ID = reader.ReadString(index + 0, 4);
    SizeInLongs = reader.ReadUint32(index + 4);
    CheckSum = reader.ReadInt32(index + 8);
    HostID = reader.ReadUint32(index + 12);
    BlockSize = reader.ReadUint32(index + 16);
    Flags = reader.ReadUint32(index + 20);
    BadBlockList = reader.ReadUint32(index + 24);
    PartitionList = reader.ReadUint32(index + 28);
    FilesystemHeaderList = reader.ReadUint32(index + 32);
    DriveInitCode = reader.ReadUint32(index + 36);

    // Physical drive characteristics
    Cylinders = reader.ReadUint32(index + 64);
    SectorsPerTrack = reader.ReadUint32(index + 68);
    Heads = reader.ReadUint32(index + 72);
    Interleave = reader.ReadUint32(index + 76);
    ParkingZone = reader.ReadUint32(index + 80);
    WritePreComp = reader.ReadUint32(index + 96);
    ReducedWrite = reader.ReadUint32(index + 100);
    StepRate = reader.ReadUint32(index + 104);

    // Logical drive characteristics
    RDBBlockLow = reader.ReadUint32(index + 128);
    RDBBlockHigh = reader.ReadUint32(index + 132);
    LowCylinder = reader.ReadUint32(index + 136);
    HighCylinder = reader.ReadUint32(index + 140);
    CylinderBlocks = reader.ReadUint32(index + 144);
    AutoParkSeconds = reader.ReadUint32(index + 148);
    HighRDSKBlock = reader.ReadUint32(index + 152);

    // Drive identification
    DiskVendor = reader.ReadString(index + 160, 8);
    DiskProduct = reader.ReadString(index + 168, 16);
    DiskRevision = reader.ReadString(index + 184, 4);
    ControllerVendor = reader.ReadString(index + 188, 8);
    ControllerProduct = reader.ReadString(index + 196, 16);
    ControllerRevision = reader.ReadString(index + 212, 4);

    HasValidCheckSum = CheckSumCalculator::HasValidCheckSum(reader, 128, index);

    if (!HasValidCheckSum)
    {
      _core.Log->AddLog("Hardfile RDB header checksum error.\n");
      return;
    }

    uint32_t nextPartition = PartitionList;
    while (nextPartition != 0xffffffff)
    {
      RDBPartition* partition = new RDBPartition();
      partition->ReadFromFile(reader, nextPartition, BlockSize);
      partition->Log();

      if (!partition->HasValidCheckSum)
      {
        _core.Log->AddLog("Hardfile RDB partition checksum error.\n");
        Partitions.clear();
        HasPartitionErrors = true;
        return;
      }

      if (nextPartition == partition->Next)
      {
        _core.Log->AddLog("Hardfile RDB partition list error, next partition points to the previous.\n");
        Partitions.clear();
        HasPartitionErrors = true;
        return;
      }

      Partitions.push_back(unique_ptr<RDBPartition>(partition));
      nextPartition = partition->Next;
    }

    if (!geometryOnly)
    {
      uint32_t nextFilesystemHeader = FilesystemHeaderList;
      while (nextFilesystemHeader != 0xffffffff)
      {
        RDBFileSystemHeader* fileSystemHeader = new RDBFileSystemHeader();
        fileSystemHeader->ReadFromFile(reader, nextFilesystemHeader, BlockSize);
        fileSystemHeader->Log();

        if (!fileSystemHeader->HasValidCheckSum)
        {
          _core.Log->AddLog("Hardfile RDB filesystem header checksum error.\n");
          FileSystemHeaders.clear();
          HasFileSystemHandlerErrors = true;
          return;
        }

        if (fileSystemHeader->HasFileSystemDataErrors)
        {
          _core.Log->AddLog("Hardfile RDB filesystem data checksum error.\n");
          FileSystemHeaders.clear();
          HasFileSystemHandlerErrors = true;
          return;
        }

        if (nextFilesystemHeader == fileSystemHeader->Next)
        {
          _core.Log->AddLog("Hardfile RDB filesystem list error, next filesystem points to the previous.\n");
          FileSystemHeaders.clear();
          HasFileSystemHandlerErrors = true;
          return;
        }

        FileSystemHeaders.push_back(unique_ptr<RDBFileSystemHeader>(fileSystemHeader));
        nextFilesystemHeader = fileSystemHeader->Next;
      }
    }
  }

  void RDB::Log()
  {
    _core.Log->AddLogDebug("RDB Hardfile\n");
    _core.Log->AddLogDebug("-----------------------------------------\n");
    _core.Log->AddLogDebug("0   - id:                     %s\n", ID.c_str());
    _core.Log->AddLogDebug("4   - size in longs:          %u\n", SizeInLongs);
    _core.Log->AddLogDebug("8   - checksum:               %.8X (%s)\n", CheckSum, HasValidCheckSum ? "Valid" : "Invalid");
    _core.Log->AddLogDebug("12  - host id:                %u\n", HostID);
    _core.Log->AddLogDebug("16  - block size:             %u\n", BlockSize);
    _core.Log->AddLogDebug("20  - flags:                  %X\n", Flags);
    _core.Log->AddLogDebug("24  - bad block list:         %d\n", BadBlockList);
    _core.Log->AddLogDebug("28  - partition list:         %d\n", PartitionList);
    _core.Log->AddLogDebug("32  - filesystem header list: %d\n", FilesystemHeaderList);
    _core.Log->AddLogDebug("36  - drive init code:        %X\n", DriveInitCode);
    _core.Log->AddLogDebug("Physical drive characteristics:---------\n");
    _core.Log->AddLogDebug("64  - cylinders:              %u\n", Cylinders);
    _core.Log->AddLogDebug("68  - sectors per track:      %u\n", SectorsPerTrack);
    _core.Log->AddLogDebug("72  - heads:                  %u\n", Heads);
    _core.Log->AddLogDebug("76  - interleave:             %u\n", Interleave);
    _core.Log->AddLogDebug("80  - parking zone:           %u\n", ParkingZone);
    _core.Log->AddLogDebug("96  - write pre-compensation: %u\n", WritePreComp);
    _core.Log->AddLogDebug("100 - reduced write:          %u\n", ReducedWrite);
    _core.Log->AddLogDebug("104 - step rate:              %u\n", StepRate);
    _core.Log->AddLogDebug("Logical drive characteristics:----------\n");
    _core.Log->AddLogDebug("128 - RDB block low:          %u\n", RDBBlockLow);
    _core.Log->AddLogDebug("132 - RDB block high:         %u\n", RDBBlockHigh);
    _core.Log->AddLogDebug("136 - low cylinder:           %u\n", LowCylinder);
    _core.Log->AddLogDebug("140 - high cylinder:          %u\n", HighCylinder);
    _core.Log->AddLogDebug("144 - cylinder blocks:        %u\n", CylinderBlocks);
    _core.Log->AddLogDebug("148 - auto park seconds:      %u\n", AutoParkSeconds);
    _core.Log->AddLogDebug("152 - high RDSK block:        %u\n", HighRDSKBlock);
    _core.Log->AddLogDebug("Drive identification:-------------------\n");
    _core.Log->AddLogDebug("160 - disk vendor:            %.8s\n", DiskVendor.c_str());
    _core.Log->AddLogDebug("168 - disk product:           %.16s\n", DiskProduct.c_str());
    _core.Log->AddLogDebug("184 - disk revision:          %.4s\n", DiskRevision.c_str());
    _core.Log->AddLogDebug("188 - controller vendor:      %.8s\n", ControllerVendor.c_str());
    _core.Log->AddLogDebug("196 - controller product:     %.16s\n", ControllerProduct.c_str());
    _core.Log->AddLogDebug("212 - controller revision:    %.4s\n", ControllerRevision.c_str());
    _core.Log->AddLogDebug("-----------------------------------------\n\n");
  }

  RDB::RDB() :
    SizeInLongs(0),
    CheckSum(0),
    HostID(0),
    BlockSize(0),
    Flags(0),
    BadBlockList(0),
    PartitionList(0),
    FilesystemHeaderList(0),
    DriveInitCode(0),
    Cylinders(0),
    SectorsPerTrack(0),
    Heads(0),
    Interleave(0),
    ParkingZone(0),
    WritePreComp(0),
    ReducedWrite(0),
    StepRate(0),
    RDBBlockLow(0),
    RDBBlockHigh(0),
    LowCylinder(0),
    HighCylinder(0),
    CylinderBlocks(0),
    AutoParkSeconds(0),
    HighRDSKBlock(0),
    HasValidCheckSum(false),
    HasPartitionErrors(false),
    HasFileSystemHandlerErrors(false)
  {
  }
}
