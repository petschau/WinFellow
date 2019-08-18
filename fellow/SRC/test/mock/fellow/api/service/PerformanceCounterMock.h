#pragma once

#include "fellow/api/service/IPerformanceCounter.h"

namespace test::mock::fellow::api
{
  class PerformanceCounterMock : public ::fellow::api::IPerformanceCounter
  {
  private:
    const char *_name;

  public:
    void Start() override;
    void Stop() override;
    int64_t GetAverage() const override;
    int64_t GetTotalTime() const override;
    unsigned int GetCallCount() const override;
    const char *GetName() const override;
    int64_t GetFrequency() override;
    void LogTimerProperties() override;

    PerformanceCounterMock(const char *name);
    virtual ~PerformanceCounterMock() = default;
  };

  class PerformanceCounterFactoryMock : public ::fellow::api::IPerformanceCounterFactory
  {
  public:
    ::fellow::api::IPerformanceCounter *Create(const char *name) override;
  };
}
