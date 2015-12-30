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

extern UBY *draw_buffer_current_ptr;
extern UBY draw_dual_translate[2][256][256];

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
  _active[0].l <<= pixelCount;
  _active[1].l <<= pixelCount;
  _active[2].l <<= pixelCount;
  _active[3].l <<= pixelCount;
  _active[4].l <<= pixelCount;
  _active[5].l <<= pixelCount;
}

void PixelSerializer::Commit(UWO dat1, UWO dat2, UWO dat3, UWO dat4, UWO dat5, UWO dat6)
{
  ULO scrollodd, scrolleven;
  ULO oddmask, invoddmask;
  ULO evenmask, invevenmask;

  _activated = true;

  if (BitplaneUtility::IsLores())
  {
    scrollodd = 16 - 1 - oddscroll;
    scrolleven = 16 - 1 - evenscroll;
  }
  else
  {
    scrollodd = 16 - 2 - oddhiscroll;
    scrolleven = 16 - 2 - evenhiscroll;
  }

  oddmask = 0xffff << scrollodd;
  evenmask = 0xffff << scrolleven;
  invoddmask = ~oddmask;
  invevenmask = ~evenmask;

  _active[0].l = (_active[0].l & invoddmask)  | ( ( ((ULO) dat1) << scrollodd)  & oddmask);
  _active[1].l = (_active[1].l & invevenmask) | ( ( ((ULO) dat2) << scrolleven) & evenmask);
  _active[2].l = (_active[2].l & invoddmask)  | ( ( ((ULO) dat3) << scrollodd)  & oddmask);
  _active[3].l = (_active[3].l & invevenmask) | ( ( ((ULO) dat4) << scrolleven) & evenmask);
  _active[4].l = (_active[4].l & invoddmask)  | ( ( ((ULO) dat5) << scrollodd)  & oddmask);
  _active[5].l = (_active[5].l & invevenmask) | ( ( ((ULO) dat6) << scrolleven) & evenmask);
}

void PixelSerializer::SerializePixels(ULO pixelCount)
{
  ULO pixelIterations8 = pixelCount >> 3;
  for (ULO i = 0; i < pixelIterations8; ++i)
  {
    GraphicsContext.Planar2ChunkyDecoder.P2CNext8Pixels(_active[0].b[3],
			                                _active[1].b[3],
			                                _active[2].b[3],
			                                _active[3].b[3],
			                                _active[4].b[3],
			                                _active[5].b[3]);
    ShiftActive(8);
  }
  if (pixelCount & 4)
  {
    GraphicsContext.Planar2ChunkyDecoder.P2CNext4Pixels(_active[0].b[3],
			                                _active[1].b[3],
			                                _active[2].b[3],
			                                _active[3].b[3],
			                                _active[4].b[3],
			                                _active[5].b[3]);
    ShiftActive(4);
  }
  
  ULO remainingPixels = pixelCount & 3;
  if (remainingPixels > 0)
  {
    GraphicsContext.Planar2ChunkyDecoder.P2CNextPixels(remainingPixels,
                                                       _active[0].b[3],
			                               _active[1].b[3],
			                               _active[2].b[3],
			                               _active[3].b[3],
			                               _active[4].b[3],
			                               _active[5].b[3]);
    ShiftActive(remainingPixels);
  }
}

void PixelSerializer::SerializeBatch(ULO cylinderCount)
{
  if (BitplaneUtility::IsLores())
  {
    SerializePixels(cylinderCount);
  }
  else
  {
    SerializePixels(cylinderCount*2);
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
  SerializeBatch(cylinderCount);
  if (GraphicsContext.DIWYStateMachine.IsVisible() && _activated)
  {
    GraphicsContext.Sprites.OutputSprites(_lastCylinderOutput + 1, cylinderCount);
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
    GraphicsContext.Sprites.EndOfLine(rasterY);
    EventSetup(MakeArriveTime(rasterY + 1, LAST_CYLINDER));
    return;
  }

  OutputCylindersUntil(rasterY, cylinder);

  for (ULO i = 0; i < 6; ++i)
  {
    _active[i].l = 0;
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
    GraphicsContext.Sprites.EndOfLine(rasterY);
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
