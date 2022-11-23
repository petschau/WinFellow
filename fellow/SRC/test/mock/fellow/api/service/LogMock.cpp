#include "test/mock/fellow/api/service/LogMock.h"

using namespace std;

namespace test::mock::fellow::api
{
  void LogMock::AddLogDebug(const char *, ...)
  {
  }

  void LogMock::AddLog(const char *, ...)
  {
  }

  void LogMock::AddLog(const string &message)
  {
  }

  void LogMock::AddLog2(STR *msg)
  {
  }

  void LogMock::AddLogList(const list<string> &messages)
  {
  }

  void LogMock::AddTimelessLog(const char *format, ...)
  {
  }
}
