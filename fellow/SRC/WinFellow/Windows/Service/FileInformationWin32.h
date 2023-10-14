#pragma once

#include "Service/IFileInformation.h"

class FileInformationWin32 : public Service::IFileInformation
{
public:
  Service::FileProperties *GetFileProperties(const char *filename) override;
  int Stat(const char* szFilename, struct stat* pStatBuffer) override;
};
