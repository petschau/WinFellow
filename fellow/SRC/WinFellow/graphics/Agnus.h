#pragma once

#include "CustomChipset/IAgnus.h"
#include "Scheduler/SchedulerEvent.h"
#include "Scheduler/Clocks.h"
#include "Scheduler/FrameParameters.h"

class Agnus : public IAgnus
{
private:
  SchedulerEvent &_eolEvent;
  SchedulerEvent &_eofEvent;
  Clocks &_clocks;

  FrameParameters _palLongFrameParameters{};
  FrameParameters _palShortFrameParameters{};
  FrameParameters &_currentFrameParameters;

  void InitializePalLongFrameParameters();
  void InitializePalShortFrameParameters();
  void InitializePredefinedFrameParameters();

public:
  void SetFrameParameters(bool isLongFrame) override;

  void EndOfLine() override;
  void EndOfFrame() override;

  void InitializeEndOfLineEvent() override;
  void InitializeEndOfFrameEvent() override;

  void HardReset() override;

  Agnus(SchedulerEvent &eolEvent, SchedulerEvent &eofEvent, Clocks &clocks, FrameParameters &currentFrameParameters);
  virtual ~Agnus();
};