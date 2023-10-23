#include "mock/Core/Service/FileInformationMock.h"

using namespace ::Service;
using namespace std;

namespace mock::Core::Service
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
