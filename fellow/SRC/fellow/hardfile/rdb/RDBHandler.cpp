#include "fellow/hardfile/rdb/RDBFileReader.h"
#include "fellow/hardfile/rdb/RDBHandler.h"

using namespace std;

namespace fellow::hardfile::rdb
{
  bool RDBHandler::HasRigidDiskBlock(RDBFileReader& reader)
  {
    string headerID = reader.ReadString(0, 4);
    return headerID == "RDSK";
  }

  RDB* RDBHandler::GetDriveInformation(RDBFileReader& reader)
  {
    RDB* rdb = new RDB();
    rdb->ReadFromFile(reader);
    rdb->Log();
    return rdb;
  }
}
