#include "hardfile/rdb/RDBPartition.h"
#include "hardfile/rdb/CheckSumCalculator.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

namespace fellow::hardfile::rdb
{
  void RDBPartition::ReadFromFile(RDBFileReader& reader, uint32_t blockChainStart, uint32_t blockSize)
  {
    uint32_t index = blockSize * blockChainStart;

    ID = reader.ReadString(index, 4);
    SizeInLongs = reader.ReadUint32(index + 4);
    CheckSum = reader.ReadInt32(index + 8);
    HostID = reader.ReadUint32(index + 12);
    Next = reader.ReadUint32(index + 16);
    Flags = reader.ReadUint32(index + 20);
    DevFlags = reader.ReadUint32(index + 32);
    DriveNameLength = reader.ReadUint8(index + 36);
    DriveName = reader.ReadString(index + 37, DriveNameLength);

    // DOS Environment vector
    SizeOfVector = reader.ReadUint32(index + 128);
    SizeBlock = reader.ReadUint32(index + 132);
    SecOrg = reader.ReadUint32(index + 136);
    Surfaces = reader.ReadUint32(index + 140);
    SectorsPerBlock = reader.ReadUint32(index + 144);
    BlocksPerTrack = reader.ReadUint32(index + 148);
    Reserved = reader.ReadUint32(index + 152);
    PreAlloc = reader.ReadUint32(index + 156);
    Interleave = reader.ReadUint32(index + 160);
    LowCylinder = reader.ReadUint32(index + 164);
    HighCylinder = reader.ReadUint32(index + 168);
    NumBuffer = reader.ReadUint32(index + 172);
    BufMemType = reader.ReadUint32(index + 176);
    MaxTransfer = reader.ReadUint32(index + 180);
    Mask = reader.ReadUint32(index + 184);
    BootPri = reader.ReadUint32(index + 188);
    DOSType = reader.ReadUint32(index + 192);
    Baud = reader.ReadUint32(index + 196);
    Control = reader.ReadUint32(index + 200);
    Bootblocks = reader.ReadUint32(index + 204);

    HasValidCheckSum = (SizeInLongs == 64) && CheckSumCalculator::HasValidCheckSum(reader, 64, index);
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
    Service->Log.AddLogDebug("RDB Partition\n");
    Service->Log.AddLogDebug("----------------------------------------------------\n");
    Service->Log.AddLogDebug("0   - id:                       %s (Should be PART)\n", ID.c_str());
    Service->Log.AddLogDebug("4   - size in longs:            %u (Should be 64)\n", SizeInLongs);
    Service->Log.AddLogDebug("8   - checksum:                 %.8X (%s)\n", CheckSum, HasValidCheckSum ? "Valid" : "Invalid");
    Service->Log.AddLogDebug("12  - host id:                  %u\n", HostID);
    Service->Log.AddLogDebug("16  - next block:               %d\n", Next);
    Service->Log.AddLogDebug("20  - flags:                    %X (%s, %s)\n", Flags, IsBootable() ? "Bootable" : "Not bootable", IsAutomountable() ? "Automount" : "No automount");
    Service->Log.AddLogDebug("32  - DevFlags:                 %X\n", DevFlags);
    Service->Log.AddLogDebug("36  - DriveNameLength:          %d\n", DriveNameLength);
    Service->Log.AddLogDebug("37  - DriveName:                %s\n", DriveName.c_str());
    Service->Log.AddLogDebug("Partition DOS Environment vector:-------------------\n");
    Service->Log.AddLogDebug("128 - size of vector (in longs):%u (=%d bytes)\n", SizeOfVector, SizeOfVector * 4);
    Service->Log.AddLogDebug("132 - SizeBlock (in longs):     %u (=%d bytes)\n", SizeBlock, SizeBlock * 4);
    Service->Log.AddLogDebug("136 - SecOrg:                   %u (Should be 0)\n", SecOrg);
    Service->Log.AddLogDebug("140 - Surfaces:                 %u\n", Surfaces);
    Service->Log.AddLogDebug("144 - Sectors per block:        %u\n", SectorsPerBlock);
    Service->Log.AddLogDebug("148 - Blocks per track:         %u\n", BlocksPerTrack);
    Service->Log.AddLogDebug("152 - Reserved (blocks):        %u\n", Reserved);
    Service->Log.AddLogDebug("156 - Pre Alloc:                %u\n", PreAlloc);
    Service->Log.AddLogDebug("160 - Interleave:               %u\n", Interleave);
    Service->Log.AddLogDebug("164 - low cylinder:             %u\n", LowCylinder);
    Service->Log.AddLogDebug("168 - high cylinder:            %u\n", HighCylinder);
    Service->Log.AddLogDebug("172 - num buffer:               %u\n", NumBuffer);
    Service->Log.AddLogDebug("176 - BufMemType:               %u\n", BufMemType);
    Service->Log.AddLogDebug("180 - MaxTransfer:              %u\n", MaxTransfer);
    Service->Log.AddLogDebug("184 - Mask:                     %X\n", Mask);
    Service->Log.AddLogDebug("188 - BootPri:                  %u\n", BootPri);
    Service->Log.AddLogDebug("192 - DosType:                  %u\n", DOSType);
    Service->Log.AddLogDebug("196 - Baud:                     %u\n", Baud);
    Service->Log.AddLogDebug("200 - Control:                  %u\n", Control);
    Service->Log.AddLogDebug("204 - Bootblocks:               %u\n", Bootblocks);
  }

  RDBPartition::RDBPartition() :
    SizeInLongs(0),
    CheckSum(0),
    HostID(0),
    Next(0),
    Flags(0),
    BadBlockList(0),
    DevFlags(0),
    DriveNameLength(0),
    SizeOfVector(0),
    SizeBlock(0),
    SecOrg(0),
    Surfaces(0),
    SectorsPerBlock(0),
    BlocksPerTrack(0),
    Reserved(0),
    PreAlloc(0),
    Interleave(0),
    LowCylinder(0),
    HighCylinder(0),
    NumBuffer(0),
    BufMemType(0),
    MaxTransfer(0),
    Mask(0),
    BootPri(0),
    DOSType(0),
    Baud(0),
    Control(0),
    Bootblocks(0),
    HasValidCheckSum(false)
  {
  }
}
