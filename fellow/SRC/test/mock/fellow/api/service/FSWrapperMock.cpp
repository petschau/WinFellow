#include "test/mock/fellow/api/service/FSWrapperMock.h"

namespace test::mock::fellow::api::service
{
  fs_wrapper_point *FSWrapperMock::MakePoint(STR *point)
  {
    return new fs_wrapper_point();
  }
}
