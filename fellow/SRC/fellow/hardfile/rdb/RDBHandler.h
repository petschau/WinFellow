#ifndef FELLOW_HARDFILE_RDB_RDBHANDLER_H
#define FELLOW_HARDFILE_RDB_RDBHANDLER_H

#include "fellow/api/module/IHardfileHandler.h"
#include "fellow/hardfile/rdb/RDBFileReader.h"
#include "fellow/hardfile/rdb/RDB.h"


namespace fellow::hardfile::rdb
{
  class RDBHandler
  {
  private:
    static unsigned int GetIndexOfRDB(RDBFileReader& reader);

  public:
    static fellow::api::module::rdb_status HasRigidDiskBlock(RDBFileReader& reader);
    static RDB* GetDriveInformation(RDBFileReader& reader, bool geometryOnly = false);
  };
}

#endif
