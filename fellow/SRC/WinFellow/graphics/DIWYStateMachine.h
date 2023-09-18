#pragma once

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

#include "DEFS.H"
#include "GraphicsEventQueue.h"

typedef enum DIWYStates_
{
  DIWY_STATE_WAITING_FOR_START_LINE = 0,
  DIWY_STATE_WAITING_FOR_STOP_LINE = 1
} DIWYStates;

class DIWYStateMachine : public GraphicsEvent
{
private:
  DIWYStates _state;
  uint32_t _minValidY;
  uint32_t _maxValidY;

  void Log(uint32_t line, uint32_t cylinder);
  uint32_t GetStartLine(void);
  uint32_t GetStopLine(void);
  void SetState(DIWYStates newState, uint32_t arriveTime);
  void SetStateWaitingForStartLine(uint32_t rasterY);
  void SetStateWaitingForStopLine(uint32_t rasterY);
  void DoStateWaitingForStartLine(uint32_t rasterY);
  void DoStateWaitingForStopLine(uint32_t rasterY);


public:
  bool IsVisible(void);
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

  DIWYStateMachine(void) : GraphicsEvent() {};

};
