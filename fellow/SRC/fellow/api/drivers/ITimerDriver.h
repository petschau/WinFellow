#pragma once

#include "fellow/api/defs.h"

typedef void (*timerCallbackFunction)(ULO millisecondTicks);

class ITimerDriver
{
public:
  virtual void AddCallback(timerCallbackFunction callback) = 0;
  virtual ULO GetTimeMs() = 0;

  virtual void EmulationStart() = 0;
  virtual void EmulationStop() = 0;
  virtual void Startup() = 0;
  virtual void Shutdown() = 0;
};
