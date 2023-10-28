#pragma once

#include "Service/IFileInformation.h"
#include <filesystem>

class FileInformation : public Service::IFileInformation
{
private:
  Service::FileType GetFileType(std::filesystem::file_status &filestatus);
  bool GetIsReadable(const std::filesystem::perms permissions);
  bool GetIsWritable(const std::filesystem::perms permissions, const std::filesystem::path &filename);

public:
  Service::FileProperties *GetFileProperties(const std::string &filename) override;
  Service::FilePropertiesW *GetFilePropertiesW(const std::wstring &filename) override;
};
