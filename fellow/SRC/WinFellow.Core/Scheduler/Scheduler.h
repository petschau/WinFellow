#pragma once

#include <cstdint>

#include "Timekeeper.h"
#include "SchedulerEvents.h"
#include "SchedulerQueue.h"
#include "FrameParameters.h"

constexpr auto BUS_CYCLE_FROM_280NS_SHIFT = 3;
constexpr auto BUS_CYCLE_FROM_140NS_SHIFT = 2;
constexpr auto BUS_CYCLE_FROM_280NS_MULTIPLIER = 8;
constexpr auto BUS_CYCLE_FROM_140NS_MULTIPLIER = 4;
constexpr uint32_t BUS_CYCLE_140NS_MASK = 0xfffffffc;

class Scheduler
{
private:
  typedef void (Scheduler::*ChipBusRunHandlerFunc)();

  Timekeeper &_timekeeper;
  SchedulerEvents &_events;
  FrameParameters &_currentFrameParameters;

  SchedulerQueue _queue;

  bool _stopEmulationRequested{};

  void RunGeneric();

  void MasterEventLoop_InstructionsUntilChipAndThenChipUntilNextInstruction();
  void MasterEventLoop_InstructionsUntilChipAndThenChipUntilNextInstruction_Assume68000();
  void MasterEventLoop_DebugStepOneInstruction();
  void MasterEventLoop_DebugStepUntilPc(uint32_t overPc);

public:
  void RequestEmulationStop();
  void ClearRequestEmulationStop();

  void DetermineCpuInstructionEventHandler();

  void InitializeQueue();
  void ClearQueue();
  void InsertEvent(SchedulerEvent *ev);
  void RemoveEvent(SchedulerEvent *ev);
  // void RebaseForNewFrame(SchedulerEvent *ev);
  // void RebaseForNewFrameWithNullCheck(SchedulerEvent *ev);
  SchedulerEvent *PeekNextEvent();

  void DisableEvent(SchedulerEvent &ev);

  void HandleNextEvent();

  void Run();
  bool DebugStepOneInstruction();
  bool DebugStepUntilPc(uint32_t untilPc);

  void NewFrame(const FrameParameters &newFrameParameters);

  void EmulationStart();
  void EmulationStop();
  void SoftReset();
  void HardReset();

  void Startup();
  void Shutdown();

  Scheduler(Timekeeper &timekeeper, SchedulerEvents &events, FrameParameters &currentFrameParameters);
};
