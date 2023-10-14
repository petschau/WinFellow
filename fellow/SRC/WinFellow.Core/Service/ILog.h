#pragma once

#include <string>
#include <list>

namespace Service
{
  class ILog
  {
  public:
    virtual void AddLogDebug(const char* format, ...) = 0;
    virtual void AddLog(const char*, ...) = 0;
    virtual void AddLogList(const std::list<std::string>& messages) = 0;
    virtual void AddLog2(char* msg) = 0;
    virtual void AddTimelessLog(const char* format, ...) = 0;

    ILog() = default;
    virtual ~ILog() = default;
  };
}
