#pragma once

#include "CustomChipset/Copper/ICopper.h"
#include "Scheduler/Scheduler.h"
#include "CustomChipset/Copper/CopperRegisters.h"

class LineExactCopper : public ICopper
{
private:
  static uint32_t cycletable[16];

  Scheduler &_scheduler;
  SchedulerEvent &_copperEvent;
  SchedulerEvent &_cpuEvent;
  FrameParameters &_currentFrameParameters;
  Timekeeper &_timekeeper;
  CopperRegisters &_copperRegisters;

  /*============================================================================*/
  /* Translation table for raster ypos to cycle translation                     */
  /*============================================================================*/

  uint32_t ytable[512];

  void YTableInit();
  uint32_t GetCheckedWaitCycle(uint32_t waitCycle);
  void RemoveEvent();
  void InsertEvent(uint32_t cycle);

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
      Scheduler &scheduler,
      SchedulerEvent &copperEvent,
      SchedulerEvent &cpuEvent,
      FrameParameters &currentFrameParameters,
      Timekeeper &timekeeper,
      CopperRegisters &copperRegisters);
  virtual ~LineExactCopper();
};
