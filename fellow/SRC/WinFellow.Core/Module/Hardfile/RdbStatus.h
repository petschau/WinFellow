#pragma once

namespace Module::Hardfile
{
  enum class rdb_status
  {
    RDB_NOT_FOUND = 0,
    RDB_FOUND = 1,
    RDB_FOUND_WITH_HEADER_CHECKSUM_ERROR = 2,
    RDB_FOUND_WITH_PARTITION_ERROR = 3
  };
}
