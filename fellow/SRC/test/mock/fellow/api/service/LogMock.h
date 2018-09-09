#ifndef TEST_MOCK_FELLOW_API_SERVICE_LOGMOCK_H
#define TEST_MOCK_FELLOW_API_SERVICE_LOGMOCK_H

#include "fellow/api/service/ILog.h"

namespace test::mock::fellow::api::service
{
  class LogMock : public ::fellow::api::service::ILog
  {
  public:
    void AddLog(const char *, ...) override;
    void AddLog2(STR *msg) override;
  };
}

#endif
