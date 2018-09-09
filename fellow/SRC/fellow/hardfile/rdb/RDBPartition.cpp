#include "fellow/hardfile/rdb/RDBPartition.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

namespace fellow::hardfile::rdb
{
  void RDBPartition::ReadFromFile(RDBFileReader& reader, ULO blockChainStart, ULO blockSize)
  {
    ULO index = blockSize * blockChainStart;

    ID = reader.ReadString(index, 4);
    SizeInLongs = reader.ReadULO(index + 4);
    CheckSum = reader.ReadLON(index + 8);
    HostID = reader.ReadULO(index + 12);
    Next = reader.ReadULO(index + 16);
    Flags = reader.ReadULO(index + 20);
    DevFlags = reader.ReadULO(index + 32);
    DriveNameLength = reader.ReadUBY(index + 36);
    DriveName = reader.ReadString(index + 37, DriveNameLength);

    // DOS Environment vector
    SizeOfVector = reader.ReadULO(index + 128);
    SizeBlock = reader.ReadULO(index + 132);
    SecOrg = reader.ReadULO(index + 136);
    Surfaces = reader.ReadULO(index + 140);
    SectorsPerBlock = reader.ReadULO(index + 144);
    BlocksPerTrack = reader.ReadULO(index + 148);
    Reserved = reader.ReadULO(index + 152);
    PreAlloc = reader.ReadULO(index + 156);
    Interleave = reader.ReadULO(index + 160);
    LowCylinder = reader.ReadULO(index + 164);
    HighCylinder = reader.ReadULO(index + 168);
    NumBuffer = reader.ReadULO(index + 172);
    BufMemType = reader.ReadULO(index + 176);
    MaxTransfer = reader.ReadULO(index + 180);
    Mask = reader.ReadULO(index + 184);
    BootPri = reader.ReadULO(index + 188);
    DOSType = reader.ReadULO(index + 192);
    Baud = reader.ReadULO(index + 196);
    Control = reader.ReadULO(index + 200);
    Bootblocks = reader.ReadULO(index + 204);
  }

  bool RDBPartition::IsAutomountable()
  {
    return (Flags & 2) == 0;
  }

  bool RDBPartition::IsBootable()
  {
    return (Flags & 1) == 1;
  }

  void RDBPartition::Log()
  {
    Service->Log.AddLog("RDB Partition\n");
    Service->Log.AddLog("----------------------------------------------------\n");
    Service->Log.AddLog("0   - id:                       %s (Should be PART)\n", ID.c_str());
    Service->Log.AddLog("4   - size in longs:            %u (Should be 64)\n", SizeInLongs);
    Service->Log.AddLog("8   - checksum:                 %.8X\n", CheckSum);
    Service->Log.AddLog("12  - host id:                  %u\n", HostID);
    Service->Log.AddLog("16  - next block:               %d\n", Next);
    Service->Log.AddLog("20  - flags:                    %X (%s, %s)\n", Flags, IsBootable() ? "Bootable" : "Not bootable", IsAutomountable() ? "Automount" : "No automount");
    Service->Log.AddLog("32  - DevFlags:                 %X\n", DevFlags);
    Service->Log.AddLog("36  - DriveNameLength:          %d\n", DriveNameLength);
    Service->Log.AddLog("37  - DriveName:                %s\n", DriveName.c_str());
    Service->Log.AddLog("Partition DOS Environment vector:-------------------\n");
    Service->Log.AddLog("128 - size of vector (in longs):%u (=%d bytes)\n", SizeOfVector, SizeOfVector * 4);
    Service->Log.AddLog("132 - SizeBlock (in longs):     %u (=%d bytes)\n", SizeBlock, SizeBlock * 4);
    Service->Log.AddLog("136 - SecOrg:                   %u (Should be 0)\n", SecOrg);
    Service->Log.AddLog("140 - Surfaces:                 %u\n", Surfaces);
    Service->Log.AddLog("144 - Sectors per block:        %u\n", SectorsPerBlock);
    Service->Log.AddLog("148 - Blocks per track:         %u\n", BlocksPerTrack);
    Service->Log.AddLog("152 - Reserved (blocks):        %u\n", Reserved);
    Service->Log.AddLog("156 - Pre Alloc:                %u\n", PreAlloc);
    Service->Log.AddLog("160 - Interleave:               %u\n", Interleave);
    Service->Log.AddLog("164 - low cylinder:             %u\n", LowCylinder);
    Service->Log.AddLog("168 - high cylinder:            %u\n", HighCylinder);
    Service->Log.AddLog("172 - num buffer:               %u\n", NumBuffer);
    Service->Log.AddLog("176 - BufMemType:               %u\n", BufMemType);
    Service->Log.AddLog("180 - MaxTransfer:              %u\n", MaxTransfer);
    Service->Log.AddLog("184 - Mask:                     %X\n", Mask);
    Service->Log.AddLog("188 - BootPri:                  %u\n", BootPri);
    Service->Log.AddLog("192 - DosType:                  %u\n", DOSType);
    Service->Log.AddLog("196 - Baud:                     %u\n", Baud);
    Service->Log.AddLog("200 - Control:                  %u\n", Control);
    Service->Log.AddLog("204 - Bootblocks:               %u\n", Bootblocks);
  }
}
