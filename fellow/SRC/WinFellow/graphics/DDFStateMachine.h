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

#ifndef DDFSTATEMACHINE_H
#define DDFSTATEMACHINE_H

#include "DEFS.H"
#include "GraphicsEventQueue.h"

typedef enum DDFStates_
{
  DDF_STATE_WAITING_FOR_FIRST_FETCH = 0,
  DDF_STATE_WAITING_FOR_NEXT_FETCH = 1
} DDFStates;

class DDFStateMachine : public GraphicsEvent
{
private:
  DDFStates _state;
  ULO _minValidX;
  ULO _maxValidX;

  void Log(ULO line, ULO cylinder);
  ULO GetStartPosition(void);
  ULO GetStopPosition(void);
  ULO GetFetchSize(void);
  void SetState(DDFStates newState, ULO arriveTime);
  void SetStateWaitingForFirstFetch(ULO rasterY, ULO cylinder);
  void SetStateWaitingForNextFetch(ULO rasterY, ULO cylinder);
  void DoStateWaitingForFirstFetch(ULO rasterY, ULO cylinder);
  void DoStateWaitingForNextFetch(ULO rasterY, ULO cylinder);

public:
  bool CanRead(void);
  void ChangedValue(void);

  virtual void InitializeEvent(GraphicsEventQueue *queue);
  virtual void Handler(ULO rasterY, ULO cylinder);

  void SoftReset(void);
  void HardReset(void);
  void EndOfFrame(void);
  void EmulationStart(void);
  void EmulationStop(void);
  void Startup(void);
  void Shutdown(void);

  DDFStateMachine(void) : GraphicsEvent() {};
};

#endif
