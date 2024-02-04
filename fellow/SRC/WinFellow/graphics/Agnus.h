#pragma once

#include "CustomChipset/IAgnus.h"
#include "Scheduler/SchedulerEvent.h"
#include "Scheduler/Timekeeper.h"
#include "Scheduler/FrameParameters.h"

class Agnus : public IAgnus
{
private:
  SchedulerEvent &_eolEvent;
  SchedulerEvent &_eofEvent;
  Timekeeper &_timekeeper;

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

  Agnus(SchedulerEvent &eolEvent, SchedulerEvent &eofEvent, Timekeeper &timekeeper, FrameParameters &currentFrameParameters);
  virtual ~Agnus();
};