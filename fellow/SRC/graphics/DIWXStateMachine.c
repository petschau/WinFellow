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

#ifdef GRAPH2

#include "bus.h"
#include "graph.h"
#include "fileops.h"

#include "DIWXStateMachine.h"
#include "GraphicsEventQueue.h"

static STR *DIWXStateNames[2] = {"WAITING_FOR_START_POS",
				 "WAITING_FOR_STOP_POS"};

void DIWXStateMachine::Log(void)
{
  if (_enableLog)
  {
    if (_logfile == 0)
    {
      STR filename[MAX_PATH];
      fileopsGetGenericFileName(filename, "WinFellow", "DIWXStateMachine.log");
      _logfile = fopen(filename, "w");
    }
    fprintf(_logfile, "%.16I64X %.3X %.3X %s\n", busGetRasterFrameCount(), busGetRasterY(), busGetRasterX(), DIWXStateNames[_state]);
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

void DIWXStateMachine::SetState(DIWXStates newState, ULO cycle)
{
  _queue->Remove(this);
  _state = newState;
  _cycle = cycle;
  _queue->Insert(this);
  Log();
}

void DIWXStateMachine::SetStateWaitingForStartPos(ULO rasterY, ULO rasterX)
{
  ULO start = GetStartPosition() / 2;
  if (start <= rasterX)
  {
    // Start is seen on the next line
    rasterY++;
  }
  SetState(DIWX_STATE_WAITING_FOR_START_POS, rasterY*BUS_CYCLE_PER_LINE + start);
}

void DIWXStateMachine::SetStateWaitingForStopPos(ULO rasterY, ULO rasterX)
{
  if (GetStopPosition() > _maxValidX)
  {
    // Stop position will never be found, wait beyond end of frame (effectively disabled)
    SetState(DIWX_STATE_WAITING_FOR_STOP_POS, BUS_CYCLE_PER_FRAME + 1);
  }
  else if (GetStopPosition() <= rasterX*2)
  {
    // Stop position will be found on the next line
    SetState(DIWX_STATE_WAITING_FOR_STOP_POS, (rasterY + 1)*BUS_CYCLE_PER_LINE + GetStopPosition()/2);
  }
  else
  {
    SetState(DIWX_STATE_WAITING_FOR_STOP_POS, rasterY*BUS_CYCLE_PER_LINE + GetStopPosition()/2);
  }
}

void DIWXStateMachine::DoStateWaitingForStartPos(ULO rasterY, ULO rasterX)
{
  SetStateWaitingForStopPos(rasterY, rasterX);
}

void DIWXStateMachine::DoStateWaitingForStopPos(ULO rasterY, ULO rasterX)
{
  SetStateWaitingForStartPos(rasterY, rasterX);
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
      SetStateWaitingForStartPos(busGetRasterY(), busGetRasterX());
      break;
    case DIWX_STATE_WAITING_FOR_STOP_POS:
      SetStateWaitingForStopPos(busGetRasterY(), busGetRasterX());
      break;
  }
}

/* Bus event handler */

void DIWXStateMachine::InitializeEvent(GraphicsEventQueue *queue)
{
  _queue = queue;
  SetStateWaitingForStartPos(0, 0);
}

void DIWXStateMachine::Handler(ULO rasterY, ULO rasterX)
{
  switch (_state)
  {
    case DIWX_STATE_WAITING_FOR_START_POS:
      DoStateWaitingForStartPos(rasterY, rasterX);
      break;
    case DIWX_STATE_WAITING_FOR_STOP_POS:
      DoStateWaitingForStopPos(rasterY, rasterX);
      break;
  }
}

UBY start_mask[4] = {0xf0, 0x70, 0x30, 0x10};
UBY stop_mask[4] = {0x00, 0x80, 0xc0, 0xe0};

UBY DIWXStateMachine::GetOutputMask(ULO rasterX)
{
  ULO first_cylinder = rasterX*2 + 1;
  ULO last_cylinder = first_cylinder + 3;

  if (_state == DIWX_STATE_WAITING_FOR_START_POS)
  {
    if (GetStartPosition() > last_cylinder) return 0x00;
    else if (GetStartPosition() < first_cylinder) return 0x00;

    return start_mask[GetStartPosition() - first_cylinder];
  }

  if (GetStopPosition() > last_cylinder) return 0xff;
  else if (GetStopPosition() < first_cylinder) return 0xff;

  return stop_mask[GetStopPosition() - first_cylinder];
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
  _enableLog = false;
  _logfile = 0;
}

void DIWXStateMachine::Shutdown(void)
{
  if (_enableLog && _logfile != 0)
  {
    fclose(_logfile);
  }
}

#endif
