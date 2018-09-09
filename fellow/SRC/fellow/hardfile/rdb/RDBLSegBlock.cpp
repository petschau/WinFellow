#include "fellow/hardfile/rdb/RDBLSegBlock.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

namespace fellow::hardfile::rdb
{
  RDBLSegBlock::RDBLSegBlock() :
    SizeInLongs(0),
    CheckSum(0),
    HostID(0),
    Next(-1)
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
    ID = reader.ReadString(index, 4);
    SizeInLongs = reader.ReadLON(index + 4);
    CheckSum = reader.ReadLON(index + 8);
    HostID = reader.ReadLON(index + 12);
    Next = reader.ReadLON(index + 16);
    Data.reset(reader.ReadData(index + 20, GetDataSize()));
  }

  void RDBLSegBlock::Log()
  {
    Service->Log.AddLog("LSegBlock\n");
    Service->Log.AddLog("-----------------------------------------\n");
    Service->Log.AddLog("0   - id:                     %.4s\n", ID.c_str());
    Service->Log.AddLog("4   - size in longs:          %d\n", SizeInLongs);
    Service->Log.AddLog("8   - checksum:               %.8X\n", CheckSum);
    Service->Log.AddLog("12  - host id:                %d\n", HostID);
    Service->Log.AddLog("16  - next:                   %d\n\n", Next);
  }
}
