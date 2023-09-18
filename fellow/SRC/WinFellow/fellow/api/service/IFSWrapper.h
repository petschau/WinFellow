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
    uint8_t drive;
    std::string name;
    bool relative;
    bool writeable;
    uint32_t size;
    fs_wrapper_file_types type;
  } fs_wrapper_point;

  class IFSWrapper
  {
  public:
    virtual ~IFSWrapper() = default;
    virtual fs_wrapper_point *MakePoint(const char *point) = 0;
  };
}

#endif
