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
  uint32_t _minValidX;
  uint32_t _maxValidX;

  void Log(uint32_t line, uint32_t cylinder);
  uint32_t GetStartPosition(void);
  uint32_t GetStopPosition(void);
  uint32_t GetFetchSize(void);
  void SetState(DDFStates newState, uint32_t arriveTime);
  void SetStateWaitingForFirstFetch(uint32_t rasterY, uint32_t cylinder);
  void SetStateWaitingForNextFetch(uint32_t rasterY, uint32_t cylinder);
  void DoStateWaitingForFirstFetch(uint32_t rasterY, uint32_t cylinder);
  void DoStateWaitingForNextFetch(uint32_t rasterY, uint32_t cylinder);

public:
  bool CanRead(void);
  void ChangedValue(void);

  virtual void InitializeEvent(GraphicsEventQueue *queue);
  virtual void Handler(uint32_t rasterY, uint32_t cylinder);

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
