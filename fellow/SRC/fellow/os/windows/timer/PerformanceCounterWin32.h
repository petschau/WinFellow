#pragma once

#include <windows.h>
#include "fellow/api/service/IPerformanceCounter.h"

class PerformanceCounterWin32 : public fellow::api::IPerformanceCounter
{
private:
  const char *_name;
  unsigned int _callCount{0};
  LARGE_INTEGER _startTime{};
  LARGE_INTEGER _stopTime{};

  int64_t _totalTime{};
  static LARGE_INTEGER _frequency;
  static bool _frequencyHasValue;

public:
  void Start() override;
  void Stop() override;
  int64_t GetAverage() const override;
  int64_t GetTotalTime() const override;
  unsigned int GetCallCount() const override;
  const char *GetName() const override;
  int64_t GetFrequency() override;
  void LogTimerProperties() override;

  PerformanceCounterWin32(const char *name);
  virtual ~PerformanceCounterWin32() = default;
};

class PerformanceCounterFactoryWin32 : public fellow::api::IPerformanceCounterFactory
{
public:
  fellow::api::IPerformanceCounter *Create(const char *name) override;

  virtual ~PerformanceCounterFactoryWin32() = default;
};
