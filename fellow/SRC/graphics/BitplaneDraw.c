/*=========================================================================*/
/* Fellow                                                                  */
/* Bitplane draw (in host buffer)                                          */
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

#include "bus.h"
#include "graph.h"
#include "draw.h"
#include "fmem.h"
#include "fileops.h"

#include "Graphics.h"

extern UBY *draw_buffer_current_ptr;
extern UBY draw_dual_translate[2][256][256];
extern ULO draw_HAM_modify_table[4][2];

#define draw_HAM_modify_table_bitindex 0
#define draw_HAM_modify_table_holdmask 4

void BitplaneDraw::TempLores(ULO rasterY, ULO pixel_index, ULO pixel_count)
{
  ULO *tmpline = _tmpframe[rasterY] + pixel_index;
  UBY *playfield = GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield();

  for (ULO i = 0; i < pixel_count; i++)
  {
    ULO pixel_color = graph_color_shadow[playfield[i]>>2];

    tmpline[0] = pixel_color;
    tmpline[1] = pixel_color;
    tmpline += 2;
  }
}

void BitplaneDraw::TempLoresDual(ULO rasterY, ULO pixel_index, ULO pixel_count)
{
  UBY *draw_dual_translate_ptr = (UBY *) draw_dual_translate;
  ULO *tmpline = _tmpframe[rasterY] + pixel_index;

  if (BitplaneUtility::IsPlayfield1Pri())
  {
    draw_dual_translate_ptr += 0x10000;
  }

  UBY *playfield_odd = GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield();
  UBY *playfield_even = GraphicsContext.Planar2ChunkyDecoder.GetEvenPlayfield();
  for (ULO i = 0; i < pixel_count; i++)
  {
    ULO pixel_color = graph_color_shadow[(*(draw_dual_translate_ptr + playfield_odd[i]*256 + playfield_even[i]))>>2];
    tmpline[0] = pixel_color;
    tmpline[1] = pixel_color;
    tmpline += 2;
  }
}

void BitplaneDraw::TempLoresHam(ULO rasterY, ULO pixel_index, ULO pixel_count)
{
  ULO bitindex;
  UBY *holdmask;
  ULO *tmpline = _tmpframe[rasterY] + pixel_index;
  static ULO pixel_color;
  UBY* playfield = GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield();

  for (ULO i = 0; i < pixel_count; i++)
  {
    if ((GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield()[i] & 0xc0) == 0)
    {
      pixel_color = graph_color_shadow[playfield[i]>>2];
    }
    else
    {
      holdmask = ((UBY *) draw_HAM_modify_table + ((playfield[i] & 0xc0) >> 3));
      bitindex = *((ULO *) (holdmask + draw_HAM_modify_table_bitindex));
      pixel_color &= *((ULO *) (holdmask + draw_HAM_modify_table_holdmask));
      pixel_color |= (((playfield[i] & 0x3c) >> 2) << (bitindex & 0xff));
    }
    tmpline[0] = pixel_color;
    tmpline[1] = pixel_color;
    tmpline += 2;
  }
}

void BitplaneDraw::TempHires(ULO rasterY, ULO pixel_index, ULO pixel_count)
{
  ULO *tmpline = _tmpframe[rasterY] + pixel_index;
  UBY *playfield = GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield();

  for (ULO i = 0; i < pixel_count; i++)
  {
    ULO pixel_color = graph_color_shadow[playfield[i]>>2];
    tmpline[i] = pixel_color;
  }
}

void BitplaneDraw::TempHiresDual(ULO rasterY, ULO pixel_index, ULO pixel_count)
{
  UBY *draw_dual_translate_ptr = (UBY *) draw_dual_translate;
  ULO *tmpline = _tmpframe[rasterY] + pixel_index;

  if (BitplaneUtility::IsPlayfield1Pri())
  {
    draw_dual_translate_ptr += 0x10000;
  }

  UBY *playfield_odd = GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield();
  UBY *playfield_even = GraphicsContext.Planar2ChunkyDecoder.GetEvenPlayfield();
  for (ULO i = 0; i < pixel_count; i++)
  {
    ULO pixel_color = graph_color_shadow[(*(draw_dual_translate_ptr + playfield_odd[i]*256 + playfield_even[i]))>>2];
    tmpline[i] = pixel_color;
  }
}

void BitplaneDraw::TempNothing(ULO rasterY, ULO pixel_index, ULO pixel_count)
{
  ULO *tmpline = _tmpframe[rasterY] + pixel_index;
  ULO pixel_color = graph_color_shadow[0];

  for (ULO i = 0; i < pixel_count; i++)
  {
    tmpline[0] = pixel_color;
    tmpline[1] = pixel_color;
    tmpline += 2;
  }
}

void BitplaneDraw::DrawBatch(ULO rasterY, ULO start_cylinder)
{
  ULO pixel_index = start_cylinder*2;
  ULO pixel_count = GraphicsContext.Planar2ChunkyDecoder.GetBatchSize();

  if (!GraphicsContext.DIWXStateMachine.IsVisible())
  {
    TempNothing(rasterY, pixel_index, pixel_count);
    return;
  }

  if (BitplaneUtility::IsHires())
  {
    if (BitplaneUtility::IsDualPlayfield())
    {
      TempHiresDual(rasterY, pixel_index, pixel_count);
    }
    else
    {
      TempHires(rasterY, pixel_index, pixel_count);
    }
  }
  else
  {
    if (BitplaneUtility::IsDualPlayfield())
    {
      TempLoresDual(rasterY, pixel_index, pixel_count);
    }
    else
    {
      if (BitplaneUtility::IsHam())
      {
	TempLoresHam(rasterY, pixel_index, pixel_count);
      }
      else
      {
	TempLores(rasterY, pixel_index, pixel_count);
      }
    }
  }
}

void BitplaneDraw::TmpFrame(ULO next_line_offset)
{
  ULO real_pitch_in_bytes = next_line_offset/2;

  ULO *draw_buffer_first_ptr_local = (ULO *) draw_buffer_current_ptr;
  ULO *draw_buffer_second_ptr_local = (ULO *) (draw_buffer_current_ptr + real_pitch_in_bytes);

  ULO startx = draw_left*2;
  ULO stopx = draw_right*2;

  for (ULO y = draw_top; y < draw_bottom; y++)
  {
    ULO *tmpline = _tmpframe[y];
    for (ULO x = startx; x < stopx; x++)
    {
      ULO index = (x-startx);
      ULO pixel_color = tmpline[x];
      draw_buffer_first_ptr_local[index] = pixel_color;
      draw_buffer_second_ptr_local[index] = pixel_color;
    }
    draw_buffer_first_ptr_local += (real_pitch_in_bytes/2);
    draw_buffer_second_ptr_local += (real_pitch_in_bytes/2);
  }
}

BitplaneDraw::BitplaneDraw()
{
  _tmpframe = new ULO[313][1024];
}

BitplaneDraw::~BitplaneDraw()
{
  delete[] _tmpframe;
}
