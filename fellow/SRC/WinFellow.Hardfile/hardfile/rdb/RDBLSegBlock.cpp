#include "hardfile/rdb/RDBLSegBlock.h"
#include "hardfile/rdb/CheckSumCalculator.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

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

  LON RDBLSegBlock::GetDataSize() const
  {
    return 4 * (SizeInLongs - 5);
  }

  const UBY* RDBLSegBlock::GetData() const
  {
    return Data.get();
  }

  void RDBLSegBlock::ReadFromFile(RDBFileReader& reader, ULO index)
  {
    Blocknumber = index / 512;
    ID = reader.ReadString(index, 4);
    SizeInLongs = reader.ReadLON(index + 4);
    CheckSum = reader.ReadLON(index + 8);
    HostID = reader.ReadLON(index + 12);
    Next = reader.ReadLON(index + 16);

    HasValidCheckSum = (SizeInLongs >= 5 && SizeInLongs <= 128) && CheckSumCalculator::HasValidCheckSum(reader, SizeInLongs, index);

    if (!HasValidCheckSum)
    {
      return;
    }

    Data.reset(reader.ReadData(index + 20, GetDataSize()));
  }

  void RDBLSegBlock::Log()
  {
    Service->Log.AddLogDebug("LSegBlock (Blocknumber %d)\n", Blocknumber);
    Service->Log.AddLogDebug("-----------------------------------------\n");
    Service->Log.AddLogDebug("0   - id:                     %.4s\n", ID.c_str());
    Service->Log.AddLogDebug("4   - size in longs:          %d\n", SizeInLongs);
    Service->Log.AddLogDebug("8   - checksum:               %.8X (%s)\n", CheckSum, HasValidCheckSum ? "Valid" : "Invalid");
    Service->Log.AddLogDebug("12  - host id:                %d\n", HostID);
    Service->Log.AddLogDebug("16  - next:                   %d\n\n", Next);
  }
}
