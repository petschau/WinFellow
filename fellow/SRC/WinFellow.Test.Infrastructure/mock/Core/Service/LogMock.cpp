#include "mock/Core/Service/LogMock.h"

using namespace std;

namespace mock::Core::Service
{
  void LogMock::AddLogDebug(const char *, ...)
  {
    // Stub
  }

  void LogMock::AddLog(const char *, ...)
  {
    // Stub
  }

  void LogMock::AddLog2(const char *msg)
  {
    // Stub
  }

  void LogMock::AddLogList(const list<string> &messages)
  {
    // Stub
  }

  void LogMock::AddTimelessLog(const char *, ...)
  {
    // Stub
  }
}
