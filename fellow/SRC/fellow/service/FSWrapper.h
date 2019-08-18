#pragma once

#include "fellow/api/service/IFSWrapper.h"

namespace fellow::service
{
  class FSWrapper : public fellow::api::IFSWrapper
  {
  private:
    fellow::api::fs_wrapper_object_types GetFSObjectType(unsigned short st_mode);
    bool GetFSObjectIsWriteable(unsigned short st_mode, const std::string &pathToObject);

  public:
    fellow::api::fs_wrapper_object_info *GetFSObjectInfo(const std::string &pathToObject) override;
  };
}
