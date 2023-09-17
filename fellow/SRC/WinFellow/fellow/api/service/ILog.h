#ifndef FELLOW_API_SERVICE_ILOG_H
#define FELLOW_API_SERVICE_ILOG_H

#include <string>
#include <list>
#include "fellow/api/defs.h"

namespace fellow::api::service
{
  class ILog
  {
  public:
    virtual ~ILog() = default;
    virtual void AddLogDebug(const char *format, ...) = 0;
    virtual void AddLog(const char *, ...) = 0;
    virtual void AddLogList(const std::list<std::string>& messages) = 0;
    virtual void AddLog2(char *msg) = 0;
  };
}

#endif
