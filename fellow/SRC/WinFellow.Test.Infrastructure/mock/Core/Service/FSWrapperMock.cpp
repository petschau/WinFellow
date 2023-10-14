#include "mock/Core/Service/FSWrapperMock.h"

using namespace Service;

namespace mock::Core::Service
{
  fs_wrapper_point *FSWrapperMock::MakePoint(const char *point)
  {
    return new fs_wrapper_point();
  }

  int FSWrapperMock::Stat(const char* szFilename, struct stat* pStatBuffer)
  {
    return 0;
  }
}
