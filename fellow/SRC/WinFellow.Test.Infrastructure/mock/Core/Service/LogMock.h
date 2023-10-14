#pragma once

#include "Service/ILog.h"

namespace mock::Core::Service
{
  class LogMock : public ::Service::ILog
  {
  public:
    void AddLogDebug(const char*, ...) override;
    void AddLog(const char*, ...) override;
    void AddLog2(char* msg) override;
    void AddLogList(const std::list<std::string>& messages) override;
    void AddTimelessLog(const char* format, ...) override;
  };
}
