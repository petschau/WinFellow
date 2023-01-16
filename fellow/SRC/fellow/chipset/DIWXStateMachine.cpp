/*=========================================================================*/
/* Fellow                                                                  */
/* DIWX State Machine                                                      */
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

#include "fellow/chipset/DIWXStateMachine.h"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/BitplaneShifter.h"
#include "fellow/debug/log/DebugLogHandler.h"
#include "fellow/scheduler/Scheduler.h"

static const STR *DIWXStateNames[2] = {"WAITING_FOR_START_POSITION", "WAITING_FOR_STOP_POSITION"};

void DIWXState::Set(DIWXStates state, const SHResTimestamp &changeTime, ULO maxLinesInFrame)
{
  State = state;
  ChangeTime = changeTime;
  OutOfBounds = changeTime.Line >= maxLinesInFrame;
}

void DIWXStateMachine::AddLogEntry(DIWXLogReasons reason, const SHResTimestamp &timestamp) const
{
  if (DebugLog.Enabled)
  {
    DebugLog.AddDIWXLogEntry(
        DebugLogSource::DIWX_ACTION,
        _scheduler->GetRasterFrameCount(),
        timestamp,
        reason,
        _bitplaneRegisters->DiwXStart,
        _bitplaneRegisters->DiwXStop);
  }
}

bool DIWXStateMachine::IsVisible() const
{
  return _currentState.State == DIWXStates::DIWX_STATE_WAITING_FOR_STOP_POSITION;
}

void DIWXStateMachine::MakeNextStateCurrent()
{
  bitplane_shifter.Flush(_nextState.ChangeTime);
  _currentState = _nextState;
}

void DIWXStateMachine::MakeNextStateCurrentIfPositionsMatch(const SHResTimestamp &currentPosition)
{
  if (_nextState.ChangeTime == currentPosition)
  {
    MakeNextStateCurrent();
  }
}

SHResTimestamp DIWXStateMachine::GetEventChangeTimestamp(ULO line, ULO pixel)
{
  ULO changeLine = line;
  ULO changePixel = pixel;

  if (pixel > 0)
  {
    changePixel -= 1;
  }
  else if (line != 0)
  {
    changeLine -= 1;
    changePixel = _scheduler->GetCyclesInLine() - 1;
  }

  return {changeLine, changePixel};
}

SHResTimestamp DIWXStateMachine::FindNextStartPositionMatch(const SHResTimestamp &searchFromPosition, ULO startPosition)
{
  ULO matchLine;

  if (startPosition > searchFromPosition.Pixel)
  {
    // Start will be seen on this line
    matchLine = searchFromPosition.Line;
  }
  else
  {
    // Start will be seen on the next line
    matchLine = searchFromPosition.Line + 1;
  }

  return GetEventChangeTimestamp(matchLine, startPosition);
}

SHResTimestamp DIWXStateMachine::FindNextStopPositionMatch(const SHResTimestamp &searchFromPosition, ULO stopPosition)
{
  ULO matchLine;

  if (stopPosition >= _scheduler->GetCyclesInLine())
  {
    // Stop position will never be seen, set wait value beyond end of frame
    matchLine = _scheduler->GetLinesInFrame();
  }
  else if (stopPosition > searchFromPosition.Pixel)
  {
    // Stop will be seen on this line
    matchLine = searchFromPosition.Line;
  }
  else
  {
    // Stop will be seen on the next line
    matchLine = searchFromPosition.Line + 1;
  }

  return GetEventChangeTimestamp(matchLine, stopPosition);
}

void DIWXStateMachine::FindNextStateChange(const SHResTimestamp &searchFromPosition)
{
  if (_currentState.State == DIWXStates::DIWX_STATE_WAITING_FOR_START_POSITION)
  {
    _nextState.Set(
        DIWXStates::DIWX_STATE_WAITING_FOR_STOP_POSITION,
        FindNextStartPositionMatch(searchFromPosition, _bitplaneRegisters->DiwXStart),
        _scheduler->GetLinesInFrame());
  }
  else
  {
    _nextState.Set(
        DIWXStates::DIWX_STATE_WAITING_FOR_START_POSITION,
        FindNextStopPositionMatch(searchFromPosition, _bitplaneRegisters->DiwXStop),
        _scheduler->GetLinesInFrame());
  }
}

void DIWXStateMachine::ScheduleEvent() const
{
  if (diwxEvent.IsEnabled())
  {
    // Might be responding to a change originating from outside the handler
    _scheduler->RemoveEvent(&diwxEvent);
    diwxEvent.Disable();
  }

  if (!_nextState.OutOfBounds)
  {
    diwxEvent.cycle = _nextState.ChangeTime.ToBaseClockTimestamp();
    _scheduler->InsertEvent(&diwxEvent);
  }
}

void DIWXStateMachine::NotifyDIWChanged()
{
  const SHResTimestamp &currentPosition = _scheduler->GetSHResTimestamp();
  FindNextStateChange(currentPosition);
  ScheduleEvent();

  AddLogEntry(DIWXLogReasons::DIWX_changed, currentPosition);
}

void DIWXStateMachine::Handle()
{
  // All DIWX events are 1 shres pixel too early to simplify graphics pipeline flushes
  MakeNextStateCurrent();
  FindNextStateChange(SHResTimestamp(_scheduler->GetSHResTimestamp(), 1));
  ScheduleEvent();

  AddLogEntry((_currentState.State == DIWXStates::DIWX_STATE_WAITING_FOR_START_POSITION) ? DIWXLogReasons::DIWX_stop_found : DIWXLogReasons::DIWX_start_found, _scheduler->GetSHResTimestamp());
}

void DIWXStateMachine::InitializeNewFrame()
{
  SHResTimestamp currentPosition(0, 0);
  _currentState.Set(DIWXStates::DIWX_STATE_WAITING_FOR_START_POSITION, currentPosition, _scheduler->GetLinesInFrame());
  FindNextStateChange(SHResTimestamp(currentPosition, _scheduler->GetCycleFromCycle280ns(1)));
  ScheduleEvent();

  AddLogEntry(DIWXLogReasons::DIWX_new_frame, currentPosition);
}

/* Fellow events */

void DIWXStateMachine::EndOfFrame()
{
  InitializeNewFrame();
}

void DIWXStateMachine::SoftReset()
{
}

void DIWXStateMachine::HardReset()
{
  InitializeNewFrame();
}

void DIWXStateMachine::EmulationStart()
{
  // Continue with the state we have
}

void DIWXStateMachine::EmulationStop()
{
}

void DIWXStateMachine::Startup()
{
}

void DIWXStateMachine::Shutdown()
{
}

DIWXStateMachine::DIWXStateMachine(Scheduler *scheduler, BitplaneRegisters *bitplaneRegisters)
  : _scheduler(scheduler), _bitplaneRegisters(bitplaneRegisters)
{
}
