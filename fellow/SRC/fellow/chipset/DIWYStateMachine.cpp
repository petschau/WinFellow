/*=========================================================================*/
/* Fellow                                                                  */
/* DIWY State Machine                                                      */
/*                                                                         */
/* Authors: Petter Schau                                                   */
/*                                                                         */
/*                                                                         */
/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software Foundation, */
/* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          */
/*=========================================================================*/

#include "fellow/chipset/DIWYStateMachine.h"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/BitplaneShifter.h"
#include "fellow/debug/log/DebugLogHandler.h"
#include "fellow/scheduler/Scheduler.h"

void DIWYState::Set(DIWYStates state, const SHResTimestamp &timestamp)
{
  State = state;
  ChangeTime = timestamp;
}

DIWYState::DIWYState(DIWYStates state, const SHResTimestamp &timestamp) : State(state), ChangeTime(timestamp)
{
}

void DIWYStateMachine::AddLogEntry(DIWYLogReasons reason, const SHResTimestamp &timestamp) const
{
  if (DebugLog.Enabled)
  {
    DebugLog.AddDIWYLogEntry(DebugLogSource::DIWY_ACTION, _scheduler->GetRasterFrameCount(), timestamp, reason, _startLine, _stopLine);
  }
}

void DIWYStateMachine::SetStartLine(ULO startLine)
{
  _startLine = startLine;
}

ULO DIWYStateMachine::GetStartLine() const
{
  return _startLine;
}

void DIWYStateMachine::SetStopLine(ULO stopLine)
{
  _stopLine = stopLine;
}

ULO DIWYStateMachine::GetStopLine() const
{
  return _stopLine;
}

DIWYState &DIWYStateMachine::GetCurrentState()
{
  return _currentState;
}

DIWYState &DIWYStateMachine::GetNextState()
{
  return _nextState;
}

void DIWYStateMachine::MakeNextStateCurrent()
{
  _currentState = _nextState;
}

void DIWYStateMachine::MaybeWrapToNextLine(ULO &nextLine, ULO &nextCycle)
{
  const ULO cyclesInLine = _scheduler->GetCyclesInLine();

  if (nextCycle >= cyclesInLine)
  {
    nextCycle -= cyclesInLine;
    nextLine++;
  }
}

void DIWYStateMachine::FindNextStateChange(const SHResTimestamp &startPosition)
{
  switch (_currentState.State)
  {
    case DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE:
      if (_startLine > startPosition.Line)
      {
        // In the future
        _nextState.Set(DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE, SHResTimestamp(_startLine, 0));
      }
      else if (_startLine < startPosition.Line)
      {
        // In the past, will never be seen
        _nextState.Set(DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE, SHResTimestamp(WaitForever, 0));
      }
      else if (_currentState.ChangeTime.Pixel < startPosition.Pixel)
      {
        // Later on this line, ie. now
        _nextState.Set(DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE, startPosition);
      }
      break;
    case DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE:
      if (_stopLine > startPosition.Line)
      {
        // In the future
        _nextState.Set(DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE, SHResTimestamp(_stopLine, 0));
      }
      else if (_stopLine < startPosition.Line)
      {
        // In the past, will never be seen
        _nextState.Set(DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE, SHResTimestamp(WaitForever, 0));
      }
      else if (_currentState.ChangeTime.Pixel < startPosition.Pixel)
      {
        // Later on this line, ie. now
        _nextState.Set(DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE, startPosition);
      }
      break;
  }
}

void DIWYStateMachine::MakeNextStateCurrentIfPositionMatch(const SHResTimestamp &currentPosition)
{
  if (_nextState.ChangeTime == currentPosition)
  {
    bitplane_shifter.Flush();
    MakeNextStateCurrent();
  }
}

bool DIWYStateMachine::IsVisible() const
{
  return (_currentState.State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE) && (_scheduler->GetRasterY() >= _scheduler->GetVerticalBlankEnd());
}

void DIWYStateMachine::NotifyDIWStrtChanged(ULO newStartLine)
{
  const SHResTimestamp &currentPosition = _scheduler->GetSHResTimestamp();
  MakeNextStateCurrentIfPositionMatch(currentPosition);

  _startLine = newStartLine;

  FindNextStateChange(SHResTimestamp(currentPosition, _scheduler->GetCycleFromCycle280ns(1)));
  SetupEvent();

  AddLogEntry(DIWYLogReasons::DIWY_start_changed, currentPosition);
}

void DIWYStateMachine::NotifyDIWStopChanged(ULO stopLine)
{
  const SHResTimestamp &currentPosition = _scheduler->GetSHResTimestamp();
  MakeNextStateCurrentIfPositionMatch(currentPosition);

  _stopLine = stopLine;

  FindNextStateChange(SHResTimestamp(currentPosition, _scheduler->GetCycleFromCycle280ns(1)));
  SetupEvent();

  AddLogEntry(DIWYLogReasons::DIWY_stop_changed, currentPosition);
}

void DIWYStateMachine::InitializeStateForStartOfFrame()
{
  _currentState.Set(DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE, SHResTimestamp(0, 0));
  FindNextStateChange(SHResTimestamp(0, _scheduler->GetCycleFromCycle280ns(1)));
  SetupEvent();

  AddLogEntry(DIWYLogReasons::DIWY_new_frame, SHResTimestamp(0, 0));
}

void DIWYStateMachine::SetupEvent() const
{
  if (diwyEvent.IsEnabled())
  {
    // Event may be rescheduled from outside the handler
    _scheduler->RemoveEvent(&diwyEvent);
    diwyEvent.Disable();
  }

  if (_nextState.ChangeTime.Line != WaitForever)
  {
    diwyEvent.cycle = _nextState.ChangeTime.ToBaseClockTimestamp();
    _scheduler->InsertEvent(&diwyEvent);
  }
}

void DIWYStateMachine::HandleEvent()
{
  diwy_state_machine.Handler();
}

void DIWYStateMachine::Handler()
{
  diwyEvent.Disable();

  const SHResTimestamp &currentPosition = _scheduler->GetSHResTimestamp();

  F_ASSERT(currentPosition == _nextState.ChangeTime);
  AddLogEntry((_currentState.State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE) ? DIWYLogReasons::DIWY_start_found : DIWYLogReasons::DIWY_stop_found, _nextState.ChangeTime);

  MakeNextStateCurrent();
  FindNextStateChange(SHResTimestamp(currentPosition, _scheduler->GetCycleFromCycle280ns(1)));
  SetupEvent();
}

void DIWYStateMachine::Reset()
{
  _startLine = 0;
  _stopLine = 0;

  InitializeStateForStartOfFrame();
}

/* Fellow events */

void DIWYStateMachine::EndOfFrame()
{
  InitializeStateForStartOfFrame();
}

void DIWYStateMachine::SoftReset()
{
  Reset();
}

void DIWYStateMachine::HardReset()
{
  Reset();
}

void DIWYStateMachine::EmulationStart()
{
}

void DIWYStateMachine::EmulationStop()
{
}

void DIWYStateMachine::Startup()
{
}

void DIWYStateMachine::Shutdown()
{
}

DIWYStateMachine::DIWYStateMachine(Scheduler *scheduler)
  : _scheduler(scheduler),
  _currentState(DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE, SHResTimestamp(0, 0)),
    _nextState(DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE, SHResTimestamp(0, scheduler->GetCycleFromCycle280ns(2))),
    _startLine(0),
    _stopLine(0)
{
}
