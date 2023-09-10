#ifndef TEST_MOCK_FELLOW_API_SERVICE_FSWRAPPERMOCK_H
#define TEST_MOCK_FELLOW_API_SERVICE_FSWRAPPERMOCK_H

#include "fellow/api/service/IFSWrapper.h"

namespace test::mock::fellow::api::service
{
  class FSWrapperMock : public ::fellow::api::service::IFSWrapper
  {
  public:
    ::fellow::api::service::fs_wrapper_point *MakePoint(const STR *point) override;
  };
}

#endif
