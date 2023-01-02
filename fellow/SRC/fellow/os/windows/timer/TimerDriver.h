#pragma once
#include <list>
#include "fellow/api/drivers/ITimerDriver.h"

class TimerDriver : public ITimerDriver
{
private:
  bool _running = false;
  ULO _mmresolution = 0;
  ULO _mmtimer = 0;
  ULO _ticks = 0;
  std::list<std::function<void(ULO)>> _callbacks;

  static void CALLBACK Callback(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

public:
  void HandleCallback();

  void AddCallback(const std::function<void(ULO)>& callback) override;
  ULO GetTimeMs() override;

  void EmulationStart() override;
  void EmulationStop() override;
  void Initialize() override;
  void Release() override;

  virtual ~TimerDriver() = default;
};
