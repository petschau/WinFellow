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

#include "Defs.h"

#include "GraphicsPipeline.h"
#include "Renderer.h"
#include "MemoryInterface.h"
#include "Graphics.h"

extern uint8_t draw_dual_translate[2][256][256];
extern uint32_t draw_HAM_modify_table[4][2];

#define draw_HAM_modify_table_bitindex 0
#define draw_HAM_modify_table_holdmask 4

void BitplaneDraw::TempLores(uint32_t rasterY, uint32_t pixel_index, uint32_t pixel_count)
{
  uint32_t *tmpline = _tmpframe[rasterY] + pixel_index;
  uint8_t *playfield = GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield();

  for (uint32_t i = 0; i < pixel_count; i++)
  {
    uint32_t pixel_color = graph_color_shadow[playfield[i] >> 2];

    tmpline[0] = pixel_color;
    tmpline[1] = pixel_color;
    tmpline += 2;
  }
}

void BitplaneDraw::TempLoresDual(uint32_t rasterY, uint32_t pixel_index, uint32_t pixel_count)
{
  uint8_t *draw_dual_translate_ptr = (uint8_t *)draw_dual_translate;
  uint32_t *tmpline = _tmpframe[rasterY] + pixel_index;

  if (_core.RegisterUtility.IsPlayfield1PriorityEnabled())
  {
    draw_dual_translate_ptr += 0x10000;
  }

  uint8_t *playfield_odd = GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield();
  uint8_t *playfield_even = GraphicsContext.Planar2ChunkyDecoder.GetEvenPlayfield();
  for (uint32_t i = 0; i < pixel_count; i++)
  {
    uint32_t pixel_color = graph_color_shadow[(*(draw_dual_translate_ptr + playfield_odd[i] * 256 + playfield_even[i])) >> 2];
    tmpline[0] = pixel_color;
    tmpline[1] = pixel_color;
    tmpline += 2;
  }
}

void BitplaneDraw::TempLoresHam(uint32_t rasterY, uint32_t pixel_index, uint32_t pixel_count)
{
  uint32_t bitindex;
  uint8_t *holdmask;
  uint32_t *tmpline = _tmpframe[rasterY] + pixel_index;
  static uint32_t pixel_color;
  uint8_t *playfield = GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield();

  for (uint32_t i = 0; i < pixel_count; i++)
  {
    if ((GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield()[i] & 0xc0) == 0)
    {
      pixel_color = graph_color_shadow[playfield[i] >> 2];
    }
    else
    {
      holdmask = ((uint8_t *)draw_HAM_modify_table + ((playfield[i] & 0xc0) >> 3));
      bitindex = *((uint32_t *)(holdmask + draw_HAM_modify_table_bitindex));
      pixel_color &= *((uint32_t *)(holdmask + draw_HAM_modify_table_holdmask));
      pixel_color |= (((playfield[i] & 0x3c) >> 2) << (bitindex & 0xff));
    }
    tmpline[0] = pixel_color;
    tmpline[1] = pixel_color;
    tmpline += 2;
  }
}

void BitplaneDraw::TempHires(uint32_t rasterY, uint32_t pixel_index, uint32_t pixel_count)
{
  uint32_t *tmpline = _tmpframe[rasterY] + pixel_index;
  uint8_t *playfield = GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield();

  for (uint32_t i = 0; i < pixel_count; i++)
  {
    uint32_t pixel_color = graph_color_shadow[playfield[i] >> 2];
    tmpline[i] = pixel_color;
  }
}

void BitplaneDraw::TempHiresDual(uint32_t rasterY, uint32_t pixel_index, uint32_t pixel_count)
{
  uint8_t *draw_dual_translate_ptr = (uint8_t *)draw_dual_translate;
  uint32_t *tmpline = _tmpframe[rasterY] + pixel_index;

  if (_core.RegisterUtility.IsPlayfield1PriorityEnabled())
  {
    draw_dual_translate_ptr += 0x10000;
  }

  uint8_t *playfield_odd = GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield();
  uint8_t *playfield_even = GraphicsContext.Planar2ChunkyDecoder.GetEvenPlayfield();
  for (uint32_t i = 0; i < pixel_count; i++)
  {
    uint32_t pixel_color = graph_color_shadow[(*(draw_dual_translate_ptr + playfield_odd[i] * 256 + playfield_even[i])) >> 2];
    tmpline[i] = pixel_color;
  }
}

void BitplaneDraw::TempNothing(uint32_t rasterY, uint32_t pixel_index, uint32_t pixel_count)
{
  uint32_t *tmpline = _tmpframe[rasterY] + pixel_index;
  uint32_t pixel_color = graph_color_shadow[0];

  for (uint32_t i = 0; i < pixel_count; i++)
  {
    tmpline[0] = pixel_color;
    tmpline[1] = pixel_color;
    tmpline += 2;
  }
}

void BitplaneDraw::DrawBatch(uint32_t rasterY, uint32_t start_cylinder)
{
  uint32_t pixel_index = start_cylinder * 2;
  uint32_t pixel_count = GraphicsContext.Planar2ChunkyDecoder.GetBatchSize();

  if (!GraphicsContext.DIWXStateMachine.IsVisible())
  {
    TempNothing(rasterY, pixel_index, pixel_count);
    return;
  }

  if (_core.RegisterUtility.IsHiresEnabled())
  {
    if (_core.RegisterUtility.IsDualPlayfieldEnabled())
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
    if (_core.RegisterUtility.IsDualPlayfieldEnabled())
    {
      TempLoresDual(rasterY, pixel_index, pixel_count);
    }
    else
    {
      if (_core.RegisterUtility.IsHAMEnabled())
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

void BitplaneDraw::TmpFrame(uint32_t next_line_offset)
{
  uint32_t real_pitch_in_bytes = next_line_offset / 2;

  uint32_t *draw_buffer_first_ptr_local = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *draw_buffer_second_ptr_local = (uint32_t *)(draw_buffer_info.current_ptr + real_pitch_in_bytes);

  uint32_t startx = drawGetInternalClip().left * 2;
  uint32_t stopx = drawGetInternalClip().right * 2;

  for (uint32_t y = drawGetInternalClip().top; y < drawGetInternalClip().bottom; y++)
  {
    uint32_t *tmpline = _tmpframe[y];
    for (uint32_t x = startx; x < stopx; x++)
    {
      uint32_t index = (x - startx);
      uint32_t pixel_color = tmpline[x];
      draw_buffer_first_ptr_local[index] = pixel_color;
      draw_buffer_second_ptr_local[index] = pixel_color;
    }
    draw_buffer_first_ptr_local += (real_pitch_in_bytes / 2);
    draw_buffer_second_ptr_local += (real_pitch_in_bytes / 2);
  }
}

BitplaneDraw::BitplaneDraw()
{
  _tmpframe = new uint32_t[313][1024];
}

BitplaneDraw::~BitplaneDraw()
{
  delete[] _tmpframe;
}
