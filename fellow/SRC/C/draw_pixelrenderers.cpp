/*=========================================================================*/
/* Fellow                                                                  */
/* Pixel renderer functions                                                */
/*                                                                         */
/* Authors: Petter Schau                                                   */
/*          Worfje                                                         */
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

#include "fellow/api/defs.h"
#include "GRAPH.H"
#include "fellow/application/HostRenderer.h"
#include "draw_interlace_control.h"
#include "LineExactSprites.h"
#include "fellow/chipset/DualPlayfieldMapper.h"

//==============================
// HAM color modify helper table
//==============================

uint32_t draw_HAM_modify_table[4][2];

// Indexes into the HAM drawing table
constexpr unsigned int draw_HAM_modify_table_bitindex = 0;
constexpr unsigned int draw_HAM_modify_table_holdmask = 4;

//=======================================================================
// Pointers to drawing routines that handle the drawing of Amiga graphics
// on the current host screen mode
//=======================================================================

draw_line_func draw_line_routine;
draw_line_func draw_line_BG_routine;
draw_line_func draw_line_BPL_manage_routine;
draw_line_BPL_segment_func draw_line_BPL_res_routine;
draw_line_BPL_segment_func draw_line_lores_routine;
draw_line_BPL_segment_func draw_line_hires_routine;
draw_line_BPL_segment_func draw_line_dual_lores_routine;
draw_line_BPL_segment_func draw_line_dual_hires_routine;
draw_line_BPL_segment_func draw_line_HAM_lores_routine;

//=================================
// Calulate data needed to draw HAM
//=================================

uint32_t drawMakeHoldMask(const unsigned int pos, const unsigned int size, const bool longdestination)
{
  uint32_t holdmask = 0;
  for (unsigned int i = pos; i < (pos + size); i++)
  {
    holdmask |= (1 << i);
  }

  return longdestination ? (~holdmask) : ((~holdmask) & 0xffff) | ((~holdmask) << 16);
}

void drawHAMTableInit(const GfxDrvColorBitsInformation &colorBitsInformation)
{
  const bool has24Or32BitPixels = colorBitsInformation.ColorBits > 16;

  draw_HAM_modify_table[0][0] = 0;
  draw_HAM_modify_table[0][1] = 0;
  draw_HAM_modify_table[1][0] = (uint32_t)(colorBitsInformation.BluePosition + colorBitsInformation.BlueSize - 4);
  draw_HAM_modify_table[1][1] = drawMakeHoldMask(colorBitsInformation.BluePosition, colorBitsInformation.BlueSize, has24Or32BitPixels);
  draw_HAM_modify_table[2][0] = (uint32_t)(colorBitsInformation.RedPosition + colorBitsInformation.RedSize - 4);
  draw_HAM_modify_table[2][1] = drawMakeHoldMask(colorBitsInformation.RedPosition, colorBitsInformation.RedSize, has24Or32BitPixels);
  draw_HAM_modify_table[3][0] = (uint32_t)(colorBitsInformation.GreenPosition + colorBitsInformation.GreenSize - 4);
  draw_HAM_modify_table[3][1] = drawMakeHoldMask(colorBitsInformation.GreenPosition, colorBitsInformation.GreenSize, has24Or32BitPixels);
}

const uint8_t *const drawGetDualTranslatePtr(const graph_line *const linedescription)
{
  return DualPlayfieldMapper::GetMappingPtr((linedescription->bplcon2 & 0x0040) == 0);
}

static uint32_t drawMake32BitColorFrom16Bit(const uint16_t color)
{
  return ((uint32_t)color) | ((uint32_t)color) << 16;
}

static uint64_t drawMake64BitColorFrom16Bit(const uint16_t color)
{
  return ((uint64_t)color) | ((uint64_t)color) << 16 | ((uint64_t)color) << 32 | ((uint64_t)color) << 48;
}

uint64_t drawMake64BitColorFrom32Bit(const uint32_t color)
{
  return ((uint64_t)color) | ((uint64_t)color) << 32;
}

static uint8_t drawGetDualColorIndex(const uint8_t *const dual_translate_ptr, const uint8_t playfield1_value, const uint8_t playfield2_value)
{
  return *(dual_translate_ptr + ((playfield1_value << 8) + playfield2_value));
}

static uint16_t drawGetDual16BitColor(const uint64_t *const colors, const uint8_t *const dual_translate_ptr, const uint8_t playfield1_value, const uint8_t playfield2_value)
{
  const uint8_t color_index = drawGetDualColorIndex(dual_translate_ptr, playfield1_value, playfield2_value);
  return (uint16_t)colors[color_index];
}

static uint32_t drawGetDual32BitColor(const uint64_t *const colors, const uint8_t *const dual_translate_ptr, const uint8_t playfield1_value, const uint8_t playfield2_value)
{
  const uint8_t color_index = drawGetDualColorIndex(dual_translate_ptr, playfield1_value, playfield2_value);
  return (uint32_t)colors[color_index];
}

static uint64_t drawGetDual64BitColor(const uint64_t *const colors, const uint8_t *const dual_translate_ptr, const uint8_t playfield1_value, const uint8_t playfield2_value)
{
  const uint8_t color_index = drawGetDualColorIndex(dual_translate_ptr, playfield1_value, playfield2_value);
  return colors[color_index];
}

static uint32_t drawUpdateHAMPixel(uint32_t hampixel, const uint8_t pixel_value)
{
  const uint8_t *const holdmask = ((uint8_t *)draw_HAM_modify_table + ((pixel_value & 0x30) >> 1));
  const uint32_t bitindex = *((uint32_t *)(holdmask + draw_HAM_modify_table_bitindex));
  hampixel &= *((uint32_t *)(holdmask + draw_HAM_modify_table_holdmask));
  return hampixel | ((pixel_value & 0xf) << (bitindex & 0xff));
}

static uint32_t drawMakeHAMPixel(const uint64_t *const colors, const uint32_t hampixel, const uint8_t pixel_value)
{
  return ((pixel_value & 0x30) == 0) ? (uint32_t)colors[pixel_value] : drawUpdateHAMPixel(hampixel, pixel_value);
}

static uint32_t drawProcessNonVisibleHAMPixels(const graph_line *const linedescription, int32_t pixel_count)
{
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DDF_start;
  uint32_t hampixel = 0;
  while (pixel_count-- > 0)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
  }
  return hampixel;
}

uint32_t GetFirstHamPixelFromInitialInvisibleHAMPixels(const graph_line *const linedescription)
{
  const int32_t non_visible_pixel_count = (int32_t)(linedescription->DIW_first_draw - linedescription->DDF_start);
  if (non_visible_pixel_count > 0)
  {
    return drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  return 0;
}

static void drawSetPixel1x1_16Bit(uint16_t *const framebuffer, const uint16_t pixel_color)
{
  framebuffer[0] = pixel_color;
}

static void drawSetPixel1x2_16Bit(uint16_t *const framebuffer, const ptrdiff_t nextlineoffset, const uint16_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset] = pixel_color;
}

static void drawSetPixel2x1_16Bit(uint32_t *const framebuffer, const uint32_t pixel_color)
{
  framebuffer[0] = pixel_color;
}

static void drawSetPixel2x2_16Bit(uint32_t *const framebuffer, const ptrdiff_t nextlineoffset, const uint32_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset] = pixel_color;
}

static void drawSetPixel2x4_16Bit(uint32_t *const framebuffer, const ptrdiff_t nextlineoffset1, const ptrdiff_t nextlineoffset2, const ptrdiff_t nextlineoffset3, const uint32_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
  framebuffer[nextlineoffset2] = pixel_color;
  framebuffer[nextlineoffset3] = pixel_color;
}

static void drawSetPixel4x2_16Bit(uint64_t *const framebuffer, const ptrdiff_t nextlineoffset, const uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset] = pixel_color;
}

static void drawSetPixel4x4_16Bit(uint64_t *const framebuffer, const ptrdiff_t nextlineoffset1, const ptrdiff_t nextlineoffset2, const ptrdiff_t nextlineoffset3, const uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
  framebuffer[nextlineoffset2] = pixel_color;
  framebuffer[nextlineoffset3] = pixel_color;
}

//==========================================================
// general function for drawing one line using normal pixels
// single pixels
// single lines
//
// pixel format: 15/16 bit RGB
//==========================================================

static uint8_t *drawLineNormal1x1_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint16_t *framebuffer = (uint16_t *)framebufferLinePtr;
  const uint16_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    const uint16_t pixel_color = (uint16_t)linedescription->colors[*source_ptr++];
    drawSetPixel1x1_16Bit(framebuffer++, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* single pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineNormal1x2_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint16_t *framebuffer = (uint16_t *)framebufferLinePtr;
  const uint16_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 2;

  while (framebuffer != framebuffer_end)
  {
    const uint16_t pixel_color = (uint16_t)linedescription->colors[*source_ptr++];
    drawSetPixel1x2_16Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineNormal2x1_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = (uint32_t)linedescription->colors[*source_ptr++];
    drawSetPixel2x1_16Bit(framebuffer++, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineNormal2x2_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = (uint32_t)linedescription->colors[*source_ptr++];
    drawSetPixel2x2_16Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineNormal2x4_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 4;
  const ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = (uint32_t)linedescription->colors[*source_ptr++];
    drawSetPixel2x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineNormal4x2_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = linedescription->colors[*source_ptr++];
    drawSetPixel4x2_16Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineNormal4x4_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  const ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = linedescription->colors[*source_ptr++];
    drawSetPixel4x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* single pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static uint8_t *drawLineDual1x1_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint16_t *framebuffer = (uint16_t *)framebufferLinePtr;
  const uint16_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    const uint16_t pixel_color = drawGetDual16BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel1x1_16Bit(framebuffer++, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* single pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static uint8_t *drawLineDual1x2_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint16_t *framebuffer = (uint16_t *)framebufferLinePtr;
  const uint16_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 2;

  while (framebuffer != framebuffer_end)
  {
    const uint16_t pixel_color = drawGetDual16BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel1x2_16Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static uint8_t *drawLineDual2x1_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = drawGetDual32BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x1_16Bit(framebuffer++, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static uint8_t *drawLineDual2x2_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = drawGetDual32BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x2_16Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static uint8_t *drawLineDual2x4_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 4;
  const ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = drawGetDual32BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static uint8_t *drawLineDual4x2_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = drawGetDual64BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel4x2_16Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static uint8_t *drawLineDual4x4_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  const ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = drawGetDual64BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel4x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static uint8_t *drawLineHAM2x1_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel2x1_16Bit(framebuffer++, drawMake32BitColorFrom16Bit(hampixel));
  }

  line_exact_sprites->MergeHAM2x1x16((uint32_t *)framebufferLinePtr, linedescription);

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static uint8_t *drawLineHAM2x2_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel2x2_16Bit(framebuffer++, nextlineoffset1, drawMake32BitColorFrom16Bit(hampixel));
  }

  line_exact_sprites->MergeHAM2x2x16((uint32_t *)framebufferLinePtr, linedescription, nextlineoffset1);

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static uint8_t *drawLineHAM4x2_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel4x2_16Bit(framebuffer++, nextlineoffset1, drawMake64BitColorFrom16Bit(hampixel));
  }

  line_exact_sprites->MergeHAM4x2x16((uint64_t *)framebufferLinePtr, linedescription, nextlineoffset1);

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static uint8_t *drawLineHAM4x4_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  const ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel4x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, drawMake64BitColorFrom16Bit(hampixel));
  }

  line_exact_sprites->MergeHAM4x4x16((uint64_t *)framebufferLinePtr, linedescription, nextlineoffset1, nextlineoffset2, nextlineoffset3);

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineSegmentBG2x1_16Bit(const uint32_t pixelcount, const uint32_t bgcolor, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + pixelcount;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel2x1_16Bit(framebuffer++, bgcolor);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineSegmentBG2x2_16Bit(const uint32_t pixelcount, const uint32_t bgcolor, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + pixelcount;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel2x2_16Bit(framebuffer++, nextlineoffset1, bgcolor);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineSegmentBG4x2_16Bit(const uint32_t pixelcount, const uint64_t bgcolor, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + pixelcount;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel4x2_16Bit(framebuffer++, nextlineoffset1, bgcolor);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineSegmentBG4x4_16Bit(const uint32_t pixelcount, const uint64_t bgcolor, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + pixelcount;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  const ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel4x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, bgcolor);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL2x1_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = drawLineSegmentBG2x1_16Bit(linedescription->BG_pad_front, (uint32_t)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  drawLineSegmentBG2x1_16Bit(linedescription->BG_pad_back, (uint32_t)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL2x2_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = drawLineSegmentBG2x2_16Bit(linedescription->BG_pad_front, (uint32_t)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  drawLineSegmentBG2x2_16Bit(linedescription->BG_pad_back, (uint32_t)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL4x2_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = drawLineSegmentBG4x2_16Bit(linedescription->BG_pad_front, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  drawLineSegmentBG4x2_16Bit(linedescription->BG_pad_back, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL4x4_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = drawLineSegmentBG4x4_16Bit(linedescription->BG_pad_front, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  drawLineSegmentBG4x4_16Bit(linedescription->BG_pad_back, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG2x1_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  drawLineSegmentBG2x1_16Bit(linedescription->MaxClipWidth / 4, (uint32_t)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG2x2_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  drawLineSegmentBG2x2_16Bit(linedescription->MaxClipWidth / 4, (uint32_t)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG4x2_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  drawLineSegmentBG4x2_16Bit(linedescription->MaxClipWidth / 4, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG4x4_16Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  drawLineSegmentBG4x4_16Bit(linedescription->MaxClipWidth / 4, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/* 24 Bit */

static void drawSetPixel1x1_24Bit(uint8_t *framebuffer, const uint32_t pixel_color)
{
  *((uint32_t *)framebuffer) = pixel_color;
}

static void drawSetPixel1x2_24Bit(uint8_t *framebuffer, const ptrdiff_t nextlineoffset, const uint32_t pixel_color)
{
  *((uint32_t *)framebuffer) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset)) = pixel_color;
}

static void drawSetPixel2x1_24Bit(uint8_t *framebuffer, const uint32_t pixel_color)
{
  *((uint32_t *)framebuffer) = pixel_color;
  *((uint32_t *)(framebuffer + 3)) = pixel_color;
}

static void drawSetPixel2x2_24Bit(uint8_t *framebuffer, const ptrdiff_t nextlineoffset, const uint32_t pixel_color)
{
  *((uint32_t *)framebuffer) = pixel_color;
  *((uint32_t *)(framebuffer + 3)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset + 3)) = pixel_color;
}

static void drawSetPixel2x4_24Bit(uint8_t *framebuffer, const ptrdiff_t nextlineoffset1, const ptrdiff_t nextlineoffset2, const ptrdiff_t nextlineoffset3, const uint32_t pixel_color)
{
  *((uint32_t *)framebuffer) = pixel_color;
  *((uint32_t *)(framebuffer + 3)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset1)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset1 + 3)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset2)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset2 + 3)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset3)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset3 + 3)) = pixel_color;
}

static void drawSetPixel4x2_24Bit(uint8_t *framebuffer, const ptrdiff_t nextlineoffset, const uint32_t pixel_color)
{
  *((uint32_t *)framebuffer) = pixel_color;
  *((uint32_t *)(framebuffer + 3)) = pixel_color;
  *((uint32_t *)(framebuffer + 6)) = pixel_color;
  *((uint32_t *)(framebuffer + 9)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset + 3)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset + 6)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset + 9)) = pixel_color;
}

static void drawSetPixel4x4_24Bit(uint8_t *framebuffer, const ptrdiff_t nextlineoffset1, const ptrdiff_t nextlineoffset2, const ptrdiff_t nextlineoffset3, const uint32_t pixel_color)
{
  *((uint32_t *)framebuffer) = pixel_color;
  *((uint32_t *)(framebuffer + 3)) = pixel_color;
  *((uint32_t *)(framebuffer + 6)) = pixel_color;
  *((uint32_t *)(framebuffer + 9)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset1)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset1 + 3)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset1 + 6)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset1 + 9)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset2)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset2 + 3)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset2 + 6)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset2 + 9)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset3)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset3 + 3)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset3 + 6)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset3 + 9)) = pixel_color;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* single pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal1x1_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 3;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = (uint32_t)linedescription->colors[*source_ptr++];
    drawSetPixel1x1_24Bit(framebuffer, pixel_color);
    framebuffer += 3;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* single pixels                                                                */
/* double lines
 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal1x2_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 3;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = (uint32_t)linedescription->colors[*source_ptr++];
    drawSetPixel1x2_24Bit(framebuffer, nextlineoffset, pixel_color);
    framebuffer += 3;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal2x1_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = (uint32_t)linedescription->colors[*source_ptr++];
    drawSetPixel2x1_24Bit(framebuffer, pixel_color);
    framebuffer += 6;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal2x2_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = (uint32_t)linedescription->colors[*source_ptr++];
    drawSetPixel2x2_24Bit(framebuffer, nextlineoffset, pixel_color);
    framebuffer += 6;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal2x4_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset2 = nextlineoffset * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = (uint32_t)linedescription->colors[*source_ptr++];
    drawSetPixel2x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, pixel_color);
    framebuffer += 6;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal4x2_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 12;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = (uint32_t)linedescription->colors[*source_ptr++];
    drawSetPixel4x2_24Bit(framebuffer, nextlineoffset, pixel_color);
    framebuffer += 12;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal4x4_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 12;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset2 = nextlineoffset * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = (uint32_t)linedescription->colors[*source_ptr++];
    drawSetPixel4x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, pixel_color);
    framebuffer += 12;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* single pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual1x1_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 3;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = drawGetDual32BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel1x1_24Bit(framebuffer, pixel_color);
    framebuffer += 3;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* single pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual1x2_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 3;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = drawGetDual32BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel1x2_24Bit(framebuffer, nextlineoffset, pixel_color);
    framebuffer += 3;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual2x1_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = drawGetDual32BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x1_24Bit(framebuffer, pixel_color);
    framebuffer += 6;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual2x2_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = drawGetDual32BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x2_24Bit(framebuffer, nextlineoffset, pixel_color);
    framebuffer += 6;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual2x4_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const ptrdiff_t nextlineoffset2 = nextlineoffset * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = drawGetDual32BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, pixel_color);
    framebuffer += 6;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual4x2_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 12;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = drawGetDual32BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel4x2_24Bit(framebuffer, nextlineoffset, pixel_color);
    framebuffer += 12;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual4x4_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 12;
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const ptrdiff_t nextlineoffset2 = nextlineoffset * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = drawGetDual32BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel4x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, pixel_color);
    framebuffer += 12;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineHAM2x1_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel2x1_24Bit(framebuffer, hampixel);
    framebuffer += 6;
  }

  line_exact_sprites->MergeHAM2x1x24(framebufferLinePtr, linedescription);

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineHAM2x2_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel2x2_24Bit(framebuffer, nextlineoffset, hampixel);
    framebuffer += 6;
  }

  line_exact_sprites->MergeHAM2x2x24(framebufferLinePtr, linedescription, nextlineoffset);

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineHAM4x2_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 12;
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel4x2_24Bit(framebuffer, nextlineoffset, hampixel);
    framebuffer += 12;
  }

  line_exact_sprites->MergeHAM4x2x24(framebufferLinePtr, linedescription, nextlineoffset);

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineHAM4x4_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 12;
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset2 = nextlineoffset * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel4x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, hampixel);
    framebuffer += 12;
  }

  line_exact_sprites->MergeHAM4x4x24(framebufferLinePtr, linedescription, nextlineoffset, nextlineoffset2, nextlineoffset3);

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineSegmentBG2x1_24Bit(const uint32_t pixelcount, const uint32_t bgcolor, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + pixelcount * 6;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel2x1_24Bit(framebuffer, bgcolor);
    framebuffer += 6;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineSegmentBG2x2_24Bit(const uint32_t pixelcount, const uint32_t bgcolor, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + pixelcount * 6;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel2x2_24Bit(framebuffer, nextlineoffset, bgcolor);
    framebuffer += 6;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineSegmentBG4x2_24Bit(const uint32_t pixelcount, const uint32_t bgcolor, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + pixelcount * 12;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel4x2_24Bit(framebuffer, nextlineoffset, bgcolor);
    framebuffer += 12;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineSegmentBG4x4_24Bit(const uint32_t pixelcount, const uint32_t bgcolor, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *const framebuffer_end = framebuffer + pixelcount * 12;
  const ptrdiff_t nextlineoffset2 = nextlineoffset * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel4x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, bgcolor);
    framebuffer += 12;
  }

  return framebuffer;
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL2x1_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = drawLineSegmentBG2x1_24Bit(linedescription->BG_pad_front, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  drawLineSegmentBG2x1_24Bit(linedescription->BG_pad_back, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL2x2_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = drawLineSegmentBG2x2_24Bit(linedescription->BG_pad_front, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  drawLineSegmentBG2x2_24Bit(linedescription->BG_pad_back, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL4x2_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = drawLineSegmentBG4x2_24Bit(linedescription->BG_pad_front, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  drawLineSegmentBG4x2_24Bit(linedescription->BG_pad_back, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL4x4_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = drawLineSegmentBG4x4_24Bit(linedescription->BG_pad_front, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  drawLineSegmentBG4x4_24Bit(linedescription->BG_pad_back, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG2x1_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  drawLineSegmentBG2x1_24Bit(linedescription->MaxClipWidth / 4, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG2x2_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  drawLineSegmentBG2x2_24Bit(linedescription->MaxClipWidth / 4, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG4x2_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  drawLineSegmentBG4x2_24Bit(linedescription->MaxClipWidth / 4, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG4x4_24Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  drawLineSegmentBG4x4_24Bit(linedescription->MaxClipWidth / 4, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/* 32-Bit */

static void drawSetPixel1x1_32Bit(uint32_t *framebuffer, const uint32_t pixel_color)
{
  framebuffer[0] = pixel_color;
}

static void drawSetPixel1x2_32Bit(uint32_t *framebuffer, const ptrdiff_t nextlineoffset1, const uint32_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
}

static void drawSetPixel2x1_32Bit(uint64_t *framebuffer, const uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
}

static void drawSetPixel2x2_32Bit(uint64_t *framebuffer, const ptrdiff_t nextlineoffset1, const uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
}

static void drawSetPixel2x4_32Bit(uint64_t *framebuffer, const ptrdiff_t nextlineoffset1, const ptrdiff_t nextlineoffset2, const ptrdiff_t nextlineoffset3, const uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
  framebuffer[nextlineoffset2] = pixel_color;
  framebuffer[nextlineoffset3] = pixel_color;
}

static void drawSetPixel4x2_32Bit(uint64_t *framebuffer, const ptrdiff_t nextlineoffset1, const uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[1] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
  framebuffer[nextlineoffset1 + 1] = pixel_color;
}

static void drawSetPixel4x4_32Bit(uint64_t *framebuffer, const ptrdiff_t nextlineoffset1, const ptrdiff_t nextlineoffset2, const ptrdiff_t nextlineoffset3, const uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[1] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
  framebuffer[nextlineoffset1 + 1] = pixel_color;
  framebuffer[nextlineoffset2] = pixel_color;
  framebuffer[nextlineoffset2 + 1] = pixel_color;
  framebuffer[nextlineoffset3] = pixel_color;
  framebuffer[nextlineoffset3 + 1] = pixel_color;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* single pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal1x1_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = (uint32_t)linedescription->colors[*source_ptr++];
    drawSetPixel1x1_32Bit(framebuffer++, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* single pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal1x2_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = (uint32_t)linedescription->colors[*source_ptr++];
    drawSetPixel1x2_32Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal2x1_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = linedescription->colors[*source_ptr++];
    drawSetPixel2x1_32Bit(framebuffer++, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal2x2_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = linedescription->colors[*source_ptr++];
    drawSetPixel2x2_32Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal2x4_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  const ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = linedescription->colors[*source_ptr++];
    drawSetPixel2x4_32Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal4x2_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 2;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = linedescription->colors[*source_ptr++];
    drawSetPixel4x2_32Bit(framebuffer, nextlineoffset1, pixel_color);
    framebuffer += 2;
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineNormal4x4_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 2;
  const uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  const ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = linedescription->colors[*source_ptr++];
    drawSetPixel4x4_32Bit(framebuffer, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
    framebuffer += 2;
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* single pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual1x1_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = drawGetDual32BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel1x1_32Bit(framebuffer++, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* single pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual1x2_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebuffer_end)
  {
    const uint32_t pixel_color = drawGetDual32BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel1x2_32Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual2x1_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = drawGetDual64BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x1_32Bit(framebuffer++, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual2x2_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = drawGetDual64BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x2_32Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual2x4_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  const ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = drawGetDual64BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x4_32Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual4x2_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 2;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = drawGetDual64BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel4x2_32Bit(framebuffer, nextlineoffset1, pixel_color);
    framebuffer += 2;
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineDual4x4_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 2;
  const uint8_t *const dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  const uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  const ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    const uint64_t pixel_color = drawGetDual64BitColor(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel4x4_32Bit(framebuffer, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
    framebuffer += 2;
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineHAM2x1_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel2x1_32Bit(framebuffer++, drawMake64BitColorFrom32Bit(hampixel));
  }

  line_exact_sprites->MergeHAM2x1x32((uint64_t *)framebufferLinePtr, linedescription);

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineHAM2x2_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel2x2_32Bit(framebuffer++, nextlineoffset1, drawMake64BitColorFrom32Bit(hampixel));
  }

  line_exact_sprites->MergeHAM2x2x32((uint64_t *)framebufferLinePtr, linedescription, nextlineoffset1);

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineHAM4x2_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 2;
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel4x2_32Bit(framebuffer, nextlineoffset1, drawMake64BitColorFrom32Bit(hampixel));
    framebuffer += 2;
  }

  line_exact_sprites->MergeHAM4x2x32((uint64_t *)framebufferLinePtr, linedescription, nextlineoffset1);

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static uint8_t *drawLineHAM4x4_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 2;
  const uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  const ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel4x4_32Bit(framebuffer, nextlineoffset1, nextlineoffset2, nextlineoffset3, drawMake64BitColorFrom32Bit(hampixel));
    framebuffer += 2;
  }

  line_exact_sprites->MergeHAM4x4x32((uint64_t *)framebufferLinePtr, linedescription, nextlineoffset1, nextlineoffset2, nextlineoffset3);
  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineSegmentBG2x1_32Bit(const uint32_t pixelcount, const uint64_t bgcolor, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + pixelcount;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel2x1_32Bit(framebuffer++, bgcolor);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineSegmentBG2x2_32Bit(const uint32_t pixelcount, const uint64_t bgcolor, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + pixelcount;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel2x2_32Bit(framebuffer++, nextlineoffset1, bgcolor);
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineSegmentBG4x2_32Bit(const uint32_t pixelcount, const uint64_t bgcolor, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + pixelcount * 2;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel4x2_32Bit(framebuffer, nextlineoffset1, bgcolor);
    framebuffer += 2;
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

static uint8_t *drawLineSegmentBG4x4_32Bit(const uint32_t pixelcount, const uint64_t bgcolor, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *const framebuffer_end = framebuffer + pixelcount * 2;
  const ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  const ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  const ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel4x4_32Bit(framebuffer, nextlineoffset1, nextlineoffset2, nextlineoffset3, bgcolor);
    framebuffer += 2;
  }

  return (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL2x1_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = drawLineSegmentBG2x1_32Bit(linedescription->BG_pad_front, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  drawLineSegmentBG2x1_32Bit(linedescription->BG_pad_back, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL2x2_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = drawLineSegmentBG2x2_32Bit(linedescription->BG_pad_front, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  drawLineSegmentBG2x2_32Bit(linedescription->BG_pad_back, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL4x2_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = drawLineSegmentBG4x2_32Bit(linedescription->BG_pad_front, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  drawLineSegmentBG4x2_32Bit(linedescription->BG_pad_back, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL4x4_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = drawLineSegmentBG4x4_32Bit(linedescription->BG_pad_front, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  drawLineSegmentBG4x4_32Bit(linedescription->BG_pad_back, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG2x1_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  drawLineSegmentBG2x1_32Bit(linedescription->MaxClipWidth / 4, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG2x2_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  drawLineSegmentBG2x2_32Bit(linedescription->MaxClipWidth / 4, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG4x2_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  drawLineSegmentBG4x2_32Bit(linedescription->MaxClipWidth / 4, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG4x4_32Bit(const graph_line *const linedescription, uint8_t *framebufferLinePtr, const ptrdiff_t nextlineoffset)
{
  drawLineSegmentBG4x4_32Bit(linedescription->MaxClipWidth / 4, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

/*============================================================================*/
/* Lookup tables that holds all the drawing routines for various Amiga and    */
/* host screen modes [color depth (3)][sizes (4)]                             */
/*============================================================================*/

draw_line_func draw_line_BPL_manage_funcs[3][4] = {{drawLineBPL2x1_16Bit, drawLineBPL2x2_16Bit, drawLineBPL4x2_16Bit, drawLineBPL4x4_16Bit},
                                                   {drawLineBPL2x1_24Bit, drawLineBPL2x2_24Bit, drawLineBPL4x2_24Bit, drawLineBPL4x4_24Bit},
                                                   {drawLineBPL2x1_32Bit, drawLineBPL2x2_32Bit, drawLineBPL4x2_32Bit, drawLineBPL4x4_32Bit}};

draw_line_func draw_line_BG_funcs[3][4] = {{drawLineBG2x1_16Bit, drawLineBG2x2_16Bit, drawLineBG4x2_16Bit, drawLineBG4x4_16Bit},
                                           {drawLineBG2x1_24Bit, drawLineBG2x2_24Bit, drawLineBG4x2_24Bit, drawLineBG4x4_24Bit},
                                           {drawLineBG2x1_32Bit, drawLineBG2x2_32Bit, drawLineBG4x2_32Bit, drawLineBG4x4_32Bit}};

draw_line_BPL_segment_func draw_line_lores_funcs[3][4] = {{drawLineNormal2x1_16Bit, drawLineNormal2x2_16Bit, drawLineNormal4x2_16Bit, drawLineNormal4x4_16Bit},
                                                          {drawLineNormal2x1_24Bit, drawLineNormal2x2_24Bit, drawLineNormal4x2_24Bit, drawLineNormal4x4_24Bit},
                                                          {drawLineNormal2x1_32Bit, drawLineNormal2x2_32Bit, drawLineNormal4x2_32Bit, drawLineNormal4x4_32Bit}};

draw_line_BPL_segment_func draw_line_hires_funcs[3][4] = {{drawLineNormal1x1_16Bit, drawLineNormal1x2_16Bit, drawLineNormal2x2_16Bit, drawLineNormal2x4_16Bit},
                                                          {drawLineNormal1x1_24Bit, drawLineNormal1x2_24Bit, drawLineNormal2x2_24Bit, drawLineNormal2x4_24Bit},
                                                          {drawLineNormal1x1_32Bit, drawLineNormal1x2_32Bit, drawLineNormal2x2_32Bit, drawLineNormal2x4_32Bit}};

draw_line_BPL_segment_func draw_line_dual_lores_funcs[3][4] = {{drawLineDual2x1_16Bit, drawLineDual2x2_16Bit, drawLineDual4x2_16Bit, drawLineDual4x4_16Bit},
                                                               {drawLineDual2x1_24Bit, drawLineDual2x2_24Bit, drawLineDual4x2_24Bit, drawLineDual4x4_24Bit},
                                                               {drawLineDual2x1_32Bit, drawLineDual2x2_32Bit, drawLineDual4x2_32Bit, drawLineDual4x4_32Bit}};

draw_line_BPL_segment_func draw_line_dual_hires_funcs[3][4] = {{drawLineDual1x1_16Bit, drawLineDual1x2_16Bit, drawLineDual2x2_16Bit, drawLineDual2x4_16Bit},
                                                               {drawLineDual1x1_24Bit, drawLineDual1x2_24Bit, drawLineDual2x2_24Bit, drawLineDual2x4_24Bit},
                                                               {drawLineDual1x1_32Bit, drawLineDual1x2_32Bit, drawLineDual2x2_32Bit, drawLineDual2x4_32Bit}};

draw_line_BPL_segment_func draw_line_HAM_lores_funcs[3][4] = {{drawLineHAM2x1_16Bit, drawLineHAM2x2_16Bit, drawLineHAM4x2_16Bit, drawLineHAM4x4_16Bit},
                                                              {drawLineHAM2x1_24Bit, drawLineHAM2x2_24Bit, drawLineHAM4x2_24Bit, drawLineHAM4x4_24Bit},
                                                              {drawLineHAM2x1_32Bit, drawLineHAM2x2_32Bit, drawLineHAM4x2_32Bit, drawLineHAM4x4_32Bit}};

unsigned int ColorBitsToFunctionLookupIndex(const unsigned int colorBits)
{
  if (colorBits == 15 || colorBits == 16)
  {
    return 0;
  }
  else if (colorBits == 24)
  {
    return 1;
  }

  return 2;
}

unsigned int ScaleFactorToFunctionLookupIndex(const ULO coreBufferScaleFactor, const bool useInterlacedRendering, const DisplayScaleStrategy displayScaleStrategy)
{
  if (useInterlacedRendering)
  {
    if (coreBufferScaleFactor == 1)
    {
      return 0; // 2x1
    }
    else
    {
      return 2; // 4x2
    }
  }

  // <Not interlaced>
  if (coreBufferScaleFactor == 1 && displayScaleStrategy == DisplayScaleStrategy::Scanlines)
  {
    return 0; // 2x1
  }
  else if (coreBufferScaleFactor == 1 && displayScaleStrategy == DisplayScaleStrategy::Solid)
  {
    return 1; // 2x2
  }
  else if (coreBufferScaleFactor == 2 && displayScaleStrategy == DisplayScaleStrategy::Scanlines)
  {
    return 2; // 4x2
  }
  else // if (scale_factor == 2 && displayScaleStrategy == DisplayScaleStrategy::Solid)
  {
    return 3; // 4x4
  }
}

void drawModeFunctionsInitialize(const unsigned int activeBufferColorBits, const unsigned int chipsetBufferScaleFactor, DisplayScaleStrategy displayScaleStrategy)
{
  const unsigned int colordepthIndex = ColorBitsToFunctionLookupIndex(activeBufferColorBits);
  const unsigned int scaleIndex = ScaleFactorToFunctionLookupIndex(chipsetBufferScaleFactor, drawGetUseInterlacedRendering(), displayScaleStrategy);

  // Main entry points that draws the entire line (BG padding - BPL segment - BG padding)
  draw_line_BPL_manage_routine = draw_line_BPL_manage_funcs[colordepthIndex][scaleIndex];
  draw_line_BG_routine = draw_line_BG_funcs[colordepthIndex][scaleIndex];
  draw_line_routine = draw_line_BG_routine; // Initialize with BG as default

  // Entry points for the BPL segment drawing
  draw_line_lores_routine = draw_line_lores_funcs[colordepthIndex][scaleIndex];
  draw_line_hires_routine = draw_line_hires_funcs[colordepthIndex][scaleIndex];
  draw_line_dual_lores_routine = draw_line_dual_lores_funcs[colordepthIndex][scaleIndex];
  draw_line_dual_hires_routine = draw_line_dual_hires_funcs[colordepthIndex][scaleIndex];
  draw_line_HAM_lores_routine = draw_line_HAM_lores_funcs[colordepthIndex][scaleIndex];
  draw_line_BPL_res_routine = draw_line_lores_routine; // Initialize with lores as default
}

void drawSetLineRoutine(draw_line_func drawLineFunction)
{
  draw_line_routine = drawLineFunction;
}
