#include "fellow/service/FSWrapper.h"

using namespace fellow::api::service;

namespace fellow::service
{
  FSWrapper::FSWrapper()
  {
  }

  fs_wrapper_point *FSWrapper::MakePoint(const STR *point)
  {
    fs_navig_point *navig_point = fsWrapMakePoint(point);
    fs_wrapper_point *result = new fs_wrapper_point();
    result->drive = navig_point->drive;
    result->name = navig_point->name;
    result->relative = navig_point->relative;
    result->size = navig_point->size;
    result->type = MapFileType(navig_point->type);
    result->writeable = navig_point->writeable;
    free(navig_point);
    return result;
  }

  fs_wrapper_file_types FSWrapper::MapFileType(fs_navig_file_types type)
  {
    if (type == fs_navig_file_types::FS_NAVIG_FILE)
    {
      return fs_wrapper_file_types::FS_NAVIG_FILE;
    }
    if (type == fs_navig_file_types::FS_NAVIG_DIR)
    {
      return fs_wrapper_file_types::FS_NAVIG_DIR;
    }
    return fs_wrapper_file_types::FS_NAVIG_OTHER;
  }
}
