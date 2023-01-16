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

#include "draw_interlace_control.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/chipset/BitplaneRegisters.h"

bool InterlaceStatus::GetUseInterlacedRendering()
{
  return use_interlaced_rendering;
}

bool InterlaceStatus::GetFrameIsLong()
{
  return frame_is_long;
}

bool InterlaceStatus::DecideUseInterlacedRendering()
{
  return enable_deinterlace && frame_is_interlaced;
}

bool InterlaceStatus::GetFrameIsInterlaced()
{
  return frame_is_interlaced;
}

void InterlaceStatus::DecideInterlaceStatusForNextFrame()
{
  frame_is_interlaced = _bitplaneRegisters->IsInterlaced;
  if (frame_is_interlaced)
  {
    // Automatic long / short frame toggeling
    _bitplaneRegisters->ToggleLof();
  }

  frame_is_long = _bitplaneRegisters->lof;
  _scheduler->SetScreenLimits(frame_is_long);

  bool new_use_interlaced_rendering = DecideUseInterlacedRendering();
  if (new_use_interlaced_rendering != use_interlaced_rendering)
  {
    if ((display_scale_strategy == DisplayScaleStrategy::Scanlines) && use_interlaced_rendering)
    {
      // Clear buffers when switching back to scanlines from interlaced rendering
      // to avoid a ghost image remaining in the scanlines.
      Draw.SetClearAllBuffersFlag();
    }

    use_interlaced_rendering = new_use_interlaced_rendering;
    Draw.ReinitializeRendering();
  }
}

void InterlaceStatus::InterlaceStartup()
{
  frame_is_interlaced = false;
  frame_is_long = true;
  enable_deinterlace = true;
  use_interlaced_rendering = false;
}

void InterlaceStatus::InterlaceEndOfFrame()
{
  DecideInterlaceStatusForNextFrame();
}

void InterlaceStatus::InterlaceConfigure(bool deinterlace, DisplayScaleStrategy displayScaleStrategy)
{
  enable_deinterlace = deinterlace;
  display_scale_strategy = displayScaleStrategy;
}

InterlaceStatus::InterlaceStatus(Scheduler *scheduler, BitplaneRegisters *bitplaceRegisters) : _scheduler(scheduler), _bitplaneRegisters(bitplaceRegisters)
{
}
