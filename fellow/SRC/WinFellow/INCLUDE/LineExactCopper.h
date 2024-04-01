#pragma once

#include "CustomChipset/Copper/ICopper.h"
#include "Scheduler/Scheduler.h"
#include "CustomChipset/Copper/CopperRegisters.h"

class LineExactCopper : public ICopper
{
private:
  static uint32_t cycletable[16];

  const MasterTimeOffset FrameStartDelay = MasterTimeOffset::FromChipTimeOffset(ChipTimeOffset{.Offset = 40});
  const ChipTimeOffset LoadStartDelay = ChipTimeOffset{.Offset = 4};
  const ChipTimeOffset InstructionInterval = ChipTimeOffset{.Offset = 4};
  const MasterTimeOffset CopperCycleCpuDelay = MasterTimeOffset::FromChipTimeOffset(ChipTimeOffset{.Offset = 2});

  Scheduler &_scheduler;
  SchedulerEvent &_copperEvent;
  SchedulerEvent &_cpuEvent;
  FrameParameters &_currentFrameParameters;
  Clocks &_clocks;
  CopperRegisters &_copperRegisters;

  /*============================================================================*/
  /* Translation table for raster ypos to cycle translation                     */
  /*============================================================================*/

  uint32_t ytable[512];

  void YTableInit();
  ChipTimestamp GetCheckedWaitTime(ChipTimestamp waitTimestamp);
  void RemoveEvent();
  void InsertEvent(MasterTimestamp timestamp);

  ChipTimestamp MakeTimestamp(const ChipTimestamp timestamp, const ChipTimeOffset offset) const;

public:
  virtual void NotifyDMAEnableChanged(bool new_dma_enable_state);
  virtual void NotifyCop1lcChanged();
  virtual void Load(uint32_t new_copper_pc);
  virtual void EventHandler();

  void InitializeCopperEvent() override;

  virtual void EndOfFrame();
  virtual void HardReset();
  virtual void EmulationStart();
  virtual void EmulationStop();

  LineExactCopper(
      Scheduler &scheduler, SchedulerEvent &copperEvent, SchedulerEvent &cpuEvent, FrameParameters &currentFrameParameters, Clocks &clocks, CopperRegisters &copperRegisters);
  virtual ~LineExactCopper();
};
