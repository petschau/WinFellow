#pragma once

#include "CustomChipset/Copper/ICopper.h"
#include "Scheduler/Scheduler.h"
#include "DebugApi/DebugLog.h"

enum class CopperStates
{
  COPPER_STATE_NONE = 0,
  COPPER_STATE_READ_FIRST_WORD = 1,
  COPPER_STATE_READ_SECOND_WORD = 2,
  COPPER_STATE_TRANSFER_SECOND_WORD = 3
};

class CycleExactCopper : public ICopper
{
private:
  Scheduler &_scheduler;
  SchedulerEvent &_copperEvent;
  SchedulerEvent &_cpuEvent;
  FrameParameters &_currentFrameParameters;
  Clocks &_clocks;
  CopperRegisters &_copperRegisters;
  DebugLog &_debugLog;

  CopperStates _state;
  uint16_t _first;
  uint16_t _second;
  bool _skip_next;

  uint16_t ReadWord();
  void IncreasePtr();

  void SetState(CopperStates newState, uint32_t chipLine, uint32_t chipLineCycle);
  void SetStateNone();

  bool IsRegisterAllowed(uint32_t regno);

  void Move();
  void Wait();
  void Skip();
  bool IsMove();
  bool IsWait();

  void ReadFirstWord();
  void ReadSecondWord();
  void TransferSecondWord();

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

  CycleExactCopper(
      Scheduler &scheduler,
      SchedulerEvent &copperEvent,
      SchedulerEvent &cpuEvent,
      FrameParameters &currentFrameParameters,
      Clocks &clocks,
      CopperRegisters &copperRegisters,
      DebugLog &debugLog);
  virtual ~CycleExactCopper();
};
