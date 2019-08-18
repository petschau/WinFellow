#pragma once

#include "fellow/api/modules/IHardfileHandler.h"
#include "fellow/hardfile/rdb/RDBFileReader.h"
#include "fellow/hardfile/rdb/RDB.h"

namespace fellow::hardfile::rdb
{
  class RDBHandler
  {
  private:
    static unsigned int GetIndexOfRDB(RDBFileReader &reader);

  public:
    static fellow::api::modules::rdb_status HasRigidDiskBlock(RDBFileReader &reader);
    static RDB *GetDriveInformation(RDBFileReader &reader, bool geometryOnly = false);
  };
}
