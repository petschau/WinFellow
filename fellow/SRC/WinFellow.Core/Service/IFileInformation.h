#pragma once

#include <string>

namespace Service
{
  typedef enum
  {
    File,
    Directory,
    Other
  } FileType;

  struct FileProperties
  {
    std::string Name;
    bool IsWritable;
    uint32_t Size;
    FileType Type;
  };

  class IFileInformation
  {
  public:
    virtual ~IFileInformation() = default;
    virtual FileProperties *GetFileProperties(const char *filename) = 0;
    virtual int Stat(const char* szFilename, struct stat* pStatBuffer) = 0;
  };
}
