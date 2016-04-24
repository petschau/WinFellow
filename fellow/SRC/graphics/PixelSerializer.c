/*=========================================================================*/
/* Fellow                                                                  */
/* Pixel serializer                                                        */
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

extern UBY draw_dual_translate[2][256][256];

#define SERIALIZER_PLAYFIELD1 0
#define SERIALIZER_PLAYFIELD2 1

bool PixelSerializer::GetActivated()
{
  return _activated;
}

bool PixelSerializer::GetIsWaitingForActivationOnLine()
{
  return _newLine;
}

ULO PixelSerializer::GetLastCylinderOutput()
{
  return _lastCylinderOutput;
}

ULO PixelSerializer::GetFirstPossibleOutputCylinder()
{
  return FIRST_CYLINDER;
}

ULO PixelSerializer::GetLastPossibleOutputCylinder()
{
  return LAST_CYLINDER;
}

UWO PixelSerializer::GetPendingWord(unsigned int bitplaneNumber)
{
  return _pendingWord[bitplaneNumber];
}

void PixelSerializer::LogEndOfLine(ULO line, ULO cylinder)
{
  if (GraphicsContext.Logger.IsLogEnabled())
  {
    GraphicsContext.Logger.Log(line, cylinder, "SERIALIZER: End of line\n");
  }
}

void PixelSerializer::LogOutput(ULO line, ULO cylinder, ULO startCylinder, ULO untilCylinder)
{
  if (GraphicsContext.Logger.IsLogEnabled())
  {
    STR msg[256];
    sprintf(msg, "Output: %d to %d\n", startCylinder, untilCylinder);
    GraphicsContext.Logger.Log(line, cylinder, msg);
  }
}

void PixelSerializer::EventSetup(ULO arriveTime)
{
  _queue->Remove(this);
  _arriveTime = arriveTime;
  _queue->Insert(this);
}

void PixelSerializer::ShiftActive(ULO pixelCount)
{
  for (unsigned int i = 0; i < 6; i++)
  {
    _activeWord[i].w <<= pixelCount;
  }
}

void PixelSerializer::CopyPendingToActive(unsigned int playfieldNumber)
{
  for (unsigned int i = playfieldNumber; i < 6; i+=2)
  {
    _activeWord[i].w = _pendingWord[i];
    _pendingWord[i] = 0;
  }
}

void PixelSerializer::Commit(UWO dat1, UWO dat2, UWO dat3, UWO dat4, UWO dat5, UWO dat6)
{
  _activated = true;

  _pendingWord[0] = dat1;
  _pendingWord[1] = dat2;
  _pendingWord[2] = dat3;
  _pendingWord[3] = dat4;
  _pendingWord[4] = dat5;
  _pendingWord[5] = dat6;
}

void PixelSerializer::SerializeCylinders(ULO cylinderCount)
{
  ULO pending_copy_mask = (BitplaneUtility::IsLores()) ? 0xf : 0x7;
  ULO cylinderPixelCount = (BitplaneUtility::IsLores()) ? 1 : 2;
  ULO currentCylinder = _lastCylinderOutput + 1;

  for (unsigned i = 0; i < cylinderCount; i++)
  {
    GraphicsContext.Planar2ChunkyDecoder.P2CNextPixels(cylinderPixelCount,
                                                       _activeWord[0].b[1],
                                                       _activeWord[1].b[1],
                                                       _activeWord[2].b[1],
                                                       _activeWord[3].b[1],
                                                       _activeWord[4].b[1],
                                                       _activeWord[5].b[1]);
    ShiftActive(cylinderPixelCount);

    if ((currentCylinder & pending_copy_mask) == (oddscroll & pending_copy_mask))
    {
      CopyPendingToActive(SERIALIZER_PLAYFIELD1);
    }
    if ((currentCylinder & pending_copy_mask) == (evenscroll & pending_copy_mask))
    {
      CopyPendingToActive(SERIALIZER_PLAYFIELD2);
    }
    currentCylinder++;
  }
}

ULO PixelSerializer::GetOutputLine(ULO rasterY, ULO cylinder)
{
  if (cylinder <= LAST_CYLINDER)
  {
    if (rasterY == 0) return busGetLinesInThisFrame() - 1;
    return rasterY - 1;
  }
  return rasterY;
}

ULO PixelSerializer::GetOutputCylinder(ULO cylinder)
{
  if (cylinder <= LAST_CYLINDER && !_newLine) return cylinder + GraphicsEventQueue::GetCylindersPerLine();
  return cylinder;
}

void PixelSerializer::OutputCylindersUntil(ULO rasterY, ULO cylinder)
{
  ULO outputUntilCylinder = GetOutputCylinder(cylinder);
  if (outputUntilCylinder <= _lastCylinderOutput)
  {
    // Multiple commits can happen on the same cycle
    return;
  }
  ULO outputLine = GetOutputLine(rasterY, cylinder);
  if (outputLine < 0x1a)
  {
    return;
  }
  _newLine = false;

  ULO cylinderCount = outputUntilCylinder - _lastCylinderOutput;

  LogOutput(outputLine, outputUntilCylinder, _lastCylinderOutput + 1, outputUntilCylinder);

  if (outputUntilCylinder > 479)
  {
    // For debug
    MessageBox(0, "outputUntilCylinder larger than it should be", "outputUntilCylinder out of range", 0);
  }

  if (outputUntilCylinder < _lastCylinderOutput)
  {
    MessageBox(0, "outputUntilCylinder less than _lastCylinderOutput", "outputUntilCylinder out of range", 0);
  }

  if (cylinderCount == 0)
  {
    return;
  }

  GraphicsContext.Planar2ChunkyDecoder.NewBatch();
  SerializeCylinders(cylinderCount);

  if (GraphicsContext.DIWYStateMachine.IsVisible() && _activated)
  {
    cycle_exact_sprites->OutputSprites(_lastCylinderOutput + 1, cylinderCount);
  }
  GraphicsContext.BitplaneDraw.DrawBatch(outputLine, _lastCylinderOutput + 1);

  _lastCylinderOutput = outputUntilCylinder;
}

void PixelSerializer::Handler(ULO rasterY, ULO cylinder)
{
  LogEndOfLine(rasterY, cylinder);

  ULO line = GetOutputLine(rasterY, cylinder);
  if (line < 0x1a)
  {
//    sprites->EndOfLine(rasterY);
    EventSetup(MakeArriveTime(rasterY + 1, LAST_CYLINDER));
    return;
  }

  OutputCylindersUntil(rasterY, cylinder);

  for (ULO i = 0; i < 6; ++i)
  {
    _activeWord[i].w = 0;
  }
  _lastCylinderOutput = FIRST_CYLINDER - 1;
  _newLine = true;
  _activated = false;

  if (line == busGetLinesInThisFrame() - 1)
  {
    drawEndOfFrame();
    EventSetup(MakeArriveTime(0x19, LAST_CYLINDER));
    return;
  }
  else
  {
//    sprites->EndOfLine(rasterY);
  }
  EventSetup(MakeArriveTime(rasterY + 1, LAST_CYLINDER));
}

void PixelSerializer::InitializeEvent(GraphicsEventQueue *queue)
{
  _queue = queue;
  _priority = 1;
  EventSetup(LAST_CYLINDER);
}

/* Fellow events */

void PixelSerializer::EndOfFrame(void)
{
  EventSetup(LAST_CYLINDER);
}

void PixelSerializer::SoftReset(void)
{
}

void PixelSerializer::HardReset(void)
{
  EventSetup(LAST_CYLINDER);
}

void PixelSerializer::EmulationStart(void)
{
}

void PixelSerializer::EmulationStop(void)
{
}

void PixelSerializer::Startup(void)
{
}

void PixelSerializer::Shutdown(void)
{
}
