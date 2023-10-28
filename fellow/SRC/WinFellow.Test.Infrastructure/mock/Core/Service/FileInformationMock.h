#pragma once

#include "Service/IFileInformation.h"

namespace mock::Core::Service
{
  class FileInformationMock : public ::Service::IFileInformation
  {
  public:
    ::Service::FileProperties *GetFileProperties(const std::string &filename) override;
    ::Service::FilePropertiesW *GetFilePropertiesW(const std::wstring &filename) override;
  };
}
