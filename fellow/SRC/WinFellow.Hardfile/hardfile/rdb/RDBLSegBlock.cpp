#include "hardfile/rdb/RDBLSegBlock.h"
#include "hardfile/rdb/CheckSumCalculator.h"
#include "VirtualHost/Core.h"

namespace fellow::hardfile::rdb
{
  RDBLSegBlock::RDBLSegBlock() :
    Blocknumber(-1),
    SizeInLongs(0),
    CheckSum(0),
    HostID(0),
    Next(-1),
    HasValidCheckSum(false)
  {
  }

  int32_t RDBLSegBlock::GetDataSize() const
  {
    return 4 * (SizeInLongs - 5);
  }

  const uint8_t* RDBLSegBlock::GetData() const
  {
    return Data.get();
  }

  void RDBLSegBlock::ReadFromFile(RDBFileReader& reader, uint32_t index)
  {
    Blocknumber = index / 512;
    ID = reader.ReadString(index, 4);
    SizeInLongs = reader.ReadInt32(index + 4);
    CheckSum = reader.ReadInt32(index + 8);
    HostID = reader.ReadInt32(index + 12);
    Next = reader.ReadInt32(index + 16);

    HasValidCheckSum = (SizeInLongs >= 5 && SizeInLongs <= 128) && CheckSumCalculator::HasValidCheckSum(reader, SizeInLongs, index);

    if (!HasValidCheckSum)
    {
      return;
    }

    Data.reset(reader.ReadData(index + 20, GetDataSize()));
  }

  void RDBLSegBlock::Log()
  {
    _core.Log->AddLogDebug("LSegBlock (Blocknumber %d)\n", Blocknumber);
    _core.Log->AddLogDebug("-----------------------------------------\n");
    _core.Log->AddLogDebug("0   - id:                     %.4s\n", ID.c_str());
    _core.Log->AddLogDebug("4   - size in longs:          %d\n", SizeInLongs);
    _core.Log->AddLogDebug("8   - checksum:               %.8X (%s)\n", CheckSum, HasValidCheckSum ? "Valid" : "Invalid");
    _core.Log->AddLogDebug("12  - host id:                %d\n", HostID);
    _core.Log->AddLogDebug("16  - next:                   %d\n\n", Next);
  }
}
