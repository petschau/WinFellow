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

#include "defs.h"

#include "Graphics.h"

static STR *DIWXStateNames[2] = {"WAITING_FOR_START_POS",
				 "WAITING_FOR_STOP_POS"};

void DIWXStateMachine::Log(ULO line, ULO cylinder)
{
  if (GraphicsContext.Logger.IsLogEnabled())
  {
    STR msg[256];
    sprintf(msg, "DIWX: %s\n", DIWXStateNames[_state]);
    GraphicsContext.Logger.Log(line, cylinder, msg);
  }
}

ULO DIWXStateMachine::GetStartPosition(void)
{
  return diwxleft;
}

ULO DIWXStateMachine::GetStopPosition(void)
{
  return diwxright;
}

void DIWXStateMachine::SetState(DIWXStates newState, ULO arriveTime)
{
  _queue->Remove(this);
  _state = newState;
  _arriveTime = arriveTime;
  _queue->Insert(this);
}

void DIWXStateMachine::OutputCylindersUntilPreviousCylinder(ULO rasterY, ULO cylinder)
{
  ULO previousCylinder = ((cylinder == 0) ? GraphicsEventQueue::GetCylindersPerLine() : cylinder) - 1;
  ULO outputLine;
  
  if (cylinder != 0)
  {
    outputLine = rasterY;
  }
  else
  {
    if (rasterY == 0) outputLine = busGetLinesInThisFrame() - 1;
    else outputLine = rasterY - 1;
  }

  GraphicsContext.PixelSerializer.OutputCylindersUntil(outputLine, previousCylinder);
}


void DIWXStateMachine::SetStateWaitingForStartPos(ULO rasterY, ULO cylinder)
{
  OutputCylindersUntilPreviousCylinder(rasterY, cylinder);

  ULO start = GetStartPosition();
  if (start <= cylinder)
  {
    // Start is seen on the next line
    rasterY++;
  }
  SetState(DIWX_STATE_WAITING_FOR_START_POS, MakeArriveTime(rasterY, start));
}

void DIWXStateMachine::SetStateWaitingForStopPos(ULO rasterY, ULO cylinder)
{
  OutputCylindersUntilPreviousCylinder(rasterY, cylinder);

  if (GetStopPosition() > _maxValidX)
  {
    // Stop position will never be found, wait beyond end of frame (effectively disabled)
    SetState(DIWX_STATE_WAITING_FOR_STOP_POS, busGetCyclesInThisFrame()*2 + 1);
  }
  else if (GetStopPosition() <= cylinder)
  {
    // Stop position will be found on the next line
    SetState(DIWX_STATE_WAITING_FOR_STOP_POS, MakeArriveTime(rasterY + 1, GetStopPosition()));
  }
  else
  {
    SetState(DIWX_STATE_WAITING_FOR_STOP_POS, MakeArriveTime(rasterY, GetStopPosition()));
  }
}

void DIWXStateMachine::DoStateWaitingForStartPos(ULO rasterY, ULO cylinder)
{
  SetStateWaitingForStopPos(rasterY, cylinder);
}

void DIWXStateMachine::DoStateWaitingForStopPos(ULO rasterY, ULO cylinder)
{
  SetStateWaitingForStartPos(rasterY, cylinder);
}

bool DIWXStateMachine::IsVisible(void)
{
  return (_state == DIWX_STATE_WAITING_FOR_STOP_POS);
}

void DIWXStateMachine::ChangedValue(void)
{
  switch (_state)
  {
    case DIWX_STATE_WAITING_FOR_START_POS:
      SetStateWaitingForStartPos(busGetRasterY(), busGetRasterX()*2);
      break;
    case DIWX_STATE_WAITING_FOR_STOP_POS:
      SetStateWaitingForStopPos(busGetRasterY(), busGetRasterX()*2);
      break;
  }
}

/* Bus event handler */

void DIWXStateMachine::InitializeEvent(GraphicsEventQueue *queue)
{
  _queue = queue;
  SetStateWaitingForStartPos(0, 0);
}

void DIWXStateMachine::Handler(ULO rasterY, ULO cylinder)
{
  Log(rasterY, cylinder);

  switch (_state)
  {
    case DIWX_STATE_WAITING_FOR_START_POS:
      DoStateWaitingForStartPos(rasterY, cylinder);
      break;
    case DIWX_STATE_WAITING_FOR_STOP_POS:
      DoStateWaitingForStopPos(rasterY, cylinder);
      break;
  }
}

/* Fellow events */

void DIWXStateMachine::EndOfFrame(void)
{
  SetStateWaitingForStartPos(0, 0);
}

void DIWXStateMachine::SoftReset(void)
{
}

void DIWXStateMachine::HardReset(void)
{
}

void DIWXStateMachine::EmulationStart(void)
{
}

void DIWXStateMachine::EmulationStop(void)
{
}

void DIWXStateMachine::Startup(void)
{
  _maxValidX = 455;
}

void DIWXStateMachine::Shutdown(void)
{
}
