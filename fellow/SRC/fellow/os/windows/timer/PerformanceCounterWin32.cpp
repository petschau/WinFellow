#include "PerformanceCounterWin32.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

IPerformanceCounter *PerformanceCounterFactoryWin32::Create(const char *name)
{
  return new PerformanceCounterWin32(name);
}

LARGE_INTEGER PerformanceCounterWin32::_frequency{};
bool PerformanceCounterWin32::_frequencyHasValue = false;

void PerformanceCounterWin32::Start()
{
  QueryPerformanceCounter(&_startTime);
}

void PerformanceCounterWin32::Stop()
{
  QueryPerformanceCounter(&_stopTime);
  _callCount++;
  int64_t elapsedTime = _stopTime.QuadPart - _startTime.QuadPart;
  _totalTime += elapsedTime;
}

int64_t PerformanceCounterWin32::GetAverage() const
{
  if (_callCount == 0) return 0;
  return _totalTime / _callCount;
}

int64_t PerformanceCounterWin32::GetTotalTime() const
{
  return _totalTime;
}

unsigned int PerformanceCounterWin32::GetCallCount() const
{
  return _callCount;
}

const char *PerformanceCounterWin32::GetName() const
{
  return _name;
}

int64_t PerformanceCounterWin32::GetFrequency()
{
  if (!_frequencyHasValue)
  {
    QueryPerformanceFrequency(&_frequency);
    _frequencyHasValue = true;
  }

  return _frequency.QuadPart;
}

void PerformanceCounterWin32::LogTimerProperties()
{
  const int64_t frequency = GetFrequency();
  Service->Log.AddLogDebug("QueryPerformance Frequency: %lld\n", frequency);
}

PerformanceCounterWin32::PerformanceCounterWin32(const char *name) : _name(name)
{
}
