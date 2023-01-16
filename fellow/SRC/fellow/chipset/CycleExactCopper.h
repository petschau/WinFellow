#pragma once

#include "fellow/api/defs.h"
#include "fellow/time/Timestamps.h"

class CopperBeamPosition
{
private:
  ULO MaxCycle;

public:
  ULO Line;
  ULO Cycle;

  void AddAndMakeEven(ULO offset);
  void Initialize(ULO line, ULO cycle, ULO maxCycleInLines);

  CopperBeamPosition();
};

class CopperWaitParameters
{
private:
  ULO VerticalBeamPosition;
  ULO HorizontalBeamPosition;
  ULO VerticalPositionCompareEnable;
  ULO HorizontalPositionCompareEnable;
  ULO LinesInFrame;
  ULO MaxCycle;

  int CompareVertical(ULO line) const;
  int CompareHorizontal(ULO cycle) const;

  bool IsInsideVerticalBounds(ULO line) const;
  bool IsInsideHorizontalBounds(ULO cycle) const;

  bool TryGetNextHorizontalBeamMatch(CopperBeamPosition &testPosition) const;
  bool TryGetNextBeamMatchOnCurrentLine(CopperBeamPosition &testPosition) const;
  bool TryGetNextBeamMatchOnLaterLines(CopperBeamPosition &testPosition) const;

public:
  bool BlitterFinishedDisable;
  bool IsWaitingForBlitter;

  bool TryGetNextBeamMatch(CopperBeamPosition &testPosition) const;
  bool Compare(const CopperBeamPosition &testPosition) const;

  void Initialize(UWO ir1, UWO ir2, ULO linesInFrame, ULO maxCycle);
  CopperWaitParameters();
};

enum CopperStep
{
  CopperStep_Halt = 0,
  CopperStep_Read_IR1 = 1,
  CopperStep_Read_IR2 = 2,
  CopperStep_Move_IR2 = 3,
  CopperStep_Wait_Setup = 4,
  CopperStep_Wait_End = 5,
  CopperStep_Skip = 6,
  CopperStep_Load_Cycle1 = 7,
  CopperStep_Load_Cycle2 = 8
};

class Scheduler;
class Blitter;
class BitplaneRegisters;

class CycleExactCopper
{
private:
  Scheduler *_scheduler;
  Blitter *_blitter;
  BitplaneRegisters *_bitplaneRegisters;

  UWO _ir1;
  UWO _ir2;
  bool _skip;
  int _lcNumberToBeLoaded;
  CopperBeamPosition _currentBeamPosition;
  CopperWaitParameters _waitParameters;
  CopperStep _nextStep;

  void ExecuteStep(const ChipBusTimestamp &currentTime, UWO value);

  void StepHalt();
  void StepReadIR1(UWO value);
  void StepReadIR2(UWO value);
  void StepMoveIR2(UWO value);
  void StepWaitSetup();
  void StepWaitEnd();
  void StepSkip();
  void StepLoadCycle1();
  void StepLoadCycle2();

  void SetupStepReadIR1();

  void ScheduleDMARead(ULO offset);
  void ScheduleDMANull(ULO offset);
  void ScheduleEvent(ULO line, ULO cycle);

  void IncreasePC();

public:
  static void DMAReadCallback(const ChipBusTimestamp &currentTime, UWO value);
  static void DMANullCallback(const ChipBusTimestamp &currentTime);

  void EventHandler();

  void NotifyDMAEnableChanged(bool new_dma_enable_state);
  void NotifyBlitterFinished();
  void NotifyCop1lcChanged();

  void Load(ULO line, ULO cycle, int lcNumber);
  void LoadNow(int lcNumber);

  void EndOfFrame();
  void HardReset();
  void EmulationStart();
  void EmulationStop();
  void Startup();
  void Shutdown();

  CycleExactCopper(Scheduler *scheduler, Blitter *blitter, BitplaneRegisters *bitplaneRegisters);
  ~CycleExactCopper() = default;
};

extern CycleExactCopper cycle_exact_copper;
