#include "fellow/chipset/DDFStateMachine.h"

#include "fellow/chipset/BitplaneDMA.h"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/scheduler/Scheduler.h"

const char *DDFStateMachine::DDFStateNames[2] = {"WAIT_FOR_START", "WAIT_FOR_STOP"};

bool DDFStateMachine::IsNextFetchLast(ULO cycle280ns) const
{
  return _endSeen || ((cycle280ns & 0xfffffffe) == _lastFetchStart);
}

bool DDFStateMachine::IsFetchEnabled() const
{
  return _fetchEnabled || _endSeen;
}

void DDFStateMachine::SetState(DDFStates newState, ULO baseClockTimestamp)
{
  if (ddfEvent.IsEnabled())
  {
    _scheduler->RemoveEvent(&ddfEvent);
  }

  ddfEvent.cycle = baseClockTimestamp;
  _state = newState;
  _scheduler->InsertEvent(&ddfEvent);
}

void DDFStateMachine::EvaluateStateWaitForStart(const ChipBusTimestamp &currentTime)
{
  if (_firstFetchStart == currentTime.GetCycle())
  {
    // Change state to DDF_WAIT_FOR_STOP
    if (_lastFetchStart < ChipBusTimestamp::GetCyclesInLine())
    {
      ULO baseClockTimestamp = (_lastFetchStart > currentTime.GetCycle()) ? ChipBusTimestamp::ToBaseClockTimestamp(currentTime.GetLine(), _lastFetchStart)
                                                                          : ChipBusTimestamp::ToBaseClockTimestamp(currentTime.GetLine() + 1, _lastFetchStart);

      SetState(DDFStates::DDF_WAIT_FOR_STOP, baseClockTimestamp);
    }

    if (_previousFetchLine != currentTime.GetLine() || !_startFetchOncePerLine)
    {
      _endSeen = false;
      _fetchEnabled = true;
      _previousFetchLine = currentTime.GetLine();

      _bitplaneDMA->StartFetchProgram(currentTime);
    }
  }
  else
  {
    // Continue DDF_WAIT_FOR_START
    if (_firstFetchStart < ChipBusTimestamp::GetCyclesInLine())
    {
      ULO baseClockTimestamp = (_firstFetchStart > currentTime.GetCycle()) ? ChipBusTimestamp::ToBaseClockTimestamp(currentTime.GetLine(), _firstFetchStart)
                                                                           : ChipBusTimestamp::ToBaseClockTimestamp(currentTime.GetLine() + 1, _firstFetchStart);

      SetState(DDFStates::DDF_WAIT_FOR_START, baseClockTimestamp);
    }
  }
}

void DDFStateMachine::EvaluateStateWaitForStop(const ChipBusTimestamp &currentTime)
{
  if (_lastFetchStart == currentTime.GetCycle())
  {
    // Change state to DDF_WAIT_FOR_START
    if (_firstFetchStart < ChipBusTimestamp::GetCyclesInLine())
    {
      ULO transitionTime = (_firstFetchStart > currentTime.GetCycle()) ? ChipBusTimestamp::ToBaseClockTimestamp(currentTime.GetLine(), _firstFetchStart)
                                                                       : ChipBusTimestamp::ToBaseClockTimestamp(currentTime.GetLine() + 1, _firstFetchStart);

      SetState(DDFStates::DDF_WAIT_FOR_START, transitionTime);
    }

    _endSeen = true;
    _fetchEnabled = false;
  }
  else
  {
    ULO transitionTime = (_lastFetchStart > currentTime.GetCycle()) ? ChipBusTimestamp::ToBaseClockTimestamp(currentTime.GetLine(), _lastFetchStart)
                                                                    : ChipBusTimestamp::ToBaseClockTimestamp(currentTime.GetLine() + 1, _lastFetchStart);

    SetState(DDFStates::DDF_WAIT_FOR_STOP, transitionTime);
  }
}

void DDFStateMachine::ChangedValue()
{
  const ChipBusTimestamp &currentTime = _scheduler->GetChipBusTimestamp();

  if (currentTime.GetCycle() == _lastFetchStart && _state == DDFStates::DDF_WAIT_FOR_STOP && ddfEvent.IsEnabled())
  {
    // A value that DDF state depends on was changed on the same cycle that a DDF state transition was scheduled
    // Evaluate the DDF transition first using old values and then reevaluate with new values

    // Does not seem to happen. (It does happen)
    F_ASSERT(false);
  }

  _firstFetchStart = _bitplaneRegisters->ddfstrt;
  _lastFetchStart = _bitplaneRegisters->ddfstop;

  EvaluateState(currentTime);
}

void DDFStateMachine::EvaluateState(const ChipBusTimestamp &currentTime)
{
  switch (_state)
  {
    case DDFStates::DDF_WAIT_FOR_START: EvaluateStateWaitForStart(currentTime); break;
    case DDFStates::DDF_WAIT_FOR_STOP: EvaluateStateWaitForStop(currentTime); break;
  }
}

void DDFStateMachine::Handler()
{
  ddfEvent.Disable();
  EvaluateState(_scheduler->GetChipBusTimestamp());
}

void DDFStateMachine::InitializeForNewFrame()
{
  SetState(DDFStates::DDF_WAIT_FOR_START, ChipBusTimestamp::ToBaseClockTimestamp(_scheduler->GetVerticalBlankEnd(), _firstFetchStart));
  _fetchEnabled = false;
  _endSeen = false;
  _previousFetchLine = 0;
}

void DDFStateMachine::Reset()
{
  InitializeForNewFrame();

  // Refresh config
  _startFetchOncePerLine = !chipsetGetECS();
}

/* Fellow events */

void DDFStateMachine::EndOfFrame()
{
  InitializeForNewFrame();
}

void DDFStateMachine::SoftReset()
{
  Reset();
}

void DDFStateMachine::HardReset()
{
  Reset();
}

void DDFStateMachine::EmulationStart()
{
}

void DDFStateMachine::EmulationStop()
{
}

void DDFStateMachine::Startup()
{
}

void DDFStateMachine::Shutdown()
{
}

DDFStateMachine::DDFStateMachine(Scheduler *scheduler, BitplaneRegisters *bitplaneRegisters, BitplaneDMA *bitplaneDMA)
  : _scheduler(scheduler), _bitplaneRegisters(bitplaneRegisters), _bitplaneDMA(bitplaneDMA)
{
}