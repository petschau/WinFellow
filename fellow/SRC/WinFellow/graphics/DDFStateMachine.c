/*=========================================================================*/
/* Fellow                                                                  */
/* DDF State Machine                                                       */
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

using namespace CustomChipset;

static STR *DDFStateNames[2] = {"WAITING_FOR_FIRST_FETCH",
				"WAITING_FOR_NEXT_FETCH"};

void DDFStateMachine::Log(ULO line, ULO cylinder)
{
  if (GraphicsContext.Logger.IsLogEnabled())
  {
    STR msg[256];
    sprintf(msg, "DDF: %s\n", DDFStateNames[_state]);
    GraphicsContext.Logger.Log(line, cylinder, msg);
  }
}

ULO DDFStateMachine::GetStartPosition(void)
{
  return (ddfstrt < _minValidX) ? _minValidX : ddfstrt;
}

ULO DDFStateMachine::GetStopPosition(void)
{
  return (ddfstop < _minValidX) ? _minValidX : ddfstop;
}

ULO DDFStateMachine::GetFetchSize(void)
{
  return (_core.RegisterUtility.IsHiresEnabled()) ? 4 : 8;
}

void DDFStateMachine::SetState(DDFStates newState, ULO arriveTime)
{
  _queue->Remove(this);
  _state = newState;
  _arriveTime = arriveTime;
  _queue->Insert(this);
}

void DDFStateMachine::SetStateWaitingForFirstFetch(ULO rasterY, ULO cylinder)
{
  ULO start = GetStartPosition();
  ULO currentCycle = cylinder / 2;

  if (start == currentCycle)
  {
    SetState(DDF_STATE_WAITING_FOR_NEXT_FETCH, MakeArriveTime(rasterY, cylinder + GetFetchSize()*2));
  }
  else if (start > currentCycle)
  {
    // Fetch start will be seen on this line
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(rasterY, start*2));
  }
  else
  {
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(rasterY + 1, start*2));
  }
}

void DDFStateMachine::SetStateWaitingForNextFetch(ULO rasterY, ULO cylinder)
{
  ULO stop = GetStopPosition();
  ULO start = GetStartPosition();
  ULO currentCycle = cylinder / 2;

  if ((stop & 7) != (start & 7)) stop += GetFetchSize();
  
  if (stop > currentCycle)
  {
    // More fetches on this line
    SetState(DDF_STATE_WAITING_FOR_NEXT_FETCH, MakeArriveTime(rasterY, cylinder + GetFetchSize()*2));
  }
  else if (start > currentCycle)
  {
    // Fetch start will be seen on this line
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(rasterY, start*2));
  }
  else
  {
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(rasterY + 1, start*2));
  }
}

void DDFStateMachine::DoStateWaitingForFirstFetch(ULO rasterY, ULO cylinder)
{
  SetStateWaitingForFirstFetch(rasterY, cylinder);
}

void DDFStateMachine::DoStateWaitingForNextFetch(ULO rasterY, ULO cylinder)
{
  SetStateWaitingForNextFetch(rasterY, cylinder);
}

bool DDFStateMachine::CanRead(void)
{
  return (_state == DDF_STATE_WAITING_FOR_NEXT_FETCH) 
    && _core.RegisterUtility.GetEnabledBitplaneCount() > 0 
    && GraphicsContext.DIWYStateMachine.IsVisible();
}

void DDFStateMachine::ChangedValue(void)
{
  ULO rasterY = busGetRasterY();
  if (rasterY < 0x1a)
  {
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(0x1a, GetStartPosition()*2));
    return;
  }

  switch (_state)
  {
    case DDF_STATE_WAITING_FOR_FIRST_FETCH:
      SetStateWaitingForFirstFetch(busGetRasterY(), busGetRasterX()*2);
      break;
    case DDF_STATE_WAITING_FOR_NEXT_FETCH:
      SetStateWaitingForNextFetch(busGetRasterY(), busGetRasterX()*2);
      break;
  }
}

/* Bus event handler */

void DDFStateMachine::InitializeEvent(GraphicsEventQueue *queue)
{
  _queue = queue;
  SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(0x1a, GetStartPosition()*2));
}

void DDFStateMachine::Handler(ULO rasterY, ULO cylinder)
{
  Log(rasterY, cylinder);

  switch (_state)
  {
    case DDF_STATE_WAITING_FOR_FIRST_FETCH:
      DoStateWaitingForFirstFetch(rasterY, cylinder);
      break;
    case DDF_STATE_WAITING_FOR_NEXT_FETCH:
      DoStateWaitingForNextFetch(rasterY, cylinder);
      break;
  }
  if (CanRead())
  {
    GraphicsContext.BitplaneDMA.Start(MakeArriveTime(rasterY, cylinder));
  }
  else
  {
    GraphicsContext.BitplaneDMA.Stop();
  }
}

/* Fellow events */

void DDFStateMachine::EndOfFrame(void)
{
  SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(0x1a, GetStartPosition()*2));
}

void DDFStateMachine::SoftReset(void)
{
}

void DDFStateMachine::HardReset(void)
{
}

void DDFStateMachine::EmulationStart(void)
{
}

void DDFStateMachine::EmulationStop(void)
{
}

void DDFStateMachine::Startup(void)
{
  _minValidX = 0x18;
  _maxValidX = 0xd8;
}

void DDFStateMachine::Shutdown(void)
{
}
