#ifndef FELLOW_API_SERVICE_IFSWRAPPER_H
#define FELLOW_API_SERVICE_IFSWRAPPER_H

#include <string>
#include "fellow/api/defs.h"

namespace fellow::api::service
{
  typedef enum
  {
    FS_NAVIG_FILE,
    FS_NAVIG_DIR,
    FS_NAVIG_OTHER
  } fs_wrapper_file_types;

  typedef struct
  {
    UBY drive;
    std::string name;
    bool relative;
    bool writeable;
    ULO size;
    fs_wrapper_file_types type;
  } fs_wrapper_point;

  class IFSWrapper
  {
  public:
    virtual ~IFSWrapper() = default;
    virtual fs_wrapper_point *MakePoint(const STR *point) = 0;
  };
}

#endif
