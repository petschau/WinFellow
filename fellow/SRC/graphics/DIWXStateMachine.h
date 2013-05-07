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

#ifndef DIWXSTATEMACHINE_H
#define DIWXSTATEMACHINE_H

#include "DEFS.H"
#include "GraphicsEventQueue.h"

typedef enum DIWXStates_
{
  DIWX_STATE_WAITING_FOR_START_POS = 0,
  DIWX_STATE_WAITING_FOR_STOP_POS = 1
} DIWXStates;

class DIWXStateMachine : public GraphicsEvent
{
private:
  DIWXStates _state;
  ULO _maxValidX;

  bool _enableLog;
  FILE *_logfile;

  void Log(void);
  ULO GetStartPosition(void);
  ULO GetStopPosition(void);
  void SetState(DIWXStates newState, ULO cycle);
  void SetStateWaitingForStartPos(ULO rasterY, ULO rasterX);
  void SetStateWaitingForStopPos(ULO rasterY, ULO rasterX);
  void DoStateWaitingForStartPos(ULO rasterY, ULO rasterX);
  void DoStateWaitingForStopPos(ULO rasterY, ULO rasterX);

public:
  UBY GetOutputMask(ULO rasterX);
  bool IsVisible(void);
  void ChangedValue(void);

  virtual void InitializeEvent(GraphicsEventQueue *queue);
  virtual void Handler(ULO rasterY, ULO rasterX);

  void SoftReset(void);
  void HardReset(void);
  void EndOfFrame(void);
  void EmulationStart(void);
  void EmulationStop(void);
  void Startup(void);
  void Shutdown(void);

  DIWXStateMachine(void) : GraphicsEvent() {};

};

#endif
