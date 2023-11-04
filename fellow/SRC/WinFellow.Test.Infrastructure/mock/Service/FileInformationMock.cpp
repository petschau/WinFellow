#include "mock/Service/FileInformationMock.h"

using namespace ::Service;
using namespace std;

namespace mock::Service
{
  FileProperties *FileInformationMock::GetFileProperties(const string &filename)
  {
    return new FileProperties{.Name = filename};
  }

  FilePropertiesW *FileInformationMock::GetFilePropertiesW(const wstring &filename)
  {
    return new FilePropertiesW{.Name = filename};
  }
}
