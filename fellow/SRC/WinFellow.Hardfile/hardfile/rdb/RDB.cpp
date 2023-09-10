#include "fellow/api/defs.h"
#include "hardfile/rdb/RDB.h"
#include "hardfile/rdb/CheckSumCalculator.h"
#include "fellow/api/Services.h"

using namespace std;
using namespace fellow::api;

namespace fellow::hardfile::rdb
{
  void RDB::ReadFromFile(RDBFileReader& reader, unsigned int index, bool geometryOnly)
  {
    ID = reader.ReadString(index + 0, 4);
    SizeInLongs = reader.ReadULO(index + 4);
    CheckSum = reader.ReadLON(index + 8);
    HostID = reader.ReadULO(index + 12);
    BlockSize = reader.ReadULO(index + 16);
    Flags = reader.ReadULO(index + 20);
    BadBlockList = reader.ReadULO(index + 24);
    PartitionList = reader.ReadULO(index + 28);
    FilesystemHeaderList = reader.ReadULO(index + 32);
    DriveInitCode = reader.ReadULO(index + 36);

    // Physical drive characteristics
    Cylinders = reader.ReadULO(index + 64);
    SectorsPerTrack = reader.ReadULO(index + 68);
    Heads = reader.ReadULO(index + 72);
    Interleave = reader.ReadULO(index + 76);
    ParkingZone = reader.ReadULO(index + 80);
    WritePreComp = reader.ReadULO(index + 96);
    ReducedWrite = reader.ReadULO(index + 100);
    StepRate = reader.ReadULO(index + 104);

    // Logical drive characteristics
    RDBBlockLow = reader.ReadULO(index + 128);
    RDBBlockHigh = reader.ReadULO(index + 132);
    LowCylinder = reader.ReadULO(index + 136);
    HighCylinder = reader.ReadULO(index + 140);
    CylinderBlocks = reader.ReadULO(index + 144);
    AutoParkSeconds = reader.ReadULO(index + 148);
    HighRDSKBlock = reader.ReadULO(index + 152);

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
      Service->Log.AddLog("Hardfile RDB header checksum error.\n");
      return;
    }

    ULO nextPartition = PartitionList;
    while (nextPartition != 0xffffffff)
    {
      RDBPartition* partition = new RDBPartition();
      partition->ReadFromFile(reader, nextPartition, BlockSize);
      partition->Log();

      if (!partition->HasValidCheckSum)
      {
        Service->Log.AddLog("Hardfile RDB partition checksum error.\n");
        Partitions.clear();
        HasPartitionErrors = true;
        return;
      }

      if (nextPartition == partition->Next)
      {
        Service->Log.AddLog("Hardfile RDB partition list error, next partition points to the previous.\n");
        Partitions.clear();
        HasPartitionErrors = true;
        return;
      }

      Partitions.push_back(unique_ptr<RDBPartition>(partition));
      nextPartition = partition->Next;
    }

    if (!geometryOnly)
    {
      ULO nextFilesystemHeader = FilesystemHeaderList;
      while (nextFilesystemHeader != 0xffffffff)
      {
        RDBFileSystemHeader* fileSystemHeader = new RDBFileSystemHeader();
        fileSystemHeader->ReadFromFile(reader, nextFilesystemHeader, BlockSize);
        fileSystemHeader->Log();

        if (!fileSystemHeader->HasValidCheckSum)
        {
          Service->Log.AddLog("Hardfile RDB filesystem header checksum error.\n");
          FileSystemHeaders.clear();
          HasFileSystemHandlerErrors = true;
          return;
        }

        if (fileSystemHeader->HasFileSystemDataErrors)
        {
          Service->Log.AddLog("Hardfile RDB filesystem data checksum error.\n");
          FileSystemHeaders.clear();
          HasFileSystemHandlerErrors = true;
          return;
        }

        if (nextFilesystemHeader == fileSystemHeader->Next)
        {
          Service->Log.AddLog("Hardfile RDB filesystem list error, next filesystem points to the previous.\n");
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
    Service->Log.AddLogDebug("RDB Hardfile\n");
    Service->Log.AddLogDebug("-----------------------------------------\n");
    Service->Log.AddLogDebug("0   - id:                     %s\n", ID.c_str());
    Service->Log.AddLogDebug("4   - size in longs:          %u\n", SizeInLongs);
    Service->Log.AddLogDebug("8   - checksum:               %.8X (%s)\n", CheckSum, HasValidCheckSum ? "Valid" : "Invalid");
    Service->Log.AddLogDebug("12  - host id:                %u\n", HostID);
    Service->Log.AddLogDebug("16  - block size:             %u\n", BlockSize);
    Service->Log.AddLogDebug("20  - flags:                  %X\n", Flags);
    Service->Log.AddLogDebug("24  - bad block list:         %d\n", BadBlockList);
    Service->Log.AddLogDebug("28  - partition list:         %d\n", PartitionList);
    Service->Log.AddLogDebug("32  - filesystem header list: %d\n", FilesystemHeaderList);
    Service->Log.AddLogDebug("36  - drive init code:        %X\n", DriveInitCode);
    Service->Log.AddLogDebug("Physical drive characteristics:---------\n");
    Service->Log.AddLogDebug("64  - cylinders:              %u\n", Cylinders);
    Service->Log.AddLogDebug("68  - sectors per track:      %u\n", SectorsPerTrack);
    Service->Log.AddLogDebug("72  - heads:                  %u\n", Heads);
    Service->Log.AddLogDebug("76  - interleave:             %u\n", Interleave);
    Service->Log.AddLogDebug("80  - parking zone:           %u\n", ParkingZone);
    Service->Log.AddLogDebug("96  - write pre-compensation: %u\n", WritePreComp);
    Service->Log.AddLogDebug("100 - reduced write:          %u\n", ReducedWrite);
    Service->Log.AddLogDebug("104 - step rate:              %u\n", StepRate);
    Service->Log.AddLogDebug("Logical drive characteristics:----------\n");
    Service->Log.AddLogDebug("128 - RDB block low:          %u\n", RDBBlockLow);
    Service->Log.AddLogDebug("132 - RDB block high:         %u\n", RDBBlockHigh);
    Service->Log.AddLogDebug("136 - low cylinder:           %u\n", LowCylinder);
    Service->Log.AddLogDebug("140 - high cylinder:          %u\n", HighCylinder);
    Service->Log.AddLogDebug("144 - cylinder blocks:        %u\n", CylinderBlocks);
    Service->Log.AddLogDebug("148 - auto park seconds:      %u\n", AutoParkSeconds);
    Service->Log.AddLogDebug("152 - high RDSK block:        %u\n", HighRDSKBlock);
    Service->Log.AddLogDebug("Drive identification:-------------------\n");
    Service->Log.AddLogDebug("160 - disk vendor:            %.8s\n", DiskVendor.c_str());
    Service->Log.AddLogDebug("168 - disk product:           %.16s\n", DiskProduct.c_str());
    Service->Log.AddLogDebug("184 - disk revision:          %.4s\n", DiskRevision.c_str());
    Service->Log.AddLogDebug("188 - controller vendor:      %.8s\n", ControllerVendor.c_str());
    Service->Log.AddLogDebug("196 - controller product:     %.16s\n", ControllerProduct.c_str());
    Service->Log.AddLogDebug("212 - controller revision:    %.4s\n", ControllerRevision.c_str());
    Service->Log.AddLogDebug("-----------------------------------------\n\n");
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
