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

#include "Defs.h"
#include "BusScheduler.h"
#include "GraphicsPipeline.h"
#include "Graphics.h"

using namespace CustomChipset;

static const char *DDFStateNames[2] = {"WAITING_FOR_FIRST_FETCH", "WAITING_FOR_NEXT_FETCH"};

void DDFStateMachine::Log(uint32_t line, uint32_t cylinder)
{
  if (GraphicsContext.Logger.IsLogEnabled())
  {
    char msg[256];
    sprintf(msg, "DDF: %s\n", DDFStateNames[_state]);
    GraphicsContext.Logger.Log(line, cylinder, msg);
  }
}

uint32_t DDFStateMachine::GetStartPosition()
{
  return (ddfstrt < _minValidX) ? _minValidX : ddfstrt;
}

uint32_t DDFStateMachine::GetStopPosition()
{
  return (ddfstop < _minValidX) ? _minValidX : ddfstop;
}

uint32_t DDFStateMachine::GetFetchSize()
{
  return (_core.RegisterUtility.IsHiresEnabled()) ? 4 : 8;
}

void DDFStateMachine::SetState(DDFStates newState, uint32_t arriveTime)
{
  _queue->Remove(this);
  _state = newState;
  _arriveTime = arriveTime;
  _queue->Insert(this);
}

void DDFStateMachine::SetStateWaitingForFirstFetch(uint32_t rasterY, uint32_t cylinder)
{
  uint32_t start = GetStartPosition();
  uint32_t currentCycle = cylinder / 2;

  if (start == currentCycle)
  {
    SetState(DDF_STATE_WAITING_FOR_NEXT_FETCH, MakeArriveTime(rasterY, cylinder + GetFetchSize() * 2));
  }
  else if (start > currentCycle)
  {
    // Fetch start will be seen on this line
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(rasterY, start * 2));
  }
  else
  {
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(rasterY + 1, start * 2));
  }
}

void DDFStateMachine::SetStateWaitingForNextFetch(uint32_t rasterY, uint32_t cylinder)
{
  uint32_t stop = GetStopPosition();
  uint32_t start = GetStartPosition();
  uint32_t currentCycle = cylinder / 2;

  if ((stop & 7) != (start & 7)) stop += GetFetchSize();

  if (stop > currentCycle)
  {
    // More fetches on this line
    SetState(DDF_STATE_WAITING_FOR_NEXT_FETCH, MakeArriveTime(rasterY, cylinder + GetFetchSize() * 2));
  }
  else if (start > currentCycle)
  {
    // Fetch start will be seen on this line
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(rasterY, start * 2));
  }
  else
  {
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(rasterY + 1, start * 2));
  }
}

void DDFStateMachine::DoStateWaitingForFirstFetch(uint32_t rasterY, uint32_t cylinder)
{
  SetStateWaitingForFirstFetch(rasterY, cylinder);
}

void DDFStateMachine::DoStateWaitingForNextFetch(uint32_t rasterY, uint32_t cylinder)
{
  SetStateWaitingForNextFetch(rasterY, cylinder);
}

bool DDFStateMachine::CanRead()
{
  return (_state == DDF_STATE_WAITING_FOR_NEXT_FETCH) && _core.RegisterUtility.GetEnabledBitplaneCount() > 0 && GraphicsContext.DIWYStateMachine.IsVisible();
}

void DDFStateMachine::ChangedValue()
{
  uint32_t rasterY = busGetRasterY();
  if (rasterY < 0x1a)
  {
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(0x1a, GetStartPosition() * 2));
    return;
  }

  switch (_state)
  {
    case DDF_STATE_WAITING_FOR_FIRST_FETCH: SetStateWaitingForFirstFetch(busGetRasterY(), busGetRasterX() * 2); break;
    case DDF_STATE_WAITING_FOR_NEXT_FETCH: SetStateWaitingForNextFetch(busGetRasterY(), busGetRasterX() * 2); break;
  }
}

/* Bus event handler */

void DDFStateMachine::InitializeEvent(GraphicsEventQueue *queue)
{
  _queue = queue;
  SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(0x1a, GetStartPosition() * 2));
}

void DDFStateMachine::Handler(uint32_t rasterY, uint32_t cylinder)
{
  Log(rasterY, cylinder);

  switch (_state)
  {
    case DDF_STATE_WAITING_FOR_FIRST_FETCH: DoStateWaitingForFirstFetch(rasterY, cylinder); break;
    case DDF_STATE_WAITING_FOR_NEXT_FETCH: DoStateWaitingForNextFetch(rasterY, cylinder); break;
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

void DDFStateMachine::EndOfFrame()
{
  SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, MakeArriveTime(0x1a, GetStartPosition() * 2));
}

void DDFStateMachine::SoftReset()
{
}

void DDFStateMachine::HardReset()
{
}

void DDFStateMachine::EmulationStart()
{
}

void DDFStateMachine::EmulationStop()
{
}

void DDFStateMachine::Startup()
{
  _minValidX = 0x18;
  _maxValidX = 0xd8;
}

void DDFStateMachine::Shutdown()
{
}
