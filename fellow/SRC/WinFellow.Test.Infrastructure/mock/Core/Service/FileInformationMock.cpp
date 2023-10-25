#include "mock/Core/Service/FileInformationMock.h"

using namespace Service;

namespace mock::Core::Service
{
  FileProperties *FileInformationMock::GetFileProperties(const char *filename)
  {
    return new FileProperties();
  }

  int FileInformationMock::Stat(const char *szFilename, struct stat *pStatBuffer)
  {
    return 0;
  }
}
