#pragma once

#include "fellow/api/service/IFSWrapper.h"

namespace test::mock::fellow::api
{
  class FSWrapperMock : public ::fellow::api::IFSWrapper
  {
  public:
    ::fellow::api::fs_wrapper_object_info *GetFSObjectInfo(const std::string &pathToObject) override;
  };
}
