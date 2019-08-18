#pragma once

#include <cstdint>

namespace fellow::api
{
  class IPerformanceCounter
  {
  public:
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual int64_t GetAverage() const = 0;
    virtual int64_t GetTotalTime() const = 0;
    virtual unsigned int GetCallCount() const = 0;
    virtual const char *GetName() const = 0;
    virtual int64_t GetFrequency() = 0;
    virtual void LogTimerProperties() = 0;

    IPerformanceCounter() = default;
    virtual ~IPerformanceCounter() = default;
  };

  class IPerformanceCounterFactory
  {
  public:
    virtual IPerformanceCounter *Create(const char *name) = 0;

    virtual ~IPerformanceCounterFactory() = default;
  };
} // namespace fellow::api
