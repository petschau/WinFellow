#pragma once

#include <cstdint>
#include <string>

namespace Service
{
  enum class FileType
  {
    Other,
    File,
    Directory,
  };

  struct FileProperties
  {
    std::string Name;
    bool IsWritable;
    uintmax_t Size;
    FileType Type;
  };

  struct FilePropertiesW
  {
    std::wstring Name;
    bool IsWritable;
    uintmax_t Size;
    FileType Type;
  };

  class IFileInformation
  {
  public:
    virtual ~IFileInformation() = default;

    virtual FileProperties *GetFileProperties(const std::string &filename) = 0;
    virtual FilePropertiesW *GetFilePropertiesW(const std::wstring &filename) = 0;
  };
}
