#pragma once

#include "fellow/api/service/IFSWrapper.h"
#include "FSWRAP.H"

namespace fellow::service
{
  class FSWrapper : public fellow::api::service::IFSWrapper
  {
  private:
    fellow::api::service::fs_wrapper_file_types MapFileType(fs_navig_file_types type);

  public:
    fellow::api::service::fs_wrapper_point *MakePoint(const char *point) override;
    FSWrapper();
  };
}
