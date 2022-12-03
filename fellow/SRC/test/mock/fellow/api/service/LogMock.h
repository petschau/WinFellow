#pragma once

#include "fellow/api/service/ILog.h"

namespace test::mock::fellow::api
{
  class LogMock : public ::fellow::api::ILog
  {
  public:
    void AddLogDebug(const char *, ...) override;
    void AddLog(const char *, ...) override;
    void AddLog(const std::string &message) override;
    void AddLog2(const char *msg) override;
    void AddLogList(const std::list<std::string> &messages) override;
    void AddTimelessLog(const char *format, ...) override;
    void AddTimelessLog(const std::string &message) override;
  };
}
