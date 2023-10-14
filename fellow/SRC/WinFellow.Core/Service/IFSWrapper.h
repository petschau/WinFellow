#pragma once

#include <string>

namespace Service
{
  typedef enum
  {
    FS_NAVIG_FILE,
    FS_NAVIG_DIR,
    FS_NAVIG_OTHER
  } fs_wrapper_file_types;

  struct fs_wrapper_point
  {
    std::string name;
    bool writeable;
    uint32_t size;
    fs_wrapper_file_types type;
  };

  class IFSWrapper
  {
  public:
    virtual ~IFSWrapper() = default;
    virtual fs_wrapper_point *MakePoint(const char *point) = 0;
    virtual int Stat(const char* szFilename, struct stat* pStatBuffer) = 0;
  };
}
