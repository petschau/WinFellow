#include "mock/service/LogMock.h"

using namespace std;

namespace test::mock::fellow::api::service
{
  void LogMock::AddLogDebug(const char *, ...)
  {
    // Stub
  }

  void LogMock::AddLog(const char *, ...)
  {
    // Stub
  }

  void LogMock::AddLog2(STR *msg)
  {
    // Stub
  }

  void LogMock::AddLogList(const list<string>& messages)
  {
    // Stub
  }
}
