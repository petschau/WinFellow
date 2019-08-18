#include "PerformanceCounterMock.h"

using namespace fellow::api;

namespace test::mock::fellow::api
{

  void PerformanceCounterMock::Start()
  {
  }

  void PerformanceCounterMock::Stop()
  {
  }

  int64_t PerformanceCounterMock::GetAverage() const
  {
    return 0;
  }

  int64_t PerformanceCounterMock::GetTotalTime() const
  {
    return 0;
  }

  unsigned int PerformanceCounterMock::GetCallCount() const
  {
    return 0;
  }

  const char *PerformanceCounterMock::GetName() const
  {
    return _name;
  }

  int64_t PerformanceCounterMock::GetFrequency()
  {
    return 0;
  }

  void PerformanceCounterMock::LogTimerProperties()
  {
  }

  PerformanceCounterMock::PerformanceCounterMock(const char *name) : _name(name)
  {
  }

  IPerformanceCounter *PerformanceCounterFactoryMock::Create(const char *name)
  {
    return new PerformanceCounterMock(name);
  }
}
