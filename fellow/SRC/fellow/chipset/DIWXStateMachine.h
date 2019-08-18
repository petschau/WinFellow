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

#pragma once

#include "fellow/api/defs.h"
#include "fellow/time/Timestamps.h"
#include "fellow/debug/log/DIWXLogEntry.h"

enum class DIWXStates
{
  DIWX_STATE_WAITING_FOR_START_POSITION = 0,
  DIWX_STATE_WAITING_FOR_STOP_POSITION = 1
};

struct DIWXState
{
  DIWXStates State;
  SHResTimestamp ChangeTime;
  bool OutOfBounds;

  void Set(DIWXStates state, const SHResTimestamp &changeTime);
};

class DIWXStateMachine
{
private:
  DIWXState _currentState;
  DIWXState _nextState;

  void InitializeNewFrame();
  void MakeNextStateCurrent();
  void MakeNextStateCurrentIfPositionsMatch(const SHResTimestamp &currentPosition);
  SHResTimestamp GetEventChangeTimestamp(ULO line, ULO pixel);
  SHResTimestamp FindNextStartPositionMatch(const SHResTimestamp &currentPosition, ULO startPosition);
  SHResTimestamp FindNextStopPositionMatch(const SHResTimestamp &currentPosition, ULO stopPosition);
  void FindNextStateChange(const SHResTimestamp &startPosition);
  void ScheduleEvent() const;

  void AddLogEntry(DIWXLogReasons reason, const SHResTimestamp &timestamp) const;

public:
  void NotifyDIWChanged();
  bool IsVisible() const;

  static void HandleEvent();

  void Handle();

  void SoftReset();
  void HardReset();
  void EndOfFrame();
  void EmulationStart();
  void EmulationStop();
  void Startup();
  void Shutdown();

  DIWXStateMachine() = default;
  ~DIWXStateMachine() = default;
};

extern DIWXStateMachine diwx_state_machine;
