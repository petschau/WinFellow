#pragma once

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

#include "Defs.h"
#include "GraphicsEventQueue.h"

enum class DIWXStates
{
  DIWX_STATE_WAITING_FOR_START_POS = 0,
  DIWX_STATE_WAITING_FOR_STOP_POS = 1
};

class DIWXStateMachine : public GraphicsEvent
{
private:
  DIWXStates _state;
  uint32_t _maxValidX;

  void Log(uint32_t line, uint32_t cylinder);
  uint32_t GetStartPosition();
  uint32_t GetStopPosition();
  void SetState(DIWXStates newState, uint32_t arriveTime);
  void SetStateWaitingForStartPos(uint32_t rasterY, uint32_t cylinder);
  void SetStateWaitingForStopPos(uint32_t rasterY, uint32_t cylinder);
  void DoStateWaitingForStartPos(uint32_t rasterY, uint32_t cylinder);
  void DoStateWaitingForStopPos(uint32_t rasterY, uint32_t cylinder);

  void OutputCylindersUntilPreviousCylinder(uint32_t rasterY, uint32_t cylinder);

public:
  // uint8_t GetOutputMask(uint32_t rasterX);
  bool IsVisible();
  void ChangedValue();

  virtual void InitializeEvent(GraphicsEventQueue *queue);
  virtual void Handler(uint32_t rasterY, uint32_t cylinder);

  void SoftReset();
  void HardReset();
  void EndOfFrame();
  void EmulationStart();
  void EmulationStop();
  void Startup();
  void Shutdown();

  DIWXStateMachine() : GraphicsEvent(){};
};
