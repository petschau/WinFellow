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

#include "defs.h"

#include "chipset.h"
#include "bus.h"
#include "graph.h"
#include "fmem.h"
#include "Graphics.h"

using namespace CustomChipset;

static const char* BPLDMA_StateNames[3] = { "NONE",
                                    "FETCH_LORES",
                                    "FETCH_HIRES" };

void BitplaneDMA::Log(uint32_t line, uint32_t cylinder)
{
  if (GraphicsContext.Logger.IsLogEnabled())
  {
    char msg[256];
    sprintf(msg, "BitplaneDMA %s\n", BPLDMA_StateNames[_state]);
    GraphicsContext.Logger.Log(line, cylinder, msg);
  }
}

uint16_t BitplaneDMA::ReadWord(uint32_t address)
{
  return chipmemReadWord(address);
}

void BitplaneDMA::IncreaseBplPt(uint32_t* bplpt, uint32_t size)
{
  *bplpt = chipsetMaskPtr((*bplpt) + size);
}

uint16_t BitplaneDMA::GetHold(uint32_t bplNo, uint32_t bplsEnabled, uint32_t* bplpt)
{
  if (bplNo <= bplsEnabled)
  {
    uint16_t hold = ReadWord(*bplpt);
    IncreaseBplPt(bplpt, 2);
    return hold;
  }
  return 0;
}

void BitplaneDMA::AddModulo()
{
  switch (_core.RegisterUtility.GetEnabledBitplaneCount())
  {
  case 6: IncreaseBplPt(&bpl6pt, bpl2mod);
  case 5: IncreaseBplPt(&bpl5pt, bpl1mod);
  case 4: IncreaseBplPt(&bpl4pt, bpl2mod);
  case 3: IncreaseBplPt(&bpl3pt, bpl1mod);
  case 2: IncreaseBplPt(&bpl2pt, bpl2mod);
  case 1: IncreaseBplPt(&bpl1pt, bpl1mod);
  }
}

// Stop after the current unit
void BitplaneDMA::Stop()
{
  _stopDDF = true;
}

void BitplaneDMA::SetState(BPLDMAStates newState, uint32_t arriveTime)
{
  _queue->Remove(this);
  _state = newState;
  _arriveTime = arriveTime;
  _queue->Insert(this);
}

void BitplaneDMA::SetStateNone()
{
  _queue->Remove(this);
  _state = BPL_DMA_STATE_NONE;
  _arriveTime = GraphicsEventQueue::GRAPHICS_ARRIVE_TIME_NONE;
}

// Called from outside the state machine
// arrive_time points to the start of the fetch unit
void BitplaneDMA::Start(uint32_t arriveTime)
{
  if (_core.RegisterUtility.IsMasterDMAAndBitplaneDMAEnabled())
  {
    if (_core.RegisterUtility.IsLoresEnabled())
    {
      SetState(BPL_DMA_STATE_FETCH_LORES, arriveTime + (7 * 2 + 1));
    }
    else
    {
      SetState(BPL_DMA_STATE_FETCH_HIRES, arriveTime + (3 * 2 + 1));
    }
  }
}

// Called from within the state machine
void BitplaneDMA::Restart(bool ddfIsActive)
{
  if (ddfIsActive || !ddfIsActive && _stopDDF && _core.RegisterUtility.IsHiresEnabled())
  {
    _stopDDF = false;
    uint32_t startOfNextFetchUnit = _arriveTime + 1;
    Start(startOfNextFetchUnit);
  }
  else
  {
    SetStateNone();
    AddModulo();
  }
}

void BitplaneDMA::FetchLores()
{
  uint32_t bplsEnabled = _core.RegisterUtility.GetEnabledBitplaneCount();

  GraphicsContext.PixelSerializer.Commit(GetHold(1, bplsEnabled, &bpl1pt),
    GetHold(2, bplsEnabled, &bpl2pt),
    GetHold(3, bplsEnabled, &bpl3pt),
    GetHold(4, bplsEnabled, &bpl4pt),
    GetHold(5, bplsEnabled, &bpl5pt),
    GetHold(6, bplsEnabled, &bpl6pt));
}

void BitplaneDMA::FetchHires()
{
  uint32_t bplsEnabled = _core.RegisterUtility.GetEnabledBitplaneCount();

  GraphicsContext.PixelSerializer.Commit(GetHold(1, bplsEnabled, &bpl1pt),
    GetHold(2, bplsEnabled, &bpl2pt),
    GetHold(3, bplsEnabled, &bpl3pt),
    GetHold(4, bplsEnabled, &bpl4pt),
    0,
    0);
}

void BitplaneDMA::InitializeEvent(GraphicsEventQueue* queue)
{
  _queue = queue;
  SetStateNone();
}

void BitplaneDMA::Handler(uint32_t rasterY, uint32_t cylinder)
{
  Log(rasterY, cylinder);

  GraphicsContext.PixelSerializer.OutputCylindersUntil(rasterY, cylinder);

  if (_core.RegisterUtility.IsMasterDMAAndBitplaneDMAEnabled())
  {
    switch (_state)
    {
    case BPL_DMA_STATE_FETCH_LORES:
      FetchLores();
      break;
    case BPL_DMA_STATE_FETCH_HIRES:
      FetchHires();
      break;
    }
    Restart(GraphicsContext.DDFStateMachine.CanRead());
  }
  else
  {
    SetStateNone();
  }
}

/* Fellow events */

void BitplaneDMA::EndOfFrame()
{
  SetStateNone();
}

