#include "fellow/hardfile/rdb/RDB.h"
#include "fellow/api/Services.h"

using namespace std;
using namespace fellow::api;

namespace fellow::hardfile::rdb
{
  void RDB::ReadFromFile(RDBFileReader& reader)
  {
    ID = reader.ReadString(0, 4);
    SizeInLongs = reader.ReadULO(4);
    CheckSum = reader.ReadLON(8);
    HostID = reader.ReadULO(12);
    BlockSize = reader.ReadULO(16);
    Flags = reader.ReadULO(20);
    BadBlockList = reader.ReadULO(24);
    PartitionList = reader.ReadULO(28);
    FilesystemHeaderList = reader.ReadULO(32);
    DriveInitCode = reader.ReadULO(36);

    // Physical drive characteristics
    Cylinders = reader.ReadULO(64);
    SectorsPerTrack = reader.ReadULO(68);
    Heads = reader.ReadULO(72);
    Interleave = reader.ReadULO(76);
    ParkingZone = reader.ReadULO(80);
    WritePreComp = reader.ReadULO(96);
    ReducedWrite = reader.ReadULO(100);
    StepRate = reader.ReadULO(104);

    // Logical drive characteristics
    RDBBlockLow = reader.ReadULO(128);
    RDBBlockHigh = reader.ReadULO(132);
    LowCylinder = reader.ReadULO(136);
    HighCylinder = reader.ReadULO(140);
    CylinderBlocks = reader.ReadULO(144);
    AutoParkSeconds = reader.ReadULO(148);
    HighRDSKBlock = reader.ReadULO(152);

    DiskVendor = reader.ReadString(160, 8);
    DiskProduct = reader.ReadString(168, 16);
    DiskRevision = reader.ReadString(184, 4);
    ControllerVendor = reader.ReadString(188, 8);
    ControllerProduct = reader.ReadString(196, 16);
    ControllerRevision = reader.ReadString(212, 4);

    ULO nextPartition = PartitionList;
    while (nextPartition != -1)
    {
      RDBPartition* partition = new RDBPartition();
      partition->ReadFromFile(reader, nextPartition, BlockSize);
      partition->Log();
      Partitions.push_back(unique_ptr<RDBPartition>(partition));
      nextPartition = partition->Next;
    }

    ULO nextFilesystemHeader = FilesystemHeaderList;
    while (nextFilesystemHeader != -1)
    {
      RDBFileSystemHeader* fileSystemHeader = new RDBFileSystemHeader();
      fileSystemHeader->ReadFromFile(reader, nextFilesystemHeader, BlockSize);
      fileSystemHeader->Log();
      FileSystemHeaders.push_back(unique_ptr<RDBFileSystemHeader>(fileSystemHeader));
      nextFilesystemHeader = fileSystemHeader->Next;
    }
  }

  void RDB::Log()
  {
    Service->Log.AddLog("RDB Hardfile\n");
    Service->Log.AddLog("-----------------------------------------\n");
    Service->Log.AddLog("0   - id:                     %s\n", ID.c_str());
    Service->Log.AddLog("4   - size in longs:          %u\n", SizeInLongs);
    Service->Log.AddLog("8   - checksum:               %.8X\n", CheckSum);
    Service->Log.AddLog("12  - host id:                %u\n", HostID);
    Service->Log.AddLog("16  - block size:             %u\n", BlockSize);
    Service->Log.AddLog("20  - flags:                  %X\n", Flags);
    Service->Log.AddLog("24  - bad block list:         %d\n", BadBlockList);
    Service->Log.AddLog("28  - partition list:         %d\n", PartitionList);
    Service->Log.AddLog("32  - filesystem header list: %d\n", FilesystemHeaderList);
    Service->Log.AddLog("36  - drive init code:        %X\n", DriveInitCode);
    Service->Log.AddLog("Physical drive characteristics:---------\n");
    Service->Log.AddLog("64  - cylinders:              %u\n", Cylinders);
    Service->Log.AddLog("68  - sectors per track:      %u\n", SectorsPerTrack);
    Service->Log.AddLog("72  - heads:                  %u\n", Heads);
    Service->Log.AddLog("76  - interleave:             %u\n", Interleave);
    Service->Log.AddLog("80  - parking zone:           %u\n", ParkingZone);
    Service->Log.AddLog("96  - write pre-compensation: %u\n", WritePreComp);
    Service->Log.AddLog("100 - reduced write:          %u\n", ReducedWrite);
    Service->Log.AddLog("104 - step rate:              %u\n", StepRate);
    Service->Log.AddLog("Logical drive characteristics:----------\n");
    Service->Log.AddLog("128 - RDB block low:          %u\n", RDBBlockLow);
    Service->Log.AddLog("132 - RDB block high:         %u\n", RDBBlockHigh);
    Service->Log.AddLog("136 - low cylinder:           %u\n", LowCylinder);
    Service->Log.AddLog("140 - high cylinder:          %u\n", HighCylinder);
    Service->Log.AddLog("144 - cylinder blocks:        %u\n", CylinderBlocks);
    Service->Log.AddLog("148 - auto park seconds:      %u\n", AutoParkSeconds);
    Service->Log.AddLog("152 - high RDSK block:        %u\n", HighRDSKBlock);
    Service->Log.AddLog("Drive identification:-------------------\n");
    Service->Log.AddLog("160 - disk vendor:            %.8s\n", DiskVendor.c_str());
    Service->Log.AddLog("168 - disk product:           %.16s\n", DiskProduct.c_str());
    Service->Log.AddLog("184 - disk revision:          %.4s\n", DiskRevision.c_str());
    Service->Log.AddLog("188 - controller vendor:      %.8s\n", ControllerVendor.c_str());
    Service->Log.AddLog("196 - controller product:     %.16s\n", ControllerProduct.c_str());
    Service->Log.AddLog("212 - controller revision:    %.4s\n", ControllerRevision.c_str());
    Service->Log.AddLog("-----------------------------------------\n\n");
  }
}
