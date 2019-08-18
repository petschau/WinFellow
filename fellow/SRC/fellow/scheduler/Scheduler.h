#pragma once

#include "fellow/api/defs.h"
#include "fellow/time/Timestamps.h"
#include "fellow/scheduler/SchedulerQueue.h"

constexpr auto BUS_CYCLE_FROM_280NS_SHIFT = 3;
constexpr auto BUS_CYCLE_FROM_140NS_SHIFT = 2;
constexpr auto BUS_CYCLE_FROM_280NS_MULTIPLIER = 8;
constexpr auto BUS_CYCLE_FROM_140NS_MULTIPLIER = 4;
constexpr unsigned int BUS_CYCLE_140NS_MASK = 0xfffffffc;

struct FrameParameters
{
  ULO HorisontalBlankStart;
  ULO HorisontalBlankEnd;
  ULO VerticalBlankEnd;
  ULO ShortLineCycles;
  ULO LongLineCycles;
  ULO LinesInFrame;
  ULO CyclesInFrame;
  ULO MinimumDDF;
  ULO MaximumDDF;
};

class Scheduler
{
private:
  typedef void (Scheduler::*ChipBusRunHandlerFunc)();

  SchedulerQueue _queue;

  ULL FrameNo;
  ULO FrameCycle;
  ULO FrameCycleInsertGuard;

  SHResTimestamp CurrentCycleSHResTimestamp;
  ChipBusTimestamp CurrentCycleChipBusTimestamp;

  FrameParameters *FrameParameters;

  ULO GetHorisontalBlankEnd();
  ULO GetCylindersInLine(); // 140ns (LORES ref)
  ULO GetCycleFromCycle140ns(ULO cycle140ns);

  // Queue and event handling
  void LogPop(SchedulerEvent *ev);
  void InitializeCpuEvent();

  void InitializePalLongFrame();
  void InitializePalShortFrame();
  void InitializeScreenLimits();

  void Run68000Fast();
  void Run68000FastExperimental();
  void RunGeneric();
  ChipBusRunHandlerFunc GetRunHandler() const;
  SchedulerEventHandler GetCpuInstructionEventHandler();

public:
  void EndOfLine();
  void EndOfFrame();

  void DetermineCpuInstructionEventHandler();

  void InitializeQueue();
  void ClearQueue();
  void InsertEvent(SchedulerEvent *ev);
  void RemoveEvent(SchedulerEvent *ev);
  void RebaseForNewFrame(SchedulerEvent *ev);
  void RebaseForNewFrameWithNullCheck(SchedulerEvent *ev);
  SchedulerEvent *PeekNextEvent();
  void HandleNextEvent();

  void SetFrameCycle(ULO frameCycle);
  const ChipBusTimestamp &GetChipBusTimestamp();
  ULO GetVerticalBlankEnd();
  ULO GetLinesInFrame();
  ULL GetRasterFrameCount();
  ULO GetCyclesInLine();      // 35ns (SHRES ref)
  ULO Get280nsCyclesInLine(); // 280ns (Chip bus ref)
  const SHResTimestamp &GetSHResTimestamp();
  ULO GetCycleFromCycle280ns(ULO cycle280ns);
  unsigned int GetRasterX() const;
  unsigned int GetRasterY() const;
  ULO GetFrameCycle(bool doassert = false);

  ULO GetCycle280nsFromCycle(ULO cycle);
  ULO GetLineCycle280ns();
  ULO GetCyclesInFrame();
  ULO GetBaseClockTimestamp(ULO line, ULO cycle);
  ULO GetLastCycleInCurrentCylinder();
  ULO GetHorisontalBlankStart();

  void SetScreenLimits(bool is_long_frame);

  void Run();
  void DebugStepOneInstruction();

  void EmulationStart();
  void EmulationStop();
  void SoftReset();
  void HardReset();
  void Startup();
  void Shutdown();
};

extern Scheduler scheduler;

extern SchedulerEvent cpuEvent;
extern SchedulerEvent copperEvent;
extern SchedulerEvent eolEvent;
extern SchedulerEvent eofEvent;
extern SchedulerEvent ciaEvent;
extern SchedulerEvent blitterEvent;
extern SchedulerEvent interruptEvent;
extern SchedulerEvent chipBusAccessEvent;
extern SchedulerEvent ddfEvent;
extern SchedulerEvent diwxEvent;
extern SchedulerEvent diwyEvent;
extern SchedulerEvent shifterEvent;
extern SchedulerEvent bitplaneDMAEvent;
extern SchedulerEvent spriteDMAEvent;
