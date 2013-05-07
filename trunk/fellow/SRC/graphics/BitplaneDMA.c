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

#ifdef GRAPH2

#include "bus.h"
#include "graph.h"
#include "fmem.h"
#include "fileops.h"

#include "Graphics.h"

static STR *BPLDMA_StateNames[3] = {"NONE",
				    "FETCH_LORES",
				    "FETCH_HIRES"};

void BitplaneDMA::Log(void)
{
  if (_enableLog)
  {
    if (_logfile == 0)
    {
      STR filename[MAX_PATH];
      fileopsGetGenericFileName(filename, "WinFellow", "BPLDMAStateMachine.log");
      _logfile = fopen(filename, "w");
    }
    fprintf(_logfile, "%.16I64X %.3X %.3X %s\n", busGetRasterFrameCount(), busGetRasterY(), busGetRasterX(), BPLDMA_StateNames[_state]);
  }
}

UWO BitplaneDMA::ReadWord(ULO address)
{
  return ((UWO) (memory_chip[address] << 8) | (UWO) memory_chip[address + 1]);
}

void BitplaneDMA::IncreaseBplPt(ULO *bplpt, ULO size)
{
  *bplpt = ((*bplpt) + size) & 0x1ffffe;
}

UWO BitplaneDMA::GetHold(ULO bplNo, ULO bplsEnabled, ULO *bplpt)
{
  UWO hold;

  if (bplNo <= bplsEnabled)
  {
    hold = ReadWord(*bplpt);
    IncreaseBplPt(bplpt, 2);
  }
  else
  {
    hold = 0;
  }
  return hold;
}

void BitplaneDMA::AddModulo(void)
{
  switch (BitplaneUtility::GetEnabledBitplaneCount())
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
void BitplaneDMA::Stop(void)
{
  _stopDDF = true;
}

void BitplaneDMA::SetState(BPLDMAStates newState, ULO cycle)
{
  _queue->Remove(this);
  _state = newState;
  _cycle = cycle;
  _queue->Insert(this);
  Log();
}

void BitplaneDMA::SetStateNone(void)
{
  _queue->Remove(this);
  _state = BPL_DMA_STATE_NONE;
  _cycle = GraphicsEventQueue::GRAPHICS_CYCLE_NONE;
  Log();
}

// Called from outside the state machine, cycle is the display fetch start for this unit
void BitplaneDMA::Start(ULO cycle)
{
  if (BitplaneUtility::IsDMAEnabled())
  {
    if (BitplaneUtility::IsLores())
    {
      SetState(BPL_DMA_STATE_FETCH_LORES, cycle + 7);
    }
    else
    {
      SetState(BPL_DMA_STATE_FETCH_HIRES, cycle + 3);
    }
  }
}

// Called from within the state machine
void BitplaneDMA::Restart(bool ddfIsActive)
{
  if (ddfIsActive || !ddfIsActive && _stopDDF && BitplaneUtility::IsHires())
  {
    _stopDDF = false;
    Start(_cycle + 1);
  }
  else
  {
    SetStateNone();
    AddModulo();
  }
}

void BitplaneDMA::FetchLores(void)
{
  ULO bplsEnabled = BitplaneUtility::GetEnabledBitplaneCount();

  GraphicsContext.PixelSerializer.Commit(GetHold(1, bplsEnabled, &bpl1pt),
		                         GetHold(2, bplsEnabled, &bpl2pt),
		                         GetHold(3, bplsEnabled, &bpl3pt),
		                         GetHold(4, bplsEnabled, &bpl4pt),
		                         GetHold(5, bplsEnabled, &bpl5pt),
		                         GetHold(6, bplsEnabled, &bpl6pt));
}

void BitplaneDMA::FetchHires(void)
{
  ULO bplsEnabled = BitplaneUtility::GetEnabledBitplaneCount();

  GraphicsContext.PixelSerializer.Commit(GetHold(1, bplsEnabled, &bpl1pt),
		                         GetHold(2, bplsEnabled, &bpl2pt),
		                         GetHold(3, bplsEnabled, &bpl3pt),
		                         GetHold(4, bplsEnabled, &bpl4pt),
		                         0,
		                         0);
}

void BitplaneDMA::InitializeEvent(GraphicsEventQueue *queue)
{
  _queue = queue;
  SetStateNone();
}

void BitplaneDMA::Handler(ULO rasterY, ULO rasterX)
{
  if (BitplaneUtility::IsDMAEnabled())
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

void BitplaneDMA::EndOfFrame(void)
{
  SetStateNone();
}

void BitplaneDMA::SoftReset(void)
{
}

void BitplaneDMA::HardReset(void)
{
}

void BitplaneDMA::EmulationStart(void)
{
}

void BitplaneDMA::EmulationStop(void)
{
}

void BitplaneDMA::Startup(void)
{
  _enableLog = false;
  _logfile = 0;
  _stopDDF = false;
}

void BitplaneDMA::Shutdown(void)
{
  if (_enableLog && _logfile != 0)
  {
    fclose(_logfile);
  }
}

#endif
