#include "test/mock/fellow/api/service/FSWrapperMock.h"

using namespace std;
using namespace fellow::api;

namespace test::mock::fellow::api
{
  fs_wrapper_object_info *FSWrapperMock::GetFSObjectInfo(const string &pathToObject)
  {
    return new fs_wrapper_object_info();
  }
}
