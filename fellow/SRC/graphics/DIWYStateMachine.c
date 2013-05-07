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

#include "defs.h"

#ifdef GRAPH2

#include "bus.h"
#include "graph.h"
#include "fileops.h"

#include "DIWYStateMachine.h"

static STR *DIWYStateNames[2] = {"WAITING_FOR_START_LINE",
				 "WAITING_FOR_STOP_LINE"};

void DIWYStateMachine::Log(void)
{
  if (_enableLog)
  {
    if (_logfile == 0)
    {
      STR filename[MAX_PATH];
      fileopsGetGenericFileName(filename, "WinFellow", "DIWYStateMachine.log");
      _logfile = fopen(filename, "w");
    }
    fprintf(_logfile, "%.16I64X %.3X %.3X %s\n", busGetRasterFrameCount(), busGetRasterY(), busGetRasterX(), DIWYStateNames[_state]);
  }
}

ULO DIWYStateMachine::GetStartLine(void)
{
  return (diwytop < _minValidY) ? _minValidY : diwytop;
}

ULO DIWYStateMachine::GetStopLine(void)
{
  return diwybottom;
}

void DIWYStateMachine::SetState(DIWYStates newState, ULO cycle)
{
  _queue->Remove(this);
  _state = newState;
  _cycle = cycle;
  _queue->Insert(this);
  Log();
}

void DIWYStateMachine::SetStateWaitingForStartLine(ULO rasterY)
{
  if ((GetStartLine() > _maxValidY) || (GetStartLine() < rasterY))
  {
    // Start will not be seen (again) in this frame, wait until end of frame (effectively disabled)
    SetState(DIWY_STATE_WAITING_FOR_START_LINE, BUS_CYCLE_PER_FRAME + 1);
  }
  else
  {
    SetState(DIWY_STATE_WAITING_FOR_START_LINE, GetStartLine()*BUS_CYCLE_PER_LINE);
  }
}

void DIWYStateMachine::SetStateWaitingForStopLine(ULO rasterY)
{
  if ((GetStopLine() > _maxValidY) || (GetStopLine() <= rasterY))
  {
    // Start will not be seen (again) in this frame, wait until end of frame (effectively enabled until end of frame)
    SetState(DIWY_STATE_WAITING_FOR_STOP_LINE, BUS_CYCLE_PER_FRAME + 1);
  }
  else
  {
    SetState(DIWY_STATE_WAITING_FOR_STOP_LINE, GetStopLine()*BUS_CYCLE_PER_LINE);
  }
}

void DIWYStateMachine::DoStateWaitingForStartLine(ULO rasterY)
{
  SetStateWaitingForStopLine(rasterY);
}

void DIWYStateMachine::DoStateWaitingForStopLine(ULO rasterY)
{
  SetStateWaitingForStartLine(rasterY);
}

bool DIWYStateMachine::IsVisible(void)
{
  return _state == DIWY_STATE_WAITING_FOR_STOP_LINE;
}

void DIWYStateMachine::ChangedValue(void)
{
  switch (_state)
  {
    case DIWY_STATE_WAITING_FOR_START_LINE:
      SetStateWaitingForStartLine(busGetRasterY());
      break;
    case DIWY_STATE_WAITING_FOR_STOP_LINE:
      SetStateWaitingForStopLine(busGetRasterY());
      break;
  }
}

/* Bus event handler */

void DIWYStateMachine::InitializeEvent(GraphicsEventQueue *queue)
{
  _queue = queue;
  SetStateWaitingForStartLine(0);
}

void DIWYStateMachine::Handler(ULO rasterY, ULO rasterX)
{
  switch (_state)
  {
    case DIWY_STATE_WAITING_FOR_START_LINE:
      DoStateWaitingForStartLine(rasterY);
      break;
    case DIWY_STATE_WAITING_FOR_STOP_LINE:
      DoStateWaitingForStopLine(rasterY);
      break;
  }
}

/* Fellow events */

void DIWYStateMachine::EndOfFrame(void)
{
  SetStateWaitingForStartLine(0);
}

void DIWYStateMachine::SoftReset(void)
{
}

void DIWYStateMachine::HardReset(void)
{
}

void DIWYStateMachine::EmulationStart(void)
{
}

void DIWYStateMachine::EmulationStop(void)
{
}

void DIWYStateMachine::Startup(void)
{
  _minValidY = 26;
  _maxValidY = BUS_LINES_PER_FRAME;
  _enableLog = false;
  _logfile = 0;
}

void DIWYStateMachine::Shutdown(void)
{
  if (_enableLog && _logfile != 0)
  {
    fclose(_logfile);
  }
}

#endif