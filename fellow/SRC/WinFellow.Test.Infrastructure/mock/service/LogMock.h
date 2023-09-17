#pragma once

#include "fellow/api/service/ILog.h"

namespace test::mock::fellow::api::service
{
  class LogMock : public ::fellow::api::service::ILog
  {
  public:
    void AddLogDebug(const char *, ...) override;
    void AddLog(const char *, ...) override;
    void AddLog2(char *msg) override;
    void AddLogList(const std::list<std::string>& messages) override;
  };
}
