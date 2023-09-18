#pragma once

/*=========================================================================*/
/* Fellow                                                                  */
/* Bitplane DMA                                                            */
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

typedef enum BPLDMAStates_
{
  BPL_DMA_STATE_NONE = 0,
  BPL_DMA_STATE_FETCH_LORES = 1,
  BPL_DMA_STATE_FETCH_HIRES = 2
} BPLDMAStates;

class BitplaneDMA : public GraphicsEvent
{
private:
  BPLDMAStates _state;
  bool _stopDDF;
  bool _hasBeenActive;

  void Log(uint32_t line, uint32_t cylinder);

  uint16_t ReadWord(uint32_t address);
  void IncreaseBplPt(uint32_t *bplpt, uint32_t size);
  uint16_t GetHold(uint32_t bplNo, uint32_t bplsEnabled, uint32_t *bplpt);
  void AddModulo();
  void SetState(BPLDMAStates newState, uint32_t cycle);
  void SetStateNone();
  void Restart(bool ddfIsActive);
  void FetchLores();
  void FetchHires();

public:
  void Start(uint32_t cycle);
  void Stop();

  virtual void InitializeEvent(GraphicsEventQueue *queue);
  virtual void Handler(uint32_t rasterY, uint32_t cylinder);

  void EndOfFrame();

  BitplaneDMA(void) : GraphicsEvent(), _stopDDF(false), _state(BPL_DMA_STATE_NONE) {};

};
