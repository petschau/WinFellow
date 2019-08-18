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

#pragma once

#include "fellow/api/defs.h"
#include "fellow/debug/log/DIWYLogEntry.h"

enum class DIWYStates
{
  DIWY_STATE_WAITING_FOR_START_LINE = 0,
  DIWY_STATE_WAITING_FOR_STOP_LINE = 1
};

struct DIWYState
{
  DIWYStates State;
  SHResTimestamp ChangeTime;

  void Set(DIWYStates state, const SHResTimestamp &timestamp);
  DIWYState(DIWYStates state, const SHResTimestamp &timestamp);
};

class DIWYStateMachine
{
private:
  DIWYState _currentState;
  DIWYState _nextState;

  ULO _startLine;
  ULO _stopLine;
  static const ULO WaitForever = 99999;

  void AddLogEntry(DIWYLogReasons reason, const SHResTimestamp &timestamp) const;

  void InitializeStateForStartOfFrame();
  void MakeNextStateCurrent();
  void MakeNextStateCurrentIfPositionMatch(const SHResTimestamp &currentPosition);
  void FindNextStateChange(const SHResTimestamp &startPosition);
  void SetupEvent() const;

  static void MaybeWrapToNextLine(ULO &nextLine, ULO &nextCycle);

  void Reset();

public:
  void SetStartLine(ULO startLine);
  ULO GetStartLine() const;
  void SetStopLine(ULO stopLine);
  ULO GetStopLine() const;
  DIWYState &GetCurrentState();
  DIWYState &GetNextState();

  bool IsVisible() const;
  void NotifyDIWStrtChanged(ULO startLine);
  void NotifyDIWStopChanged(ULO stopLine);

  static void HandleEvent();
  void Handler();

  void SoftReset();
  void HardReset();
  void EndOfFrame();
  void EmulationStart();
  void EmulationStop();
  void Startup();
  void Shutdown();

  DIWYStateMachine();
  ~DIWYStateMachine() = default;
};

extern DIWYStateMachine diwy_state_machine;
