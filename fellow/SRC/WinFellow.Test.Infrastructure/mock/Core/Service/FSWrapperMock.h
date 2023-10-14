#pragma once

#include "Service/IFSWrapper.h"

namespace mock::Core::Service
{
  class FSWrapperMock : public ::Service::IFSWrapper
  {
  public:
    ::Service::fs_wrapper_point *MakePoint(const char *point) override;
    int Stat(const char* szFilename, struct stat* pStatBuffer) override;
  };
}
