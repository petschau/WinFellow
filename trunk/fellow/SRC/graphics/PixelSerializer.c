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

#ifdef GRAPH2

#include "bus.h"
#include "graph.h"
#include "draw.h"
#include "fmem.h"
#include "fileops.h"

#include "Graphics.h"

#define PIXEL_SERIALIZER_FIRST_CYCLE 28
#define PIXEL_SERIALIZER_LAST_CYCLE 11
#define PIXEL_SERIALIZER_OUTPUT_SIZE 2

extern UBY *draw_buffer_current_ptr;
extern UBY draw_dual_translate[2][256][256];

void PixelSerializer::Log(ULO rasterY, ULO rasterX)
{
  if (_enableLog)
  {
    if (_logfile == 0)
    {
      STR filename[MAX_PATH];
      fileopsGetGenericFileName(filename, "WinFellow", "BPLSerializer.log");
      _logfile = fopen(filename, "w");
    }
    fprintf(_logfile, "%.16I64X %.3X %.3X\n", busGetRasterFrameCount(), rasterY, rasterX);
  }
}

void PixelSerializer::EventSetup(ULO cycle)
{
  _queue->Remove(this);
  _cycle = cycle;
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

  if (BitplaneUtility::IsLores())
  {
    scrollodd = 16 - oddscroll;
    scrolleven = 16 - evenscroll;
    scrollodd--;  // 0.5 cycle delay
    scrolleven--;
  }
  else
  {
    scrollodd = 16 - oddhiscroll;
    scrolleven = 16 - evenhiscroll;
    scrollodd -= 2; // 0.5 cycle delay
    scrolleven -= 2;
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

// Serialize one unit of data (here 2 cylinders, 4 lores or 8 hires pixels)
void PixelSerializer::SerializeBatch(void)
{
  if (BitplaneUtility::IsLores())
  {
    GraphicsContext.Planar2ChunkyDecoder.P2CNext4Pixels(_active[0].b[3],
			                                _active[1].b[3],
			                                _active[2].b[3],
			                                _active[3].b[3],
			                                _active[4].b[3],
			                                _active[5].b[3]);
    ShiftActive(4);
  }
  else
  {
    GraphicsContext.Planar2ChunkyDecoder.P2CNext8Pixels(_active[0].b[3],
			                                _active[1].b[3],
			                                _active[2].b[3],
			                                _active[3].b[3],
			                                _active[4].b[3],
			                                _active[5].b[3]);
    ShiftActive(8);
  }
}

bool PixelSerializer::OutputCylinders(ULO rasterY, ULO rasterX)
{
  bool isEndOfLine = (rasterX == PIXEL_SERIALIZER_LAST_CYCLE);

  if (rasterY == 0) rasterY = BUS_LINES_PER_FRAME;

  if (rasterX <= PIXEL_SERIALIZER_LAST_CYCLE)
  {
    rasterX += BUS_CYCLE_PER_LINE;
    rasterY--;
  }

  GraphicsContext.Planar2ChunkyDecoder.NewBatch();
  SerializeBatch();
  GraphicsContext.Sprites.OutputSprites(rasterX, 2);
  GraphicsContext.BitplaneDraw.DrawBatch(rasterY, rasterX);

  return isEndOfLine;
}

void PixelSerializer::Handler(ULO rasterY, ULO rasterX)
{
  bool isEndOfLine = OutputCylinders(rasterY, rasterX);

  if (rasterY == 0x19)
  {
    GraphicsContext.Sprites.EndOfLine(rasterY);
    EventSetup(0x1a*BUS_CYCLE_PER_LINE + PIXEL_SERIALIZER_LAST_CYCLE);
  }
  else if (rasterY == 0x1a && isEndOfLine)
  {
    GraphicsContext.Sprites.EndOfLine(rasterY);
    EventSetup(0x1a*BUS_CYCLE_PER_LINE + PIXEL_SERIALIZER_FIRST_CYCLE);
  }
  else if (rasterY == 0 && isEndOfLine)
  {
    drawEndOfFrame();
    EventSetup(0x19*BUS_CYCLE_PER_LINE + PIXEL_SERIALIZER_LAST_CYCLE);
    GraphicsContext.Sprites.EndOfFrame();
  }
  else if (isEndOfLine)
  {
    EventSetup(rasterY*BUS_CYCLE_PER_LINE + PIXEL_SERIALIZER_FIRST_CYCLE);

    for (ULO i = 0; i < 6; ++i)
    {
      _active[i].l = 0;
    }

    GraphicsContext.Sprites.EndOfLine(rasterY);
  }
  else 
  {
    EventSetup(rasterY*BUS_CYCLE_PER_LINE + rasterX + PIXEL_SERIALIZER_OUTPUT_SIZE);
  }
}

void PixelSerializer::InitializeEvent(GraphicsEventQueue *queue)
{
  _queue = queue;
  EventSetup(1);
}

/* Fellow events */

void PixelSerializer::EndOfFrame(void)
{
  EventSetup(1);
}

void PixelSerializer::SoftReset(void)
{
}

void PixelSerializer::HardReset(void)
{
  EventSetup(1);
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

#endif
