#pragma once

#include "Service/ILog.h"

namespace mock::Service
{
  class LogMock : public ::Service::ILog
  {
  public:
    void AddLogDebug(const char *, ...) override;
    void AddLog(const char *, ...) override;
    void AddLog2(const char *msg) override;
    void AddLogList(const std::list<std::string> &messages) override;
    void AddTimelessLog(const char *format, ...) override;
  };
}
