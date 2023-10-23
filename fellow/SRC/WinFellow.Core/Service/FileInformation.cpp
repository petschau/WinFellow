#include "Service/FileInformation.h"
#include "VirtualHost/Core.h"

#include <fstream>

using namespace std;
using namespace Service;

FileProperties* FileInformation::GetFileProperties(const string& filename)
{
  FileProperties* fileProperties = nullptr;

  try
  {
    auto filepath = filename;
    auto filestatus = filesystem::status(filepath);

    if (!filesystem::exists(filestatus))
    {
      return nullptr;
    }

    auto permissions = filestatus.permissions();
    if (!GetIsReadable(permissions))
    {
      return nullptr;
    }

    fileProperties = new FileProperties
    {
        .Name = filename,
        .IsWritable = false,
        .Size = 0,
        .Type = GetFileType(filestatus)
    };

    if (fileProperties->Type != FileType::File)
    {
      return fileProperties;
    }

    fileProperties->IsWritable = GetIsWritable(permissions, filepath);
    fileProperties->Size = filesystem::file_size(filepath);
  }
  catch (filesystem::filesystem_error error)
  {
    delete fileProperties;
    return nullptr;
  }

  return fileProperties;
}

FilePropertiesW* FileInformation::GetFilePropertiesW(const wstring& filename)
{
  FilePropertiesW* fileProperties = nullptr;

  try
  {
    auto filepath = filename;
    auto filestatus = filesystem::status(filepath);

    if (!filesystem::exists(filestatus))
    {
      return nullptr;
    }

    auto permissions = filestatus.permissions();
    if (!GetIsReadable(permissions))
    {
      return nullptr;
    }

    fileProperties = new FilePropertiesW
    {
        .Name = filename,
        .IsWritable = false,
        .Size = 0,
        .Type = GetFileType(filestatus)
    };

    if (fileProperties->Type != FileType::File)
    {
      return fileProperties;
    }

    fileProperties->IsWritable = GetIsWritable(permissions, filepath);
    fileProperties->Size = filesystem::file_size(filepath);
  }
  catch (filesystem::filesystem_error error)
  {
    delete fileProperties;
    return nullptr;
  }

  return fileProperties;
}

bool FileInformation::GetIsReadable(const filesystem::perms permissions)
{
  return (permissions & std::filesystem::perms::owner_read) != std::filesystem::perms::none;
}

bool FileInformation::GetIsWritable(const filesystem::perms permissions, const filesystem::path& filename)
{
  if ((permissions & std::filesystem::perms::owner_write) == std::filesystem::perms::none)
  {
    return false;
  }

  ofstream ofs(filename, ios::app);
  bool failed = ofs.fail();
  ofs.close();

  return !failed;
}

FileType FileInformation::GetFileType(filesystem::file_status& filestatus)
{
  if (filesystem::is_regular_file(filestatus))
  {
    return FileType::File;
  }
  else if (filesystem::is_directory(filestatus))
  {
    return FileType::Directory;
  }

  return FileType::Other;
}
