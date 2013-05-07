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

#ifdef GRAPH2

#include "bus.h"
#include "graph.h"
#include "fileops.h"

#include "Graphics.h"

static STR *DDFStateNames[2] = {"WAITING_FOR_FIRST_FETCH",
				"WAITING_FOR_NEXT_FETCH"};

void DDFStateMachine::Log(ULO rasterY, ULO rasterX)
{
  if (_enableLog)
  {
    if (_logfile == 0)
    {
      STR filename[MAX_PATH];
      fileopsGetGenericFileName(filename, "WinFellow", "DDFStateMachine.log");
      _logfile = fopen(filename, "w");
    }
    fprintf(_logfile, "%.16I64X %.3X %.3X for %.3X %s\n", busGetRasterFrameCount(), rasterY, rasterX, _cycle % BUS_CYCLE_PER_LINE, DDFStateNames[_state]);
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
  return (bplcon0 & 0x8000) ? 4 : 8;
}

void DDFStateMachine::SetState(DDFStates newState, ULO cycle)
{
  _queue->Remove(this);
  _state = newState;
  _cycle = cycle;
  _queue->Insert(this);
}

void DDFStateMachine::SetStateWaitingForFirstFetch(ULO rasterY, ULO rasterX)
{
  ULO start = GetStartPosition();
  if (start == rasterX)
  {
    SetState(DDF_STATE_WAITING_FOR_NEXT_FETCH, rasterY*BUS_CYCLE_PER_LINE + rasterX + GetFetchSize());
  }
  else if (start > rasterX)
  {
    // Fetch start will be seen on this line
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, rasterY*BUS_CYCLE_PER_LINE + start);
  }
  else
  {
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, (rasterY + 1)*BUS_CYCLE_PER_LINE + start);
  }
  Log(rasterY, rasterX);
}

void DDFStateMachine::SetStateWaitingForNextFetch(ULO rasterY, ULO rasterX)
{
  ULO stop = GetStopPosition();
  ULO start = GetStartPosition();
  if ((stop & 7) != (start & 7)) stop += GetFetchSize();
  
  if (stop > rasterX)
  {
    // More fetches on this line
    SetState(DDF_STATE_WAITING_FOR_NEXT_FETCH, rasterY*BUS_CYCLE_PER_LINE + rasterX + GetFetchSize());
  }
  else if (start > rasterX)
  {
    // Fetch start will be seen on this line
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, rasterY*BUS_CYCLE_PER_LINE + start);
  }
  else
  {
    SetState(DDF_STATE_WAITING_FOR_FIRST_FETCH, (rasterY + 1)*BUS_CYCLE_PER_LINE + start);
  }
}

void DDFStateMachine::DoStateWaitingForFirstFetch(ULO rasterY, ULO rasterX)
{
  SetStateWaitingForFirstFetch(rasterY, rasterX);
}

void DDFStateMachine::DoStateWaitingForNextFetch(ULO rasterY, ULO rasterX)
{
  SetStateWaitingForNextFetch(rasterY, rasterX);
}

bool DDFStateMachine::CanRead(void)
{
  return (_state == DDF_STATE_WAITING_FOR_NEXT_FETCH);
}

void DDFStateMachine::ChangedValue(void)
{
  switch (_state)
  {
    case DDF_STATE_WAITING_FOR_FIRST_FETCH:
      SetStateWaitingForFirstFetch(busGetRasterY(), busGetRasterX());
      break;
    case DDF_STATE_WAITING_FOR_NEXT_FETCH:
      SetStateWaitingForNextFetch(busGetRasterY(), busGetRasterX());
      break;
  }
}

/* Bus event handler */

void DDFStateMachine::InitializeEvent(GraphicsEventQueue *queue)
{
  _queue = queue;
  SetStateWaitingForFirstFetch(0, 0);
}

void DDFStateMachine::Handler(ULO rasterY, ULO rasterX)
{
  switch (_state)
  {
    case DDF_STATE_WAITING_FOR_FIRST_FETCH:
      DoStateWaitingForFirstFetch(rasterY, rasterX);
      break;
    case DDF_STATE_WAITING_FOR_NEXT_FETCH:
      DoStateWaitingForNextFetch(rasterY, rasterX);
      break;
  }
  if (CanRead() && GraphicsContext.DIWYStateMachine.IsVisible())
  {
    GraphicsContext.BitplaneDMA.Start(rasterY*BUS_CYCLE_PER_LINE + rasterX);
  }
  else if (!CanRead())
  {
    GraphicsContext.BitplaneDMA.Stop();
  }
}

/* Fellow events */

void DDFStateMachine::EndOfFrame(void)
{
  SetStateWaitingForFirstFetch(0, 0);
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
  _enableLog = false;
  _logfile = 0;
}

void DDFStateMachine::Shutdown(void)
{
  if (_enableLog && _logfile != 0)
  {
    fclose(_logfile);
  }
}

#endif
