#ifndef FELLOW_SERVICE_FSWRAPPER_H
#define FELLOW_SERVICE_FSWRAPPER_H

#include "fellow/api/service/IFSWrapper.h"
#include "FSWRAP.H"

namespace fellow::service
{
  class FSWrapper : public fellow::api::service::IFSWrapper
  {
  private:
    fellow::api::service::fs_wrapper_file_types MapFileType(fs_navig_file_types type);

  public:
    fellow::api::service::fs_wrapper_point *MakePoint(const STR *point) override;
    FSWrapper();
  };
}

#endif
