#pragma once

#include "fellow/api/service/IFSWrapper.h"

namespace test::mock::fellow::api::service
{
  class FSWrapperMock : public ::fellow::api::service::IFSWrapper
  {
  public:
    ::fellow::api::service::fs_wrapper_point *MakePoint(const STR *point) override;
  };
}
