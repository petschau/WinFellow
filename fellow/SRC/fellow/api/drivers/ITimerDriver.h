#pragma once

#include <functional>
#include "fellow/api/defs.h"

class ITimerDriver
{
public:
  virtual void AddCallback(const std::function<void(ULO)>& callback) = 0;
  virtual ULO GetTimeMs() = 0;

  virtual void EmulationStart() = 0;
  virtual void EmulationStop() = 0;
  virtual void Initialize() = 0;
  virtual void Release() = 0;
};
