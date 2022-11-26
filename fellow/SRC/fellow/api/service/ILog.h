#pragma once

#include <string>
#include <list>

namespace fellow::api
{
  class ILog
  {
  public:
    virtual void AddLogDebug(const char *format, ...) = 0;
    virtual void AddLog(const char *format, ...) = 0;
    virtual void AddLog(const std::string &message) = 0;
    virtual void AddLogList(const std::list<std::string> &messages) = 0;
    virtual void AddLog2(const char *msg) = 0;
    virtual void AddTimelessLog(const char *format, ...) = 0;
    virtual void AddTimelessLog(const std::string &message) = 0;

    virtual ~ILog() = default;
  };
}
