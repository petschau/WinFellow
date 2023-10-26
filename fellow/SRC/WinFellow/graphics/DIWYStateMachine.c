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

#include "bus.h"
#include "graph.h"

#include "Graphics.h"

static const char *DIWYStateNames[2] = {"WAITING_FOR_START_LINE", "WAITING_FOR_STOP_LINE"};

void DIWYStateMachine::Log(uint32_t line, uint32_t cylinder)
{
  if (GraphicsContext.Logger.IsLogEnabled())
  {
    char msg[256];
    sprintf(msg, "DIWY: %s\n", DIWYStateNames[_state]);
    GraphicsContext.Logger.Log(line, cylinder, msg);
  }
}

uint32_t DIWYStateMachine::GetStartLine()
{
  return (diwytop < _minValidY) ? _minValidY : diwytop;
}

uint32_t DIWYStateMachine::GetStopLine()
{
  return diwybottom;
}

void DIWYStateMachine::SetState(DIWYStates newState, uint32_t arriveTime)
{
  _queue->Remove(this);
  _state = newState;
  _arriveTime = arriveTime;
  _queue->Insert(this);
}

void DIWYStateMachine::SetStateWaitingForStartLine(uint32_t rasterY)
{
  if ((GetStartLine() > _maxValidY) || (GetStartLine() < rasterY))
  {
    // Start will not be seen (again) in this frame, wait until end of frame (effectively disabled)
    SetState(DIWY_STATE_WAITING_FOR_START_LINE, MakeArriveTime(GraphicsEventQueue::GetCylindersPerLine() + 1, 0));
  }
  else
  {
    // This should not affect the current line
    SetState(DIWY_STATE_WAITING_FOR_START_LINE, MakeArriveTime(GetStartLine(), 0));
  }
}

void DIWYStateMachine::SetStateWaitingForStopLine(uint32_t rasterY)
{
  if ((GetStopLine() > _maxValidY) || (GetStopLine() <= rasterY))
  {
    // Start will not be seen (again) in this frame, wait until end of frame (effectively enabled until end of frame)
    SetState(DIWY_STATE_WAITING_FOR_STOP_LINE, MakeArriveTime(GraphicsEventQueue::GetCylindersPerLine() + 1, 0));
  }
  else
  {
    SetState(DIWY_STATE_WAITING_FOR_STOP_LINE, MakeArriveTime(GetStopLine(), 0));
  }
}

void DIWYStateMachine::DoStateWaitingForStartLine(uint32_t rasterY)
{
  SetStateWaitingForStopLine(rasterY);
}

void DIWYStateMachine::DoStateWaitingForStopLine(uint32_t rasterY)
{
  SetStateWaitingForStartLine(rasterY);
}

bool DIWYStateMachine::IsVisible()
{
  return _state == DIWY_STATE_WAITING_FOR_STOP_LINE;
}

void DIWYStateMachine::ChangedValue()
{
  switch (_state)
  {
    case DIWY_STATE_WAITING_FOR_START_LINE: SetStateWaitingForStartLine(busGetRasterY()); break;
    case DIWY_STATE_WAITING_FOR_STOP_LINE: SetStateWaitingForStopLine(busGetRasterY()); break;
  }
}

/* Bus event handler */

void DIWYStateMachine::InitializeEvent(GraphicsEventQueue *queue)
{
  _queue = queue;
  SetStateWaitingForStartLine(0);
}

void DIWYStateMachine::Handler(uint32_t rasterY, uint32_t cylinder)
{
  Log(rasterY, cylinder);

  switch (_state)
  {
    case DIWY_STATE_WAITING_FOR_START_LINE: DoStateWaitingForStartLine(rasterY); break;
    case DIWY_STATE_WAITING_FOR_STOP_LINE: DoStateWaitingForStopLine(rasterY); break;
  }
}

/* Fellow events */

void DIWYStateMachine::EndOfFrame()
{
  SetStateWaitingForStartLine(0);
}

void DIWYStateMachine::SoftReset()
{
}

void DIWYStateMachine::HardReset()
{
}

void DIWYStateMachine::EmulationStart()
{
}

void DIWYStateMachine::EmulationStop()
{
}

void DIWYStateMachine::Startup()
{
  _minValidY = 26;
  _maxValidY = busGetLinesInThisFrame();
}

void DIWYStateMachine::Shutdown()
{
}
