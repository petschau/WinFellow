#pragma once

#include <string>
#include <list>
#include "fellow/api/defs.h"

namespace fellow::api
{
  class ILog
  {
  public:
    virtual void AddLogDebug(const char *format, ...) = 0;
    virtual void AddLog(const char *format, ...) = 0;
    virtual void AddLog(const std::string &message) = 0;
    virtual void AddLogList(const std::list<std::string> &messages) = 0;
    virtual void AddLog2(STR *msg) = 0;
    virtual void AddTimelessLog(const char *format, ...) = 0;

    virtual ~ILog() = default;
  };
}
