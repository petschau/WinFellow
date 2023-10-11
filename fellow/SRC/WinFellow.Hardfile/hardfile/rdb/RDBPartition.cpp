#include "hardfile/rdb/RDBPartition.h"
#include "hardfile/rdb/CheckSumCalculator.h"
#include "VirtualHost/Core.h"

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
    _core.Log->AddLogDebug("RDB Partition\n");
    _core.Log->AddLogDebug("----------------------------------------------------\n");
    _core.Log->AddLogDebug("0   - id:                       %s (Should be PART)\n", ID.c_str());
    _core.Log->AddLogDebug("4   - size in longs:            %u (Should be 64)\n", SizeInLongs);
    _core.Log->AddLogDebug("8   - checksum:                 %.8X (%s)\n", CheckSum, HasValidCheckSum ? "Valid" : "Invalid");
    _core.Log->AddLogDebug("12  - host id:                  %u\n", HostID);
    _core.Log->AddLogDebug("16  - next block:               %d\n", Next);
    _core.Log->AddLogDebug("20  - flags:                    %X (%s, %s)\n", Flags, IsBootable() ? "Bootable" : "Not bootable", IsAutomountable() ? "Automount" : "No automount");
    _core.Log->AddLogDebug("32  - DevFlags:                 %X\n", DevFlags);
    _core.Log->AddLogDebug("36  - DriveNameLength:          %d\n", DriveNameLength);
    _core.Log->AddLogDebug("37  - DriveName:                %s\n", DriveName.c_str());
    _core.Log->AddLogDebug("Partition DOS Environment vector:-------------------\n");
    _core.Log->AddLogDebug("128 - size of vector (in longs):%u (=%d bytes)\n", SizeOfVector, SizeOfVector * 4);
    _core.Log->AddLogDebug("132 - SizeBlock (in longs):     %u (=%d bytes)\n", SizeBlock, SizeBlock * 4);
    _core.Log->AddLogDebug("136 - SecOrg:                   %u (Should be 0)\n", SecOrg);
    _core.Log->AddLogDebug("140 - Surfaces:                 %u\n", Surfaces);
    _core.Log->AddLogDebug("144 - Sectors per block:        %u\n", SectorsPerBlock);
    _core.Log->AddLogDebug("148 - Blocks per track:         %u\n", BlocksPerTrack);
    _core.Log->AddLogDebug("152 - Reserved (blocks):        %u\n", Reserved);
    _core.Log->AddLogDebug("156 - Pre Alloc:                %u\n", PreAlloc);
    _core.Log->AddLogDebug("160 - Interleave:               %u\n", Interleave);
    _core.Log->AddLogDebug("164 - low cylinder:             %u\n", LowCylinder);
    _core.Log->AddLogDebug("168 - high cylinder:            %u\n", HighCylinder);
    _core.Log->AddLogDebug("172 - num buffer:               %u\n", NumBuffer);
    _core.Log->AddLogDebug("176 - BufMemType:               %u\n", BufMemType);
    _core.Log->AddLogDebug("180 - MaxTransfer:              %u\n", MaxTransfer);
    _core.Log->AddLogDebug("184 - Mask:                     %X\n", Mask);
    _core.Log->AddLogDebug("188 - BootPri:                  %u\n", BootPri);
    _core.Log->AddLogDebug("192 - DosType:                  %u\n", DOSType);
    _core.Log->AddLogDebug("196 - Baud:                     %u\n", Baud);
    _core.Log->AddLogDebug("200 - Control:                  %u\n", Control);
    _core.Log->AddLogDebug("204 - Bootblocks:               %u\n", Bootblocks);
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
