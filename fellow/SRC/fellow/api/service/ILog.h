#pragma once

#include <string>
#include <list>
#include "fellow/api/defs.h"

namespace fellow::api
{
  enum class FELLOW_REQUESTER_TYPE
  {
    FELLOW_REQUESTER_TYPE_NONE = 0,
    FELLOW_REQUESTER_TYPE_INFO = 1,
    FELLOW_REQUESTER_TYPE_WARN = 2,
    FELLOW_REQUESTER_TYPE_ERROR = 3
  };

  class ILog
  {
  public:
    virtual void AddLogDebug(const char *format, ...) = 0;
    virtual void AddLog(const char *format, ...) = 0;
    virtual void AddLog(const std::string &message) = 0;
    virtual void AddLogList(const std::list<std::string> &messages) = 0;
    virtual void AddLog2(STR *msg) = 0;
    virtual void AddTimelessLog(const char *format, ...) = 0;
    virtual void AddLogRequester(FELLOW_REQUESTER_TYPE type, const char *format, ...) = 0;

    virtual ~ILog() = default;
  };
}
