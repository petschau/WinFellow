#ifndef TEST_MOCK_FELLOW_API_SERVICE_FSWRAPPERMOCK_H
#define TEST_MOCK_FELLOW_API_SERVICE_FSWRAPPERMOCK_H

#include "fellow/api/service/IFSWrapper.h"

using namespace fellow::api::service;

namespace test::mock::fellow::api::service
{
  class FSWrapperMock : public ::fellow::api::service::IFSWrapper
  {
  public:
    fs_wrapper_point *MakePoint(STR *point) override;
  };
}

#endif
