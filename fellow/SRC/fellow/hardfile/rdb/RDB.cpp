#include "fellow/hardfile/rdb/RDB.h"
#include "fellow/api/Services.h"

#ifdef _DEBUG
  #define _CRTDBG_MAP_ALLOC
  #include <cstdlib>
  #include <crtdbg.h>
  #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
  #define DBG_NEW new
#endif

using namespace std;
using namespace fellow::api;

namespace fellow::hardfile::rdb
{
  void RDB::ReadFromFile(RDBFileReader& reader, bool geometryOnly)
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
    while (nextPartition != 0xffffffff)
    {
      RDBPartition* partition = DBG_NEW RDBPartition();
      partition->ReadFromFile(reader, nextPartition, BlockSize);
      partition->Log();
      Partitions.push_back(unique_ptr<RDBPartition>(partition));
      nextPartition = partition->Next;
    }

    if (!geometryOnly)
    {
      ULO nextFilesystemHeader = FilesystemHeaderList;
      while (nextFilesystemHeader != 0xffffffff)
      {
        RDBFileSystemHeader* fileSystemHeader = DBG_NEW RDBFileSystemHeader();
        fileSystemHeader->ReadFromFile(reader, nextFilesystemHeader, BlockSize);
        fileSystemHeader->Log();
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
    Service->Log.AddLogDebug("8   - checksum:               %.8X\n", CheckSum);
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
}
