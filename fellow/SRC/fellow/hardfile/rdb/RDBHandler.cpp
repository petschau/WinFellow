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

  RDB* RDBHandler::GetDriveInformation(RDBFileReader& reader, bool geometryOnly)
  {
    RDB* rdb = new RDB();
    rdb->ReadFromFile(reader, geometryOnly);
    rdb->Log();
    return rdb;
  }
}
