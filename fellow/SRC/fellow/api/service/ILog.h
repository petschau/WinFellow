#ifndef FELLOW_API_SERVICE_ILOG_H
#define FELLOW_API_SERVICE_ILOG_H

#include <string>
#include "fellow/api/defs.h"

namespace fellow::api::service
{
  class ILog
  {
  public:
    virtual ~ILog() = default;
    virtual void AddLogDebug(const char *format, ...) = 0;
    virtual void AddLog(const char *, ...) = 0;
    virtual void AddLog2(STR *msg) = 0;
  };
}

#endif
