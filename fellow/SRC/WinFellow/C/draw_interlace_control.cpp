/*=========================================================================*/
/* Fellow                                                                  */
/* Automatic interlace detection and rendering                             */
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
#include "FELLOW.H"
#include "BUS.H"
#include "GRAPH.H"
#include "DRAW.H"
#include "draw_interlace_control.h"
#include "VirtualHost/Core.h"

typedef struct
{
  bool frame_is_interlaced;
  bool frame_is_long;
  bool enable_deinterlace;
  bool use_interlaced_rendering;
} draw_interlace_status;

draw_interlace_status interlace_status;

bool drawGetUseInterlacedRendering()
{
  return interlace_status.use_interlaced_rendering;
}

bool drawGetFrameIsLong()
{
  return interlace_status.frame_is_long;
}

bool drawDecideUseInterlacedRendering()
{
  return interlace_status.enable_deinterlace && interlace_status.frame_is_interlaced;
}

void drawDecideInterlaceStatusForNextFrame()
{
  bool lace_bit = _core.RegisterUtility.IsInterlaceEnabled();

  interlace_status.frame_is_interlaced = lace_bit;
  if (interlace_status.frame_is_interlaced)
  {
    // Automatic long / short frame toggeling
    lof = lof ^ 0x8000;
  }

  interlace_status.frame_is_long = ((lof & 0x8000) == 0x8000);
  busSetScreenLimits(interlace_status.frame_is_long);

  bool use_interlaced_rendering = drawDecideUseInterlacedRendering();
  if (use_interlaced_rendering != interlace_status.use_interlaced_rendering)
  {

    if ((drawGetDisplayScaleStrategy() == DISPLAYSCALE_STRATEGY_SCANLINES) &&
      interlace_status.use_interlaced_rendering)
    {
      // Clear buffers when switching back to scanlines from interlaced rendering
      // to avoid a ghost image remaining in the scanlines.
      draw_clear_buffers = drawGetBufferCount();
    }

    interlace_status.use_interlaced_rendering = use_interlaced_rendering;
    drawReinitializeRendering();
  }

  //    _core.Log->AddLog("Frames are %s, frame no %I64d\n", (interlace_status.frame_is_long) ? "long" : "short", busGetRasterFrameCount());
}

void drawInterlaceStartup()
{
  interlace_status.frame_is_interlaced = false;
  interlace_status.frame_is_long = true;
  interlace_status.enable_deinterlace = true;
  interlace_status.use_interlaced_rendering = false;
}

void drawInterlaceEndOfFrame()
{
  drawDecideInterlaceStatusForNextFrame();
}

void drawSetDeinterlace(bool deinterlace)
{
  interlace_status.enable_deinterlace = deinterlace;
}