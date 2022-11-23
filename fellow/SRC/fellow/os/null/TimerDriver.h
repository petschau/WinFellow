#pragma once

#include "fellow/api/drivers/ITimerDriver.h"

class TimerDriver : public ITimerDriver
{
public:
  void AddCallback(timerCallbackFunction callback) override;
  ULO GetTimeMs() override;

  void EmulationStart() override;
  void EmulationStop() override;
  void Startup() override;
  void Shutdown() override;
};
