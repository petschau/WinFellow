#include "mock/service/FSWrapperMock.h"

using namespace fellow::api::service;

namespace test::mock::fellow::api::service
{
  fs_wrapper_point *FSWrapperMock::MakePoint(const char *point)
  {
    return new fs_wrapper_point();
  }
}
