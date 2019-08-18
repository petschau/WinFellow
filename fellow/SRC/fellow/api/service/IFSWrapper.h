#pragma once

#include <string>
#include "fellow/api/defs.h"

namespace fellow::api
{
  enum class fs_wrapper_object_types
  {
    FILE,
    DIRECTORY,
    OTHER
  };

  struct fs_wrapper_object_info
  {
    std::string name;
    bool writeable;
    ULO size;
    fs_wrapper_object_types type;
  };

  class IFSWrapper
  {
  public:
    virtual fs_wrapper_object_info *GetFSObjectInfo(const std::string &pathToObject) = 0;
    virtual ~IFSWrapper() = default;
  };
} // namespace fellow::api
