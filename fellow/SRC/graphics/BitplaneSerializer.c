/*=========================================================================*/
/* Fellow                                                                  */
/* Bitplane serializer                                                     */
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

#include "bus.h"
#include "graph.h"
#include "draw.h"
#include "fmem.h"

#include "Graphics.h"

enum PlayfieldNumber
{
  Playfield1 = 0,
  Playfield2 = 1
};

void BitplaneSerializer::LogEndOfLine(unsigned int lineNumber, unsigned int cylinder)
{
  if (GraphicsContext.Logger.IsLogEnabled())
  {
    GraphicsContext.Logger.Log(lineNumber, cylinder, "SERIALIZER: End of line\n");
  }
}

void BitplaneSerializer::LogOutputUntilCylinder(unsigned int lineNumber, unsigned int cylinder, unsigned int startCylinder, unsigned int untilCylinder)
{
  if (GraphicsContext.Logger.IsLogEnabled())
  {
    STR msg[256];
    sprintf(msg, "Output: %u to %u\n", startCylinder, untilCylinder);
    GraphicsContext.Logger.Log(lineNumber, cylinder, msg);
  }
}

void BitplaneSerializer::EventSetup(unsigned int arriveTime)
{
  _queue->Remove(this);
  _arriveTime = arriveTime;
  _queue->Insert(this);
}

void BitplaneSerializer::Commit(UWO dat1, UWO dat2, UWO dat3, UWO dat4, UWO dat5, UWO dat6)
{
  _state.Commit(dat1, dat2, dat3, dat4, dat5, dat6);
}

void BitplaneSerializer::MaybeCopyPendingToActive(unsigned int currentCylinder, unsigned int shiftMatchMask)
{
  unsigned int currentCylinderMatch = currentCylinder & shiftMatchMask;
  if (currentCylinderMatch == (oddscroll & shiftMatchMask))
  {
    _state.CopyPendingToActive(Playfield1);
  }
  if (currentCylinderMatch == (evenscroll & shiftMatchMask))
  {
    _state.CopyPendingToActive(Playfield2);
  }
}

unsigned int BitplaneSerializer::GetCylinderWrappedByHPos(unsigned int cylinder)
{
  // Counter wraps around after cycle 0. Affects shift.
  if (cylinder == 456) // TODO: Replace constant with dynamic line geometry
  {
    return 2;
  }
  return cylinder;
}

unsigned int BitplaneSerializer::GetRightExtendedCylinder(unsigned int lineNumber, unsigned int cylinder)
{
  if (lineNumber == _state.CurrentLineNumber)
  {
    return cylinder;
  }
  return cylinder + GraphicsEventQueue::GetCylindersPerLine();
}

void BitplaneSerializer::OutputBitplanes(unsigned int cylinderCount)
{
  bool isLores = BitplaneUtility::IsLores();
  unsigned int shiftMatchMask = (isLores) ? 0xf : 0x7;
  unsigned int cylinderPixelCount = (isLores) ? 1 : 2;
  unsigned int currentCylinder = GetCylinderWrappedByHPos(_state.CurrentCylinder);

  for (unsigned int i = 0; i < cylinderCount; i++)
  {
    GraphicsContext.Planar2ChunkyDecoder.P2CNextPixels(cylinderPixelCount, _state.Active);
    _state.ShiftActive(cylinderPixelCount);
    MaybeCopyPendingToActive(currentCylinder, shiftMatchMask);
    currentCylinder = GetCylinderWrappedByHPos(currentCylinder + 1);
  }
}

void BitplaneSerializer::OutputBackground(unsigned int cylinderCount)
{
  if (BitplaneUtility::IsHires())
  {
    cylinderCount *= 2;
  }
  GraphicsContext.Planar2ChunkyDecoder.P2CNextBackground(cylinderCount);
}

void BitplaneSerializer::OutputCylinders(unsigned int cylinderCount)
{
  if (_state.Activated)
  {
    OutputBitplanes(cylinderCount);
  }
  else
  {
    OutputBackground(cylinderCount);
  }
}

bool BitplaneSerializer::IsInVerticalBlank(unsigned int lineNumber)
{
  return lineNumber < 0x1a; // TODO: Replace constant with dynamic line geometry
}

bool BitplaneSerializer::IsFirstLineInVerticalBlank(unsigned int lineNumber)
{
  return lineNumber == 0;
}

void BitplaneSerializer::OutputCylindersUntil(unsigned int lineNumber, unsigned int cylinder)
{
  if (IsInVerticalBlank(_state.CurrentLineNumber))
  {
    return;
  }

  unsigned int outputUntilCylinder = GetRightExtendedCylinder(lineNumber, cylinder);
  if (outputUntilCylinder < _state.CurrentCylinder)
  {
    // Multiple commits can happen on the same cycle, or in the horizontal blank on a "new" line
    return;
  }

  unsigned int cylinderCount = outputUntilCylinder - _state.CurrentCylinder + 1;

  LogOutputUntilCylinder(_state.CurrentLineNumber, outputUntilCylinder, _state.CurrentCylinder, outputUntilCylinder);

  GraphicsContext.Planar2ChunkyDecoder.NewBatch();
  OutputCylinders(cylinderCount);

  if (GraphicsContext.DIWYStateMachine.IsVisible() && _state.Activated)
  {
    cycle_exact_sprites->OutputSprites(_state.CurrentCylinder, cylinderCount);
  }

  GraphicsContext.BitplaneDraw.DrawBatch(_state.CurrentLineNumber, _state.CurrentCylinder);

  _state.CurrentCylinder = outputUntilCylinder + 1;
}

void BitplaneSerializer::Handler(unsigned int lineNumber, unsigned int cylinder)
{
  LogEndOfLine(lineNumber, cylinder);

  if (IsInVerticalBlank(lineNumber) && !IsFirstLineInVerticalBlank(lineNumber))
  {
    _state.CurrentLineNumber = lineNumber;
    EventSetup(MakeArriveTime(lineNumber + 1, _state.MaximumCylinder));
    return;
  }

  OutputCylindersUntil(lineNumber, cylinder);
  _state.InitializeForNewLine(lineNumber);

  if (IsFirstLineInVerticalBlank(lineNumber))
  {
    drawEndOfFrame();
  }

  EventSetup(MakeArriveTime(lineNumber + 1, _state.MaximumCylinder));
}

void BitplaneSerializer::InitializeEvent(GraphicsEventQueue *queue)
{
  _queue = queue;
  _priority = 1;
  EventSetup(_state.MaximumCylinder);
}

/* Fellow events */

void BitplaneSerializer::EndOfFrame()
{
  EventSetup(_state.MaximumCylinder);
}

void BitplaneSerializer::SoftReset()
{
}

void BitplaneSerializer::HardReset()
{
  EventSetup(_state.MaximumCylinder);
}

void BitplaneSerializer::EmulationStart()
{
}

void BitplaneSerializer::EmulationStop()
{
}

void BitplaneSerializer::Startup()
{
}

void BitplaneSerializer::Shutdown()
{
}

BitplaneSerializer::BitplaneSerializer()
  : GraphicsEvent()
{
}
