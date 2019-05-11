#ifndef FELLOW_HARDFILE_RDB_RDBHANDLER_H
#define FELLOW_HARDFILE_RDB_RDBHANDLER_H

#include "fellow/hardfile/rdb/RDBFileReader.h"
#include "fellow/hardfile/rdb/RDB.h"

namespace fellow::hardfile::rdb
{
  class RDBHandler
  {
  public:
    static bool HasRigidDiskBlock(RDBFileReader& reader);
    static RDB* GetDriveInformation(RDBFileReader& reader, bool geometryOnly = false);
  };
}

#endif
