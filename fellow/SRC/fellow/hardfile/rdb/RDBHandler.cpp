#include "fellow/hardfile/rdb/RDBFileReader.h"
#include "fellow/hardfile/rdb/RDBHandler.h"

#ifdef _DEBUG
  #define _CRTDBG_MAP_ALLOC
  #include <cstdlib>
  #include <crtdbg.h>
  #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
  #define DBG_NEW new
#endif

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
    RDB* rdb = DBG_NEW RDB();
    rdb->ReadFromFile(reader, geometryOnly);
    rdb->Log();
    return rdb;
  }
}
