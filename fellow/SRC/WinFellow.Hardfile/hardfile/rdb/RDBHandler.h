#pragma once

#include "fellow/api/module/IHardfileHandler.h"
#include "hardfile/rdb/RDBFileReader.h"
#include "hardfile/rdb/RDB.h"

namespace fellow::hardfile::rdb
{
  class RDBHandler
  {
  private:
    static unsigned int GetIndexOfRDB(RDBFileReader &reader);

  public:
    static fellow::api::module::rdb_status HasRigidDiskBlock(RDBFileReader &reader);
    static RDB *GetDriveInformation(RDBFileReader &reader, bool geometryOnly = false);
  };
}
