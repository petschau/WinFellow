#pragma once

#include <list>
#include "fellow/api/drivers/ITimerDriver.h"

class TimerDriver : public ITimerDriver
{
private:
  bool _running;
  ULO _mmresolution;
  ULO _mmtimer;
  ULO _ticks;
  std::list<timerCallbackFunction> _callbacks;

  static void CALLBACK Callback(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

public:
  void HandleCallback();

  void AddCallback(timerCallbackFunction callback) override;
  ULO GetTimeMs() override;

  void EmulationStart() override;
  void EmulationStop() override;
  void Startup() override;
  void Shutdown() override;

  virtual ~TimerDriver() = default;
};
