#include "fellow/api/defs.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/chipset/CopperUtility.h"
#include "fellow/chipset/CopperRegisters.h"
#include "fellow/chipset/CycleExactCopper.h"
#include "fellow/chipset/DMAController.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/memory/Memory.h"
#include "fellow/chipset/Blitter.h"
#include "fellow/chipset/BitplaneRegisters.h"

void CopperBeamPosition::AddAndMakeEven(ULO offset)
{
  Cycle += offset;

  if (Cycle >= MaxCycle)
  {
    Cycle -= MaxCycle;

    Line++;
    if (Cycle & 1)
    {
      Cycle++;
    }
  }

  F_ASSERT((Cycle & 1) == 0);
  F_ASSERT(Cycle < MaxCycle);
}

void CopperBeamPosition::Initialize(ULO line, ULO cycle, ULO maxCycleInLines)
{
  Line = line;
  Cycle = cycle;
  MaxCycle = maxCycleInLines;
}

CopperBeamPosition::CopperBeamPosition() : MaxCycle(0), Line(0), Cycle(0)
{
}

bool CopperWaitParameters::IsInsideVerticalBounds(ULO line) const
{
  return line < LinesInFrame;
}

bool CopperWaitParameters::IsInsideHorizontalBounds(ULO cycle) const
{
  return cycle <= MaxCycle;
}

int CopperWaitParameters::CompareVertical(ULO line) const
{
  return ((int)(line & VerticalPositionCompareEnable)) - ((int)(VerticalBeamPosition & VerticalPositionCompareEnable));
}

int CopperWaitParameters::CompareHorizontal(ULO cycle) const
{
  return ((int)(cycle & HorizontalPositionCompareEnable)) - ((int)(HorizontalBeamPosition & HorizontalPositionCompareEnable));
}

bool CopperWaitParameters::TryGetNextHorizontalBeamMatch(CopperBeamPosition &testPosition) const
{
  ULO testCycle = testPosition.Cycle;

  while (IsInsideHorizontalBounds(testCycle) && CompareHorizontal(testCycle) < 0)
  {
    testCycle += 2;
  }

  if (IsInsideHorizontalBounds(testCycle))
  {
    testPosition.Cycle = testCycle;
    return true;
  }

  return false;
}

bool CopperWaitParameters::TryGetNextBeamMatchOnCurrentLine(CopperBeamPosition &testPosition) const
{
  int verticalComparison = CompareVertical(testPosition.Line);

  // Already passed line or exact vertical match and horizontal match later on the line
  return (verticalComparison > 0) || (verticalComparison == 0 && TryGetNextHorizontalBeamMatch(testPosition));
}

bool CopperWaitParameters::Compare(const CopperBeamPosition &testPosition) const
{
  int verticalComparison = CompareVertical(testPosition.Line);

  // Already passed line or exact vertical match and passed horizontal
  return (verticalComparison > 0) || (verticalComparison == 0 && CompareHorizontal(testPosition.Cycle) >= 0);
}

bool CopperWaitParameters::TryGetNextBeamMatchOnLaterLines(CopperBeamPosition &testPosition) const
{
  bool hasAnyHorizontalMatch = TryGetNextHorizontalBeamMatch(testPosition);
  if (hasAnyHorizontalMatch)
  {
    // Match exists for the horizontal position. The vertical position can be either larger or equal for the wait to end, whichever comes first
    while (IsInsideVerticalBounds(testPosition.Line) && CompareVertical(testPosition.Line) < 0)
    {
      testPosition.Line++;
    }

    if (CompareVertical(testPosition.Line) < 0)
    {
      // Match on vertical alone
      testPosition.Cycle = 0;
    }
  }
  else
  {
    // No match exists for the horizontal position. The vertical position must be larger for the wait to end
    while (IsInsideVerticalBounds(testPosition.Line) && CompareVertical(testPosition.Line) <= 0)
    {
      testPosition.Line++;
    }
  }

  return IsInsideVerticalBounds(testPosition.Line);
}

bool CopperWaitParameters::TryGetNextBeamMatch(CopperBeamPosition &testPosition) const
{
  bool hasMatchOnCurrentLine = TryGetNextBeamMatchOnCurrentLine(testPosition);
  if (hasMatchOnCurrentLine)
  {
    return true;
  }

  // Try later lines
  testPosition.Line++;
  testPosition.Cycle = 0;

  return TryGetNextBeamMatchOnLaterLines(testPosition);
}

void CopperWaitParameters::Initialize(UWO ir1, UWO ir2, ULO linesInFrame, ULO maxCycle)
{
  VerticalBeamPosition = ir1 >> 8;
  HorizontalBeamPosition = ir1 & 0xfe;
  VerticalPositionCompareEnable = (ir2 >> 8) | 0x80;
  HorizontalPositionCompareEnable = ir2 & 0xfe;
  BlitterFinishedDisable = CopperUtility::HasBlitterFinishedDisable(ir2);
  IsWaitingForBlitter = false;
  LinesInFrame = linesInFrame;
  MaxCycle = maxCycle;
}

CopperWaitParameters::CopperWaitParameters()
  : VerticalBeamPosition(0),
    HorizontalBeamPosition(0),
    VerticalPositionCompareEnable(0),
    HorizontalPositionCompareEnable(0),
    LinesInFrame(0),
    MaxCycle(0),
    BlitterFinishedDisable(false),
    IsWaitingForBlitter(false)
{
}

void CycleExactCopper::ScheduleDMARead(ULO offset)
{
  if (_bitplaneRegisters->IsCopperDMAEnabled())
  {
    _currentBeamPosition.AddAndMakeEven(offset);
    dma_controller.ScheduleCopperDMARead(ChipBusTimestamp(_currentBeamPosition.Line, _currentBeamPosition.Cycle), copper_registers.PC);
  }
}

void CycleExactCopper::ScheduleDMANull(ULO offset)
{
  if (_bitplaneRegisters->IsCopperDMAEnabled())
  {
    _currentBeamPosition.AddAndMakeEven(offset);
    dma_controller.ScheduleCopperDMANull(ChipBusTimestamp(_currentBeamPosition.Line, _currentBeamPosition.Cycle));
  }
}

void CycleExactCopper::ScheduleEvent(ULO line, ULO cycle)
{
  if (copperEvent.IsEnabled())
  {
    // Why would this ever happen?
    F_ASSERT(false);
    _scheduler->RemoveEvent(&copperEvent);
  }

  copperEvent.cycle = _scheduler->GetCycleFromCycle280ns(line * _scheduler->Get280nsCyclesInLine() + cycle);
  _scheduler->InsertEvent(&copperEvent);
}

void CycleExactCopper::ExecuteStep(const ChipBusTimestamp &currentTime, UWO value)
{
  F_ASSERT((currentTime.GetCycle() & 1) == 0);

  _currentBeamPosition.Initialize(currentTime.GetLine(), currentTime.GetCycle(), _scheduler->Get280nsCyclesInLine());

  switch (_nextStep)
  {
    case CopperStep_Halt: StepHalt(); break;
    case CopperStep_Read_IR1: StepReadIR1(value); break;
    case CopperStep_Read_IR2: StepReadIR2(value); break;
    case CopperStep_Move_IR2: StepMoveIR2(value); break;
    case CopperStep_Wait_Setup: StepWaitSetup(); break;
    case CopperStep_Wait_End: StepWaitEnd(); break;
    case CopperStep_Skip: StepSkip(); break;
    case CopperStep_Load_Cycle1: StepLoadCycle1(); break;
    case CopperStep_Load_Cycle2: StepLoadCycle2(); break;
    default: F_ASSERT(false); break;
  }
}

void CycleExactCopper::StepHalt()
{
  F_ASSERT(false);
}

void CycleExactCopper::StepReadIR1(UWO value)
{
  _ir1 = value;
  IncreasePC();
  _nextStep = CopperUtility::IsMove(_ir1) ? CopperStep_Move_IR2 : CopperStep_Read_IR2;
  ScheduleDMARead(2);
}

void CycleExactCopper::StepReadIR2(UWO value)
{
  _ir2 = value;
  IncreasePC();
  _nextStep = CopperUtility::IsWait(_ir2) ? CopperStep_Wait_Setup : CopperStep_Skip;
  _currentBeamPosition.AddAndMakeEven(2);
  if (_currentBeamPosition.Cycle == 0xe2)
  {
    _currentBeamPosition.AddAndMakeEven(1);
  }
  ScheduleEvent(_currentBeamPosition.Line, _currentBeamPosition.Cycle);
}

void CycleExactCopper::StepMoveIR2(UWO value)
{
  ULO regno = CopperUtility::GetRegisterNumber(_ir1);
  if (!CopperUtility::IsRegisterAllowed(regno))
  {
    _nextStep = CopperStep_Halt;
    return;
  }

  _ir2 = value;
  IncreasePC();
  if (!_skip)
  {
    memory_iobank_write[regno >> 1](_ir2, regno);
  }

  if ((regno != 0x88 && regno != 0x8a) || _skip) // Don't schedule next step if we jumped
  {
    SetupStepReadIR1();
  }

  _skip = false;
}

void CycleExactCopper::StepWaitSetup()
{
  ULO maxCycle = _scheduler->Get280nsCyclesInLine();
  CopperBeamPosition wakeupPosition(_currentBeamPosition);
  wakeupPosition.AddAndMakeEven(2);
  _waitParameters.Initialize(_ir1, _ir2, _scheduler->GetLinesInFrame(), maxCycle);

  _nextStep = CopperStep_Wait_End;
  bool hasLegalBeamMatch = _waitParameters.TryGetNextBeamMatch(wakeupPosition);
  if (hasLegalBeamMatch)
  {
    // First possible wait wakeup at waitPosition
    // A step that is not triggered by callback from DMAController
    if (wakeupPosition.Cycle == 0xe2)
    {
      // Can't schedule an instruction at 0xe2
      wakeupPosition.AddAndMakeEven(1);
    }

    // Schedule a wakeup step two cycles early, unless it is immediately or we are at the beginning of the line
    if ((wakeupPosition.Line != _currentBeamPosition.Line || wakeupPosition.Cycle != (_currentBeamPosition.Cycle + 2)) && wakeupPosition.Cycle != 0)
    {
      wakeupPosition.Cycle -= 2;
    }

    if (wakeupPosition.Cycle == 0xe0)
    {
      wakeupPosition.Cycle = 0;
      wakeupPosition.Line++;
    }

    ScheduleEvent(wakeupPosition.Line, wakeupPosition.Cycle);
  }
  // else No wakeup inside the frame. Copper is not halted, just waiting forever.
}

void CycleExactCopper::StepWaitEnd()
{
  if (!_waitParameters.BlitterFinishedDisable && _blitter->IsStarted())
  {
    _waitParameters.IsWaitingForBlitter = true;
  }
  else
  {
    SetupStepReadIR1();
  }
}

void CycleExactCopper::StepSkip()
{
  ULO maxCycle = _scheduler->Get280nsCyclesInLine();
  _waitParameters.Initialize(_ir1, _ir2, _scheduler->GetLinesInFrame(), maxCycle);
  _skip = _waitParameters.Compare(_currentBeamPosition) && (_waitParameters.BlitterFinishedDisable || !_blitter->IsStarted());
  SetupStepReadIR1();
}

void CycleExactCopper::StepLoadCycle1()
{
  _nextStep = CopperStep_Load_Cycle2;
  ScheduleDMANull(2);
}

void CycleExactCopper::StepLoadCycle2()
{
  copper_registers.PC = (_lcNumberToBeLoaded == 0) ? copper_registers.cop1lc : copper_registers.cop2lc;
  SetupStepReadIR1();
}

void CycleExactCopper::SetupStepReadIR1()
{
  copper_registers.InstructionStart = copper_registers.PC;
  _nextStep = CopperStep_Read_IR1;
  ScheduleDMARead(2);
}

void CycleExactCopper::IncreasePC()
{
  copper_registers.PC = chipsetMaskPtr(copper_registers.PC + 2);
}

void CycleExactCopper::DMAReadCallback(const ChipBusTimestamp &currentTime, UWO value)
{
  cycle_exact_copper.ExecuteStep(currentTime, value);
}

void CycleExactCopper::DMANullCallback(const ChipBusTimestamp &currentTime)
{
  cycle_exact_copper.ExecuteStep(currentTime, 0);
}

void CycleExactCopper::EventHandler()
{
  copperEvent.Disable();
  ExecuteStep(_scheduler->GetChipBusTimestamp(), 0);
}

void CycleExactCopper::LoadNow(int lcNumber)
{
  Load(_scheduler->GetRasterY(), _scheduler->GetLineCycle280ns(), lcNumber);
}

void CycleExactCopper::Load(ULO line, ULO cycle, int lcNumber)
{
  _currentBeamPosition.Initialize(line, cycle & 0xfffffffe, _scheduler->Get280nsCyclesInLine());

  if (copperEvent.IsEnabled())
  {
    // Copper is waiting or evaluating skip
    _scheduler->RemoveEvent(&copperEvent);
    copperEvent.Disable();
  }

  _lcNumberToBeLoaded = lcNumber;
  _nextStep = CopperStep_Load_Cycle1;
  _skip = false;
  ScheduleDMANull(2);
}

void CycleExactCopper::NotifyDMAEnableChanged(bool enabled)
{
  if (enabled)
  {
    _currentBeamPosition.Initialize(_scheduler->GetRasterY(), _scheduler->GetLineCycle280ns() & 0xfffffffe, _scheduler->Get280nsCyclesInLine());
    switch (_nextStep)
    {
      case CopperStep::CopperStep_Read_IR1:
      case CopperStep::CopperStep_Read_IR2:
      case CopperStep::CopperStep_Move_IR2: ScheduleDMARead(2); break;
      case CopperStep_Load_Cycle1:
      case CopperStep_Load_Cycle2: ScheduleDMANull(2); break;
      default: break;
    }
  }
  else
  {
    dma_controller.RemoveCopperDMA();
  }
}

void CycleExactCopper::NotifyCop1lcChanged()
{
  // TODO: Line based code has a "hack" here. Don't want to transfer it, yet. Examine examples.
}

void CycleExactCopper::NotifyBlitterFinished()
{
  if (_nextStep == CopperStep::CopperStep_Wait_End && _waitParameters.IsWaitingForBlitter)
  {
    // If odd cycle, both this and previous even cycle match
    _currentBeamPosition.Initialize(_scheduler->GetRasterY(), _scheduler->GetLineCycle280ns() & 0xfffffffe, _scheduler->Get280nsCyclesInLine());

    if (_waitParameters.Compare(_currentBeamPosition))
    {
      SetupStepReadIR1();
    }
    else
    {
      // Blitter is finished, but wait no longer compares.
      StepWaitSetup();
    }
  }
}

void CycleExactCopper::EndOfFrame()
{
  Load(0, 0, 0);
}

void CycleExactCopper::HardReset()
{
  Load(0, 0, 0);
}

void CycleExactCopper::EmulationStart()
{
}

void CycleExactCopper::EmulationStop()
{
}

void CycleExactCopper::Startup()
{
}

void CycleExactCopper::Shutdown()
{
}

CycleExactCopper::CycleExactCopper(Scheduler *scheduler, Blitter *blitter, BitplaneRegisters *bitplaneRegisters)
  : _scheduler(scheduler), _blitter(blitter), _bitplaneRegisters(bitplaneRegisters), _skip(false), _nextStep(CopperStep_Halt)
{
}
