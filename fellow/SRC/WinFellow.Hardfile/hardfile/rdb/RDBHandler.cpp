#include "fellow/api/defs.h"
#include "hardfile/rdb/RDBFileReader.h"
#include "hardfile/rdb/RDBHandler.h"
#include "hardfile/rdb/CheckSumCalculator.h"

using namespace std;
using namespace fellow::api::module;

namespace fellow::hardfile::rdb
{
  unsigned int RDBHandler::GetIndexOfRDB(RDBFileReader& reader)
  {
    for (unsigned int i = 0; i < 16; i++)
    {
      unsigned int index = i * 512;
      string headerID = reader.ReadString(index, 4);
      if (headerID == "RDSK")
      {
        return index;
      }
    }
    return 0xffffffff;
  }

  rdb_status RDBHandler::HasRigidDiskBlock(RDBFileReader& reader)
  {
    unsigned int indexOfRDB = GetIndexOfRDB(reader);

    if (indexOfRDB == 0xffffffff)
    {
      return rdb_status::RDB_NOT_FOUND;
    }

    bool hasValidCheckSum = CheckSumCalculator::HasValidCheckSum(reader, 128, indexOfRDB);

    if (!hasValidCheckSum)
    {
      return rdb_status::RDB_FOUND_WITH_HEADER_CHECKSUM_ERROR;
    }

    RDB* rdb = GetDriveInformation(reader, true);
    if (rdb->HasPartitionErrors)
    {
      delete rdb;
      return rdb_status::RDB_FOUND_WITH_PARTITION_ERROR;
    }

    delete rdb;
    return rdb_status::RDB_FOUND;
  }

  RDB* RDBHandler::GetDriveInformation(RDBFileReader& reader, bool geometryOnly)
  {
    unsigned int indexOfRDB = GetIndexOfRDB(reader);

    if (indexOfRDB == 0xffffffff)
    {
      return nullptr;
    }

    RDB* rdb = new RDB();
    rdb->ReadFromFile(reader, indexOfRDB, geometryOnly);
    rdb->Log();
    return rdb;
  }
}
