#pragma once

#include "Service/IFileInformation.h"

namespace mock::Core::Service
{
  class FileInformationMock : public ::Service::IFileInformation
  {
  public:
    ::Service::FileProperties *GetFileProperties(const char *filename) override;
    int Stat(const char *szFilename, struct stat *pStatBuffer) override;
  };
}
