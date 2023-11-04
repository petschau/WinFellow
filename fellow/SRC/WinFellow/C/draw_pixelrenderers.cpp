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

#include "Defs.h"
#include "GRAPH.H"
#include "Renderer.h"
#include "draw_interlace_control.h"
#include "LineExactSprites.h"

/*============================================================================*/
/* Dual playfield translation table                                           */
/* Syntax: dualtranslate[0 - PF1 behind, 1 - PF2 behind][PF1data][PF2data]    */
/*============================================================================*/

uint8_t draw_dual_translate[2][256][256];

/*============================================================================*/
/* HAM color modify helper table                                              */
/*============================================================================*/

uint32_t draw_HAM_modify_table[4][2];

// Indexes into the HAM drawing table
const uint32_t draw_HAM_modify_table_bitindex = 0;
const uint32_t draw_HAM_modify_table_holdmask = 4;

/*============================================================================*/
/* Calulate data needed to draw HAM                                           */
/*============================================================================*/

static uint32_t drawMakeHoldMask(uint32_t pos, uint32_t size, bool longdestination)
{
  uint32_t holdmask = 0;
  for (uint32_t i = pos; i < (pos + size); i++)
  {
    holdmask |= (1 << i);
  }
  if (!longdestination)
  {
    return ((~holdmask) & 0xffff) | ((~holdmask) << 16);
  }
  return (~holdmask);
}

void drawHAMTableInit()
{
  draw_HAM_modify_table[0][0] = 0;
  draw_HAM_modify_table[0][1] = 0;

  bool longdestination = (draw_buffer_info.bits > 16);
  draw_HAM_modify_table[1][0] = draw_buffer_info.bluepos + draw_buffer_info.bluesize - 4; /* Blue */
  draw_HAM_modify_table[1][1] = drawMakeHoldMask(draw_buffer_info.bluepos, draw_buffer_info.bluesize, longdestination);
  draw_HAM_modify_table[2][0] = draw_buffer_info.redpos + draw_buffer_info.redsize - 4; /* Red */
  draw_HAM_modify_table[2][1] = drawMakeHoldMask(draw_buffer_info.redpos, draw_buffer_info.redsize, longdestination);
  draw_HAM_modify_table[3][0] = draw_buffer_info.greenpos + draw_buffer_info.greensize - 4; /* Green */
  draw_HAM_modify_table[3][1] = drawMakeHoldMask(draw_buffer_info.greenpos, draw_buffer_info.greensize, longdestination);
}

/*============================================================================*/
/* Initializes the dual playfield translation table                           */
/*============================================================================*/

void drawDualTranslationInitialize()
{
  int32_t l;

  for (int32_t k = 0; k < 2; k++)
  {
    for (int32_t i = 0; i < 256; i++)
    {
      for (int32_t j = 0; j < 256; j++)
      {
        if (k == 0)
        { /* PF1 behind, PF2 in front */
          if (j == 0)
            l = i; /* PF2 transparent, PF1 visible */
          else
          { /* PF2 visible */
            /* If color is higher than 0x3c it is a sprite */
            if (j < 0x40)
            {
              l = j + 0x20;
            }
            else
            {
              l = j;
            }
          }
        }
        else
        { /* PF1 in front, PF2 behind */
          if (i == 0)
          { /* PF1 transparent, PF2 visible */
            if (j == 0)
            {
              l = 0;
            }
            else
            {
              if (j < 0x40)
              {
                l = j + 0x20;
              }
              else
              {
                l = j;
              }
            }
          }
          else
          {
            l = i; /* PF1 visible amd not transparent */
          }
        }
        draw_dual_translate[k][i][j] = (uint8_t)l;
      }
    }
  }
}

static uint8_t *drawGetDualTranslatePtr(graph_line *linedescription)
{
  uint8_t *draw_dual_translate_ptr = (uint8_t *)draw_dual_translate;

  if ((linedescription->bplcon2 & 0x40) == 0)
  {
    // bit 6 of BPLCON2 is not set, thus playfield 1 is in front of playfield 2
    // write results in draw_dual_translate[1], instead of draw_dual_translate[0]
    draw_dual_translate_ptr += 0x10000;
  }
  return draw_dual_translate_ptr;
}

static uint32_t drawMake32BitColorFrom16Bit(uint16_t color)
{
  uint32_t color32 = ((uint32_t)color) | ((uint32_t)color) << 16;
  return color32;
}

static uint64_t drawMake64BitColorFrom16Bit(uint16_t color)
{
  uint64_t color64 = ((uint64_t)color) | ((uint64_t)color) << 16 | ((uint64_t)color) << 32 | ((uint64_t)color) << 48;
  return color64;
}

uint64_t drawMake64BitColorFrom32Bit(uint32_t color)
{
  uint64_t color64 = ((uint64_t)color) | ((uint64_t)color) << 32;
  return color64;
}

static uint16_t drawGetColorUint16(uint32_t *colors, uint8_t color_index)
{
  return *((uint16_t *)((uint8_t *)colors + color_index));
}

static uint32_t drawGetColorUint32(uint32_t *colors, uint8_t color_index)
{
  return *((uint32_t *)((uint8_t *)colors + color_index));
}

static uint64_t drawGetColorULL(uint32_t *colors, uint8_t color_index)
{
  uint32_t pixel_color = drawGetColorUint32(colors, color_index);
  return drawMake64BitColorFrom32Bit(pixel_color);
}

static uint8_t drawGetDualColorIndex(uint8_t *dual_translate_ptr, uint8_t playfield1_value, uint8_t playfield2_value)
{
  return *(dual_translate_ptr + ((playfield1_value << 8) + playfield2_value));
}

static uint16_t drawGetDualColorUint16(uint32_t *colors, uint8_t *dual_translate_ptr, uint8_t playfield1_value, uint8_t playfield2_value)
{
  uint8_t color_index = drawGetDualColorIndex(dual_translate_ptr, playfield1_value, playfield2_value);
  return drawGetColorUint16(colors, color_index);
}

static uint32_t drawGetDualColorUint32(uint32_t *colors, uint8_t *dual_translate_ptr, uint8_t playfield1_value, uint8_t playfield2_value)
{
  uint8_t color_index = drawGetDualColorIndex(dual_translate_ptr, playfield1_value, playfield2_value);
  return drawGetColorUint32(colors, color_index);
}

static uint64_t drawGetDualColorULL(uint32_t *colors, uint8_t *dual_translate_ptr, uint8_t playfield1_value, uint8_t playfield2_value)
{
  uint8_t color_index = drawGetDualColorIndex(dual_translate_ptr, playfield1_value, playfield2_value);
  return drawGetColorULL(colors, color_index);
}

static uint32_t drawUpdateHAMPixel(uint32_t hampixel, uint8_t pixel_value)
{
  uint8_t *holdmask = ((uint8_t *)draw_HAM_modify_table + ((pixel_value & 0xc0) >> 3));
  uint32_t bitindex = *((uint32_t *)(holdmask + draw_HAM_modify_table_bitindex));
  hampixel &= *((uint32_t *)(holdmask + draw_HAM_modify_table_holdmask));
  hampixel |= (((pixel_value & 0x3c) >> 2) << (bitindex & 0xff));
  return hampixel;
}

static uint32_t drawMakeHAMPixel(uint32_t *colors, uint32_t hampixel, uint8_t pixel_value)
{
  if ((pixel_value & 0xc0) == 0)
  {
    return drawGetColorUint32(colors, pixel_value);
  }
  return drawUpdateHAMPixel(hampixel, pixel_value);
}

static uint32_t drawProcessNonVisibleHAMPixels(graph_line *linedescription, int32_t pixel_count)
{
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DDF_start;
  uint32_t hampixel = 0;
  while (pixel_count-- > 0)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
  }
  return hampixel;
}

static void drawSetPixel1x1_16Bit(uint16_t *framebuffer, uint16_t pixel_color)
{
  framebuffer[0] = pixel_color;
}

static void drawSetPixel1x2_16Bit(uint16_t *framebuffer, uint32_t nextlineoffset, uint16_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset] = pixel_color;
}

static void drawSetPixel2x1_16Bit(uint32_t *framebuffer, uint32_t pixel_color)
{
  framebuffer[0] = pixel_color;
}

static void drawSetPixel2x2_16Bit(uint32_t *framebuffer, uint32_t nextlineoffset, uint32_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset] = pixel_color;
}

static void drawSetPixel2x4_16Bit(uint32_t *framebuffer, uint32_t nextlineoffset1, uint32_t nextlineoffset2, uint32_t nextlineoffset3, uint32_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
  framebuffer[nextlineoffset2] = pixel_color;
  framebuffer[nextlineoffset3] = pixel_color;
}

static void drawSetPixel4x2_16Bit(uint64_t *framebuffer, uint32_t nextlineoffset, uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset] = pixel_color;
}

static void drawSetPixel4x4_16Bit(uint64_t *framebuffer, uint32_t nextlineoffset1, uint32_t nextlineoffset2, uint32_t nextlineoffset3, uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
  framebuffer[nextlineoffset2] = pixel_color;
  framebuffer[nextlineoffset3] = pixel_color;
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* single pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static void drawLineNormal1x1_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln1x1_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln1x1_16bit_tmp);
#endif

  uint16_t *framebuffer = (uint16_t *)draw_buffer_info.current_ptr;
  uint16_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    uint16_t pixel_color = drawGetColorUint16(linedescription->colors, *source_ptr++);
    drawSetPixel1x1_16Bit(framebuffer++, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln1x1_16bit_tmp, &dln1x1_16bit, &dln1x1_16bit_times);
#endif
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* single pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static void drawLineNormal1x2_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln1x2_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln1x2_16bit_tmp);
#endif

  uint16_t *framebuffer = (uint16_t *)draw_buffer_info.current_ptr;
  uint16_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 2;

  while (framebuffer != framebuffer_end)
  {
    uint16_t pixel_color = drawGetColorUint16(linedescription->colors, *source_ptr++);
    drawSetPixel1x2_16Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln1x2_16bit_tmp, &dln1x2_16bit, &dln1x2_16bit_times);
#endif
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static void drawLineNormal2x1_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln2x1_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln2x1_16bit_tmp);
#endif

  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetColorUint32(linedescription->colors, *source_ptr++);
    drawSetPixel2x1_16Bit(framebuffer++, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln2x1_16bit_tmp, &dln2x1_16bit, &dln2x1_16bit_times);
#endif
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static void drawLineNormal2x2_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln2x2_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln2x2_16bit_tmp);
#endif

  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetColorUint32(linedescription->colors, *source_ptr++);
    drawSetPixel2x2_16Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln2x2_16bit_tmp, &dln2x2_16bit, &dln2x2_16bit_times);
#endif
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static void drawLineNormal2x4_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln2x4_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln2x4_16bit_tmp);
#endif

  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 4;
  uint32_t nextlineoffset2 = nextlineoffset1 * 2;
  uint32_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetColorUint32(linedescription->colors, *source_ptr++);
    drawSetPixel2x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln2x4_16bit_tmp, &dln2x4_16bit, &dln2x4_16bit_times);
#endif
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static void drawLineNormal4x2_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln4x2_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln4x2_16bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetColorULL(linedescription->colors, *source_ptr++);
    drawSetPixel4x2_16Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln4x2_16bit_tmp, &dln4x2_16bit, &dln4x2_16bit_times);
#endif
}

/*==============================================================================*/
/* general function for drawing one line using normal pixels                    */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* pixel format: 15/16 bit RGB                                                  */
/*==============================================================================*/

static void drawLineNormal4x4_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln4x4_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln4x4_16bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint32_t nextlineoffset2 = nextlineoffset1 * 2;
  uint32_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetColorULL(linedescription->colors, *source_ptr++);
    drawSetPixel4x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln4x4_16bit_tmp, &dln4x4_16bit, &dln4x4_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* single pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static void drawLineDual1x1_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld1x1_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld1x1_16bit_tmp);
#endif

  uint16_t *framebuffer = (uint16_t *)draw_buffer_info.current_ptr;
  uint16_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    uint16_t pixel_color = drawGetDualColorUint16(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel1x1_16Bit(framebuffer++, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld1x1_16bit_tmp, &dld1x1_16bit, &dld1x1_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* single pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static void drawLineDual1x2_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld1x2_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld1x2_16bit_tmp);
#endif

  uint16_t *framebuffer = (uint16_t *)draw_buffer_info.current_ptr;
  uint16_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint32_t nextlineoffset1 = nextlineoffset / 2;

  while (framebuffer != framebuffer_end)
  {
    uint16_t pixel_color = drawGetDualColorUint16(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel1x2_16Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld1x2_16bit_tmp, &dld1x2_16bit, &dld1x2_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static void drawLineDual2x1_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld2x1_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld2x1_16bit_tmp);
#endif

  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetDualColorUint32(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x1_16Bit(framebuffer++, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld2x1_16bit_tmp, &dld2x1_16bit, &dld2x1_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static void drawLineDual2x2_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld2x2_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld2x2_16bit_tmp);
#endif

  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint32_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetDualColorUint32(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x2_16Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld2x2_16bit_tmp, &dld2x2_16bit, &dld2x2_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static void drawLineDual2x4_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld2x4_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld2x4_16bit_tmp);
#endif

  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint32_t nextlineoffset1 = nextlineoffset / 4;
  uint32_t nextlineoffset2 = nextlineoffset1 * 2;
  uint32_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetDualColorUint32(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld2x4_16bit_tmp, &dld2x4_16bit, &dld2x4_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static void drawLineDual4x2_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld4x2_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld4x2_16bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint32_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetDualColorULL(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel4x2_16Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld4x2_16bit_tmp, &dld4x2_16bit, &dld4x2_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static void drawLineDual4x4_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld4x4_16bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld4x4_16bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint32_t nextlineoffset2 = nextlineoffset1 * 2;
  uint32_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetDualColorULL(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel4x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld4x4_16bit_tmp, &dld4x4_16bit, &dld4x4_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static void drawLineHAM2x1_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  int32_t tscnonvisible = linedescription->DIW_first_draw - linedescription->DDF_start;
  dlh2x1_16bit_pixels += linedescription->DIW_pixel_count + ((tscnonvisible > 0) ? tscnonvisible : 0);
  drawTscBefore(&dlh2x1_16bit_tmp);
#endif

  uint32_t hampixel = 0;
  int32_t non_visible_pixel_count = linedescription->DIW_first_draw - linedescription->DDF_start;
  if (non_visible_pixel_count > 0)
  {
    hampixel = drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel2x1_16Bit(framebuffer++, drawMake32BitColorFrom16Bit(hampixel));
  }

  line_exact_sprites->MergeHAM2x1x16((uint32_t *)draw_buffer_current_ptr_local, linedescription);

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlh2x1_16bit_tmp, &dlh2x1_16bit, &dlh2x1_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static void drawLineHAM2x2_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  int32_t tscnonvisible = linedescription->DIW_first_draw - linedescription->DDF_start;
  dlh2x2_16bit_pixels += linedescription->DIW_pixel_count + ((tscnonvisible > 0) ? tscnonvisible : 0);
  drawTscBefore(&dlh2x2_16bit_tmp);
#endif

  uint32_t hampixel = 0;
  int32_t non_visible_pixel_count = linedescription->DIW_first_draw - linedescription->DDF_start;
  if (non_visible_pixel_count > 0)
  {
    hampixel = drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel2x2_16Bit(framebuffer++, nextlineoffset1, drawMake32BitColorFrom16Bit(hampixel));
  }

  line_exact_sprites->MergeHAM2x2x16((uint32_t *)draw_buffer_current_ptr_local, linedescription, nextlineoffset1);

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlh2x2_16bit_tmp, &dlh2x2_16bit, &dlh2x2_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static void drawLineHAM4x2_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  int32_t tscnonvisible = linedescription->DIW_first_draw - linedescription->DDF_start;
  dlh4x2_16bit_pixels += linedescription->DIW_pixel_count + ((tscnonvisible > 0) ? tscnonvisible : 0);
  drawTscBefore(&dlh4x2_16bit_tmp);
#endif

  uint32_t hampixel = 0;
  int32_t non_visible_pixel_count = linedescription->DIW_first_draw - linedescription->DDF_start;
  if (non_visible_pixel_count > 0)
  {
    hampixel = drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel4x2_16Bit(framebuffer++, nextlineoffset1, drawMake64BitColorFrom16Bit(hampixel));
  }

  line_exact_sprites->MergeHAM4x2x16((uint64_t *)draw_buffer_current_ptr_local, linedescription, nextlineoffset1);

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlh4x2_16bit_tmp, &dlh4x2_16bit, &dlh4x2_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     15/16 bit RGB                                              */
/*==============================================================================*/

static void drawLineHAM4x4_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  int32_t tscnonvisible = linedescription->DIW_first_draw - linedescription->DDF_start;
  dlh4x4_16bit_pixels += linedescription->DIW_pixel_count + ((tscnonvisible > 0) ? tscnonvisible : 0);
  drawTscBefore(&dlh4x4_16bit_tmp);
#endif

  uint32_t hampixel = 0;
  int32_t non_visible_pixel_count = linedescription->DIW_first_draw - linedescription->DDF_start;
  if (non_visible_pixel_count > 0)
  {
    hampixel = drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint32_t nextlineoffset2 = nextlineoffset1 * 2;
  uint32_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel4x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, drawMake64BitColorFrom16Bit(hampixel));
  }

  line_exact_sprites->MergeHAM4x4x16((uint64_t *)draw_buffer_current_ptr_local, linedescription, nextlineoffset1, nextlineoffset2, nextlineoffset3);

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlh4x4_16bit_tmp, &dlh4x4_16bit, &dlh4x4_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

static void drawLineSegmentBG2x1_16Bit(uint32_t pixelcount, uint32_t bgcolor, uint32_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + pixelcount;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel2x1_16Bit(framebuffer++, bgcolor);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

static void drawLineSegmentBG2x2_16Bit(uint32_t pixelcount, uint32_t bgcolor, uint32_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + pixelcount;
  uint32_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel2x2_16Bit(framebuffer++, nextlineoffset1, bgcolor);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

static void drawLineSegmentBG4x2_16Bit(uint32_t pixelcount, uint32_t bgcolor, uint32_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + pixelcount;
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint64_t bgcolor64 = drawMake64BitColorFrom32Bit(bgcolor);

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel4x2_16Bit(framebuffer++, nextlineoffset1, bgcolor64);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

static void drawLineSegmentBG4x4_16Bit(uint32_t pixelcount, uint32_t bgcolor, uint32_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + pixelcount;
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint32_t nextlineoffset2 = nextlineoffset1 * 2;
  uint32_t nextlineoffset3 = nextlineoffset1 * 3;
  uint64_t bgcolor64 = drawMake64BitColorFrom32Bit(bgcolor);

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel4x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, bgcolor64);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL2x1_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
  drawLineSegmentBG2x1_16Bit(linedescription->BG_pad_front, linedescription->colors[0], nextlineoffset);
  ((draw_line_func)(linedescription->draw_line_BPL_res_routine))(linedescription, nextlineoffset);
  drawLineSegmentBG2x1_16Bit(linedescription->BG_pad_back, linedescription->colors[0], nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL2x2_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
  drawLineSegmentBG2x2_16Bit(linedescription->BG_pad_front, linedescription->colors[0], nextlineoffset);
  ((draw_line_func)(linedescription->draw_line_BPL_res_routine))(linedescription, nextlineoffset);
  drawLineSegmentBG2x2_16Bit(linedescription->BG_pad_back, linedescription->colors[0], nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL4x2_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
  drawLineSegmentBG4x2_16Bit(linedescription->BG_pad_front, linedescription->colors[0], nextlineoffset);
  ((draw_line_func)(linedescription->draw_line_BPL_res_routine))(linedescription, nextlineoffset);
  drawLineSegmentBG4x2_16Bit(linedescription->BG_pad_back, linedescription->colors[0], nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL4x4_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
  drawLineSegmentBG4x4_16Bit(linedescription->BG_pad_front, linedescription->colors[0], nextlineoffset);
  ((draw_line_func)(linedescription->draw_line_BPL_res_routine))(linedescription, nextlineoffset);
  drawLineSegmentBG4x4_16Bit(linedescription->BG_pad_back, linedescription->colors[0], nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG2x1_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dlsbg2x1_16bit_pixels += drawGetClipWidth();
  drawTscBefore(&dlsbg2x1_16bit_tmp);
#endif

  drawLineSegmentBG2x1_16Bit(drawGetInternalClip().GetWidth(), linedescription->colors[0], nextlineoffset);

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlsbg2x1_16bit_tmp, &dlsbg2x1_16bit, &dlsbg2x1_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG2x2_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dlsbg2x2_16bit_pixels += drawGetClipWidth();
  drawTscBefore(&dlsbg2x2_16bit_tmp);
#endif

  drawLineSegmentBG2x2_16Bit(drawGetInternalClip().GetWidth(), linedescription->colors[0], nextlineoffset);

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlsbg2x2_16bit_tmp, &dlsbg2x2_16bit, &dlsbg2x2_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG4x2_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dlsbg4x2_16bit_pixels += drawGetClipWidth();
  drawTscBefore(&dlsbg4x2_16bit_tmp);
#endif

  drawLineSegmentBG4x2_16Bit(drawGetInternalClip().GetWidth(), linedescription->colors[0], nextlineoffset);

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlsbg4x2_16bit_tmp, &dlsbg4x2_16bit, &dlsbg4x2_16bit_times);
#endif
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    16 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG4x4_16Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dlsbg4x4_16bit_pixels += drawGetClipWidth();
  drawTscBefore(&dlsbg4x4_16bit_tmp);
#endif

  drawLineSegmentBG4x4_16Bit(drawGetInternalClip().GetWidth(), linedescription->colors[0], nextlineoffset);

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlsbg4x4_16bit_tmp, &dlsbg4x4_16bit, &dlsbg4x4_16bit_times);
#endif
}

/* 24 Bit */

static void drawSetPixel1x1_24Bit(uint8_t *framebuffer, uint32_t pixel_color)
{
  *((uint32_t *)framebuffer) = pixel_color;
}

static void drawSetPixel1x2_24Bit(uint8_t *framebuffer, uint32_t nextlineoffset, uint32_t pixel_color)
{
  *((uint32_t *)framebuffer) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset)) = pixel_color;
}

static void drawSetPixel2x1_24Bit(uint8_t *framebuffer, uint32_t pixel_color)
{
  *((uint32_t *)framebuffer) = pixel_color;
  *((uint32_t *)(framebuffer + 3)) = pixel_color;
}

static void drawSetPixel2x2_24Bit(uint8_t *framebuffer, uint32_t nextlineoffset, uint32_t pixel_color)
{
  *((uint32_t *)framebuffer) = pixel_color;
  *((uint32_t *)(framebuffer + 3)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset)) = pixel_color;
  *((uint32_t *)(framebuffer + nextlineoffset + 3)) = pixel_color;
}

static void drawSetPixel2x4_24Bit(uint8_t *framebuffer, uint32_t nextlineoffset1, uint32_t nextlineoffset2, uint32_t nextlineoffset3, uint32_t pixel_color)
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

static void drawSetPixel4x2_24Bit(uint8_t *framebuffer, uint32_t nextlineoffset, uint32_t pixel_color)
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

static void drawSetPixel4x4_24Bit(uint8_t *framebuffer, uint32_t nextlineoffset1, uint32_t nextlineoffset2, uint32_t nextlineoffset3, uint32_t pixel_color)
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

static void drawLineNormal1x1_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln1x1_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln1x1_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 3;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetColorUint32(linedescription->colors, *source_ptr++);
    drawSetPixel1x1_24Bit(framebuffer, pixel_color);
    framebuffer += 3;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln1x1_24bit_tmp, &dln1x1_24bit, &dln1x1_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* single pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineNormal1x2_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln1x2_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln1x2_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 3;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetColorUint32(linedescription->colors, *source_ptr++);
    drawSetPixel1x2_24Bit(framebuffer, nextlineoffset, pixel_color);
    framebuffer += 3;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln1x2_24bit_tmp, &dln1x2_24bit, &dln1x2_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineNormal2x1_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln2x1_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln2x1_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetColorUint32(linedescription->colors, *source_ptr++);
    drawSetPixel2x1_24Bit(framebuffer, pixel_color);
    framebuffer += 6;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln2x1_24bit_tmp, &dln2x1_24bit, &dln2x1_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineNormal2x2_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln2x2_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln2x2_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetColorUint32(linedescription->colors, *source_ptr++);
    drawSetPixel2x2_24Bit(framebuffer, nextlineoffset, pixel_color);
    framebuffer += 6;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln2x2_24bit_tmp, &dln2x2_24bit, &dln2x2_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineNormal2x4_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln2x4_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln2x4_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset2 = nextlineoffset * 2;
  uint32_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetColorUint32(linedescription->colors, *source_ptr++);
    drawSetPixel2x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, pixel_color);
    framebuffer += 6;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln2x4_24bit_tmp, &dln2x4_24bit, &dln2x4_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineNormal4x2_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln4x2_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln4x2_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 12;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetColorUint32(linedescription->colors, *source_ptr++);
    drawSetPixel4x2_24Bit(framebuffer, nextlineoffset, pixel_color);
    framebuffer += 12;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln4x2_24bit_tmp, &dln4x2_24bit, &dln4x2_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineNormal4x4_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln4x4_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln4x4_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 12;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset2 = nextlineoffset * 2;
  uint32_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetColorUint32(linedescription->colors, *source_ptr++);
    drawSetPixel4x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, pixel_color);
    framebuffer += 12;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln4x4_24bit_tmp, &dln4x4_24bit, &dln4x4_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* single pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual1x1_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld1x1_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld1x1_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 3;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetDualColorUint32(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel1x1_24Bit(framebuffer, pixel_color);
    framebuffer += 3;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld1x1_24bit_tmp, &dld1x1_24bit, &dld1x1_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* single pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual1x2_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld1x2_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld1x2_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 3;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetDualColorUint32(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel1x2_24Bit(framebuffer, nextlineoffset, pixel_color);
    framebuffer += 3;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld1x2_24bit_tmp, &dld1x2_24bit, &dld1x2_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual2x1_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld2x1_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld2x1_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetDualColorUint32(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x1_24Bit(framebuffer, pixel_color);
    framebuffer += 6;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld2x1_24bit_tmp, &dld2x1_24bit, &dld2x1_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual2x2_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld2x2_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld2x2_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetDualColorUint32(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x2_24Bit(framebuffer, nextlineoffset, pixel_color);
    framebuffer += 6;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld2x2_24bit_tmp, &dld2x2_24bit, &dld2x2_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual2x4_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld2x4_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld2x4_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint32_t nextlineoffset2 = nextlineoffset * 2;
  uint32_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetDualColorUint32(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, pixel_color);
    framebuffer += 6;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld2x4_24bit_tmp, &dld2x4_24bit, &dld2x4_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual4x2_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld4x2_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld4x2_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 12;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetDualColorUint32(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel4x2_24Bit(framebuffer, nextlineoffset, pixel_color);
    framebuffer += 12;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld4x2_24bit_tmp, &dld4x2_24bit, &dld4x2_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual4x4_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld4x4_24bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld4x4_24bit_tmp);
#endif

  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 12;
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint32_t nextlineoffset2 = nextlineoffset * 2;
  uint32_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetDualColorUint32(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel4x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, pixel_color);
    framebuffer += 12;
  }

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld4x4_24bit_tmp, &dld4x4_24bit, &dld4x4_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineHAM2x1_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  int32_t tscnonvisible = linedescription->DIW_first_draw - linedescription->DDF_start;
  dlh2x1_24bit_pixels += linedescription->DIW_pixel_count + ((tscnonvisible > 0) ? tscnonvisible : 0);
  drawTscBefore(&dlh2x1_24bit_tmp);
#endif

  uint32_t hampixel = 0;
  int32_t non_visible_pixel_count = linedescription->DIW_first_draw - linedescription->DDF_start;
  if (non_visible_pixel_count > 0)
  {
    hampixel = drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel2x1_24Bit(framebuffer, hampixel);
    framebuffer += 6;
  }

  line_exact_sprites->MergeHAM2x1x24(draw_buffer_current_ptr_local, linedescription);

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlh2x1_24bit_tmp, &dlh2x1_24bit, &dlh2x1_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineHAM2x2_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  int32_t tscnonvisible = linedescription->DIW_first_draw - linedescription->DDF_start;
  dlh2x2_24bit_pixels += linedescription->DIW_pixel_count + ((tscnonvisible > 0) ? tscnonvisible : 0);
  drawTscBefore(&dlh2x2_24bit_tmp);
#endif

  uint32_t hampixel = 0;
  int32_t non_visible_pixel_count = linedescription->DIW_first_draw - linedescription->DDF_start;
  if (non_visible_pixel_count > 0)
  {
    hampixel = drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 6;
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel2x2_24Bit(framebuffer, nextlineoffset, hampixel);
    framebuffer += 6;
  }

  line_exact_sprites->MergeHAM2x2x24(draw_buffer_current_ptr_local, linedescription, nextlineoffset);

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlh2x2_24bit_tmp, &dlh2x2_24bit, &dlh2x2_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineHAM4x2_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  int32_t tscnonvisible = linedescription->DIW_first_draw - linedescription->DDF_start;
  dlh4x2_24bit_pixels += linedescription->DIW_pixel_count + ((tscnonvisible > 0) ? tscnonvisible : 0);
  drawTscBefore(&dlh4x2_24bit_tmp);
#endif

  uint32_t hampixel = 0;
  int32_t non_visible_pixel_count = linedescription->DIW_first_draw - linedescription->DDF_start;
  if (non_visible_pixel_count > 0)
  {
    hampixel = drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 12;
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel4x2_24Bit(framebuffer, nextlineoffset, hampixel);
    framebuffer += 12;
  }

  line_exact_sprites->MergeHAM4x2x24(draw_buffer_current_ptr_local, linedescription, nextlineoffset);

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlh4x2_24bit_tmp, &dlh4x2_24bit, &dlh4x2_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     24 bit RGB                                                 */
/*==============================================================================*/

static void drawLineHAM4x4_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  int32_t tscnonvisible = linedescription->DIW_first_draw - linedescription->DDF_start;
  dlh4x4_24bit_pixels += linedescription->DIW_pixel_count + ((tscnonvisible > 0) ? tscnonvisible : 0);
  drawTscBefore(&dlh4x4_24bit_tmp);
#endif

  uint32_t hampixel = 0;
  int32_t non_visible_pixel_count = linedescription->DIW_first_draw - linedescription->DDF_start;
  if (non_visible_pixel_count > 0)
  {
    hampixel = drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 12;
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset2 = nextlineoffset * 2;
  uint32_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel4x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, hampixel);
    framebuffer += 12;
  }

  line_exact_sprites->MergeHAM4x4x24(draw_buffer_current_ptr_local, linedescription, nextlineoffset, nextlineoffset2, nextlineoffset3);

  draw_buffer_info.current_ptr = framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlh4x4_24bit_tmp, &dlh4x4_24bit, &dlh4x4_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

static void drawLineSegmentBG2x1_24Bit(uint32_t pixelcount, uint32_t bgcolor, uint32_t nextlineoffset)
{
  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + pixelcount * 6;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel2x1_24Bit(framebuffer, bgcolor);
    framebuffer += 6;
  }

  draw_buffer_info.current_ptr = framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

static void drawLineSegmentBG2x2_24Bit(uint32_t pixelcount, uint32_t bgcolor, uint32_t nextlineoffset)
{
  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + pixelcount * 6;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel2x2_24Bit(framebuffer, nextlineoffset, bgcolor);
    framebuffer += 6;
  }

  draw_buffer_info.current_ptr = framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

static void drawLineSegmentBG4x2_24Bit(uint32_t pixelcount, uint32_t bgcolor, uint32_t nextlineoffset)
{
  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + pixelcount * 12;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel4x2_24Bit(framebuffer, nextlineoffset, bgcolor);
    framebuffer += 12;
  }

  draw_buffer_info.current_ptr = framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

static void drawLineSegmentBG4x4_24Bit(uint32_t pixelcount, uint32_t bgcolor, uint32_t nextlineoffset)
{
  uint8_t *framebuffer = draw_buffer_info.current_ptr;
  uint8_t *framebuffer_end = framebuffer + pixelcount * 12;
  uint32_t nextlineoffset2 = nextlineoffset * 2;
  uint32_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel4x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, bgcolor);
    framebuffer += 12;
  }

  draw_buffer_info.current_ptr = framebuffer;
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL2x1_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
  drawLineSegmentBG2x1_24Bit(linedescription->BG_pad_front, linedescription->colors[0], nextlineoffset);
  ((draw_line_func)(linedescription->draw_line_BPL_res_routine))(linedescription, nextlineoffset);
  drawLineSegmentBG2x1_24Bit(linedescription->BG_pad_back, linedescription->colors[0], nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL2x2_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
  drawLineSegmentBG2x2_24Bit(linedescription->BG_pad_front, linedescription->colors[0], nextlineoffset);
  ((draw_line_func)(linedescription->draw_line_BPL_res_routine))(linedescription, nextlineoffset);
  drawLineSegmentBG2x2_24Bit(linedescription->BG_pad_back, linedescription->colors[0], nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL4x2_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
  drawLineSegmentBG4x2_24Bit(linedescription->BG_pad_front, linedescription->colors[0], nextlineoffset);
  ((draw_line_func)(linedescription->draw_line_BPL_res_routine))(linedescription, nextlineoffset);
  drawLineSegmentBG4x2_24Bit(linedescription->BG_pad_back, linedescription->colors[0], nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL4x4_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
  drawLineSegmentBG4x4_24Bit(linedescription->BG_pad_front, linedescription->colors[0], nextlineoffset);
  ((draw_line_func)(linedescription->draw_line_BPL_res_routine))(linedescription, nextlineoffset);
  drawLineSegmentBG4x4_24Bit(linedescription->BG_pad_back, linedescription->colors[0], nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG2x1_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dlsbg2x1_24bit_pixels += drawGetClipWidth();
  drawTscBefore(&dlsbg2x1_24bit_tmp);
#endif

  drawLineSegmentBG2x1_24Bit(drawGetInternalClip().GetWidth(), linedescription->colors[0], nextlineoffset);

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlsbg2x1_24bit_tmp, &dlsbg2x1_24bit, &dlsbg2x1_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG2x2_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dlsbg2x2_24bit_pixels += drawGetClipWidth();
  drawTscBefore(&dlsbg2x2_24bit_tmp);
#endif

  drawLineSegmentBG2x2_24Bit(drawGetInternalClip().GetWidth(), linedescription->colors[0], nextlineoffset);

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlsbg2x2_24bit_tmp, &dlsbg2x2_24bit, &dlsbg2x2_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG4x2_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dlsbg4x2_24bit_pixels += drawGetClipWidth();
  drawTscBefore(&dlsbg4x2_24bit_tmp);
#endif

  drawLineSegmentBG4x2_24Bit(drawGetInternalClip().GetWidth(), linedescription->colors[0], nextlineoffset);

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlsbg4x2_24bit_tmp, &dlsbg4x2_24bit, &dlsbg4x2_24bit_times);
#endif
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    24 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG4x4_24Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dlsbg4x4_24bit_pixels += drawGetClipWidth();
  drawTscBefore(&dlsbg4x4_24bit_tmp);
#endif

  drawLineSegmentBG4x4_24Bit(drawGetInternalClip().GetWidth(), linedescription->colors[0], nextlineoffset);

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlsbg4x4_24bit_tmp, &dlsbg4x4_24bit, &dlsbg4x4_24bit_times);
#endif
}

/* 32-Bit */

static void drawSetPixel1x1_32Bit(uint32_t *framebuffer, uint32_t pixel_color)
{
  framebuffer[0] = pixel_color;
}

static void drawSetPixel1x2_32Bit(uint32_t *framebuffer, uint32_t nextlineoffset1, uint32_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
}

static void drawSetPixel2x1_32Bit(uint64_t *framebuffer, uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
}

static void drawSetPixel2x2_32Bit(uint64_t *framebuffer, uint32_t nextlineoffset1, uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
}

static void drawSetPixel2x4_32Bit(uint64_t *framebuffer, uint32_t nextlineoffset1, uint32_t nextlineoffset2, uint32_t nextlineoffset3, uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
  framebuffer[nextlineoffset2] = pixel_color;
  framebuffer[nextlineoffset3] = pixel_color;
}

static void drawSetPixel4x2_32Bit(uint64_t *framebuffer, uint32_t nextlineoffset1, uint64_t pixel_color)
{
  framebuffer[0] = pixel_color;
  framebuffer[1] = pixel_color;
  framebuffer[nextlineoffset1] = pixel_color;
  framebuffer[nextlineoffset1 + 1] = pixel_color;
}

static void drawSetPixel4x4_32Bit(uint64_t *framebuffer, uint32_t nextlineoffset1, uint32_t nextlineoffset2, uint32_t nextlineoffset3, uint64_t pixel_color)
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

static void drawLineNormal1x1_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln1x1_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln1x1_32bit_tmp);
#endif

  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetColorUint32(linedescription->colors, *source_ptr++);
    drawSetPixel1x1_32Bit(framebuffer++, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln1x1_32bit_tmp, &dln1x1_32bit, &dln1x1_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* single pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineNormal1x2_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln1x2_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln1x2_32bit_tmp);
#endif

  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetColorUint32(linedescription->colors, *source_ptr++);
    drawSetPixel1x2_32Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln1x2_32bit_tmp, &dln1x2_32bit, &dln1x2_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineNormal2x1_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln2x1_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln2x1_32bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetColorULL(linedescription->colors, *source_ptr++);
    drawSetPixel2x1_32Bit(framebuffer++, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln2x1_32bit_tmp, &dln2x1_32bit, &dln2x1_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineNormal2x2_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln2x2_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln2x2_32bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetColorULL(linedescription->colors, *source_ptr++);
    drawSetPixel2x2_32Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln2x2_32bit_tmp, &dln2x2_32bit, &dln2x2_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineNormal2x4_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln2x4_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln2x4_32bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint32_t nextlineoffset2 = nextlineoffset1 * 2;
  uint32_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetColorULL(linedescription->colors, *source_ptr++);
    drawSetPixel2x4_32Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln2x4_32bit_tmp, &dln2x4_32bit, &dln2x4_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineNormal4x2_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln4x2_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln4x2_32bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 2;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetColorULL(linedescription->colors, *source_ptr++);
    drawSetPixel4x2_32Bit(framebuffer, nextlineoffset1, pixel_color);
    framebuffer += 2;
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln4x2_32bit_tmp, &dln4x2_32bit, &dln4x2_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line using normal pixels                                            */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineNormal4x4_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dln4x4_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dln4x4_32bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 2;
  uint8_t *source_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint32_t nextlineoffset2 = nextlineoffset1 * 2;
  uint32_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetColorULL(linedescription->colors, *source_ptr++);
    drawSetPixel4x4_32Bit(framebuffer, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
    framebuffer += 2;
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dln4x4_32bit_tmp, &dln4x4_32bit, &dln4x4_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* single pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual1x1_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld1x1_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld1x1_32bit_tmp);
#endif

  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetDualColorUint32(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel1x1_32Bit(framebuffer++, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld1x1_32bit_tmp, &dld1x1_32bit, &dld1x1_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* single pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual1x2_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld1x2_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld1x2_32bit_tmp);
#endif

  uint32_t *framebuffer = (uint32_t *)draw_buffer_info.current_ptr;
  uint32_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebuffer_end)
  {
    uint32_t pixel_color = drawGetDualColorUint32(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel1x2_32Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld1x2_32bit_tmp, &dld1x2_32bit, &dld1x2_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual2x1_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld2x1_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld2x1_32bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetDualColorULL(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x1_32Bit(framebuffer++, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld2x1_32bit_tmp, &dld2x1_32bit, &dld2x1_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual2x2_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld2x2_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld2x2_32bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetDualColorULL(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x2_32Bit(framebuffer++, nextlineoffset1, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld2x2_32bit_tmp, &dld2x2_32bit, &dld2x2_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* double pixels                                                                */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual2x4_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld2x4_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld2x4_32bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint32_t nextlineoffset2 = nextlineoffset1 * 2;
  uint32_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetDualColorULL(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel2x4_32Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld2x4_32bit_tmp, &dld2x4_32bit, &dld2x4_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual4x2_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld4x2_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld4x2_32bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 2;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetDualColorULL(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel4x2_32Bit(framebuffer, nextlineoffset1, pixel_color);
    framebuffer += 2;
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld4x2_32bit_tmp, &dld4x2_32bit, &dld4x2_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line mixing two playfields                                          */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineDual4x4_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dld4x4_32bit_pixels += linedescription->DIW_pixel_count;
  drawTscBefore(&dld4x4_32bit_tmp);
#endif

  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + linedescription->DIW_pixel_count * 2;
  uint8_t *dual_translate_ptr = drawGetDualTranslatePtr(linedescription);
  uint8_t *source_line1_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint8_t *source_line2_ptr = linedescription->line2 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint32_t nextlineoffset2 = nextlineoffset1 * 2;
  uint32_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    uint64_t pixel_color = drawGetDualColorULL(linedescription->colors, dual_translate_ptr, *source_line1_ptr++, *source_line2_ptr++);
    drawSetPixel4x4_32Bit(framebuffer, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixel_color);
    framebuffer += 2;
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dld4x4_32bit_tmp, &dld4x4_32bit, &dld4x4_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* double pixels                                                                */
/* single lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineHAM2x1_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  int32_t tscnonvisible = linedescription->DIW_first_draw - linedescription->DDF_start;
  dlh2x1_32bit_pixels += linedescription->DIW_pixel_count + ((tscnonvisible > 0) ? tscnonvisible : 0);
  drawTscBefore(&dlh2x1_32bit_tmp);
#endif

  uint32_t hampixel = 0;
  int32_t non_visible_pixel_count = linedescription->DIW_first_draw - linedescription->DDF_start;
  if (non_visible_pixel_count > 0)
  {
    hampixel = drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = (uint64_t *)draw_buffer_info.current_ptr + linedescription->DIW_pixel_count;
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel2x1_32Bit(framebuffer++, drawMake64BitColorFrom32Bit(hampixel));
  }

  line_exact_sprites->MergeHAM2x1x32((uint64_t *)draw_buffer_current_ptr_local, linedescription);

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlh2x1_32bit_tmp, &dlh2x1_32bit, &dlh2x1_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* double pixels                                                                */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineHAM2x2_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  int32_t tscnonvisible = linedescription->DIW_first_draw - linedescription->DDF_start;
  dlh2x2_32bit_pixels += linedescription->DIW_pixel_count + ((tscnonvisible > 0) ? tscnonvisible : 0);
  drawTscBefore(&dlh2x2_32bit_tmp);
#endif

  uint32_t hampixel = 0;
  int32_t non_visible_pixel_count = linedescription->DIW_first_draw - linedescription->DDF_start;
  if (non_visible_pixel_count > 0)
  {
    hampixel = drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = (uint64_t *)draw_buffer_info.current_ptr + linedescription->DIW_pixel_count;
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel2x2_32Bit(framebuffer++, nextlineoffset1, drawMake64BitColorFrom32Bit(hampixel));
  }

  line_exact_sprites->MergeHAM2x2x32(reinterpret_cast<uint64_t *>(draw_buffer_current_ptr_local), linedescription, nextlineoffset1);

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlh2x2_32bit_tmp, &dlh2x2_32bit, &dlh2x2_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* quad pixels                                                                  */
/* double lines                                                                 */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineHAM4x2_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  int32_t tscnonvisible = linedescription->DIW_first_draw - linedescription->DDF_start;
  dlh4x2_32bit_pixels += linedescription->DIW_pixel_count + ((tscnonvisible > 0) ? tscnonvisible : 0);
  drawTscBefore(&dlh4x2_32bit_tmp);
#endif

  uint32_t hampixel = 0;
  int32_t non_visible_pixel_count = linedescription->DIW_first_draw - linedescription->DDF_start;
  if (non_visible_pixel_count > 0)
  {
    hampixel = drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = (uint64_t *)draw_buffer_info.current_ptr + linedescription->DIW_pixel_count * 2;
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel4x2_32Bit(framebuffer, nextlineoffset1, drawMake64BitColorFrom32Bit(hampixel));
    framebuffer += 2;
  }

  line_exact_sprites->MergeHAM4x2x32(reinterpret_cast<uint64_t *>(draw_buffer_current_ptr_local), linedescription, nextlineoffset1);

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlh4x2_32bit_tmp, &dlh4x2_32bit, &dlh4x2_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line of HAM data                                                    */
/* quad pixels                                                                  */
/* quad lines                                                                   */
/*                                                                              */
/* Pixel format:     32 bit RGB                                                 */
/*==============================================================================*/

static void drawLineHAM4x4_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  int32_t tscnonvisible = linedescription->DIW_first_draw - linedescription->DDF_start;
  dlh4x4_32bit_pixels += linedescription->DIW_pixel_count + ((tscnonvisible > 0) ? tscnonvisible : 0);
  drawTscBefore(&dlh4x4_32bit_tmp);
#endif

  uint32_t hampixel = 0;
  int32_t non_visible_pixel_count = linedescription->DIW_first_draw - linedescription->DDF_start;
  if (non_visible_pixel_count > 0)
  {
    hampixel = drawProcessNonVisibleHAMPixels(linedescription, non_visible_pixel_count);
  }

  uint8_t *draw_buffer_current_ptr_local = draw_buffer_info.current_ptr;
  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = (uint64_t *)draw_buffer_info.current_ptr + linedescription->DIW_pixel_count * 2;
  uint8_t *source_line_ptr = linedescription->line1 + linedescription->DIW_first_draw;
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint32_t nextlineoffset2 = nextlineoffset1 * 2;
  uint32_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebuffer_end)
  {
    hampixel = drawMakeHAMPixel(linedescription->colors, hampixel, *source_line_ptr++);
    drawSetPixel4x4_32Bit(framebuffer, nextlineoffset1, nextlineoffset2, nextlineoffset3, drawMake64BitColorFrom32Bit(hampixel));
    framebuffer += 2;
  }

  line_exact_sprites->MergeHAM4x4x32(reinterpret_cast<uint64_t *>(draw_buffer_current_ptr_local), linedescription, nextlineoffset1, nextlineoffset2, nextlineoffset3);
  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlh4x4_32bit_tmp, &dlh4x4_32bit, &dlh4x4_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

static void drawLineSegmentBG2x1_32Bit(uint32_t pixelcount, uint32_t bgcolor, uint32_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + pixelcount;
  uint64_t bgcolor64 = drawMake64BitColorFrom32Bit(bgcolor);

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel2x1_32Bit(framebuffer++, bgcolor64);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

static void drawLineSegmentBG2x2_32Bit(uint32_t pixelcount, uint32_t bgcolor, uint32_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + pixelcount;
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint64_t bgcolor64 = drawMake64BitColorFrom32Bit(bgcolor);

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel2x2_32Bit(framebuffer++, nextlineoffset1, bgcolor64);
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

static void drawLineSegmentBG4x2_32Bit(uint32_t pixelcount, uint32_t bgcolor, uint32_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + pixelcount * 2;
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint64_t bgcolor64 = drawMake64BitColorFrom32Bit(bgcolor);

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel4x2_32Bit(framebuffer, nextlineoffset1, bgcolor64);
    framebuffer += 2;
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one line segment using background color                                 */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

static void drawLineSegmentBG4x4_32Bit(uint32_t pixelcount, uint32_t bgcolor, uint32_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)draw_buffer_info.current_ptr;
  uint64_t *framebuffer_end = framebuffer + pixelcount * 2;
  uint32_t nextlineoffset1 = nextlineoffset / 8;
  uint32_t nextlineoffset2 = nextlineoffset1 * 2;
  uint32_t nextlineoffset3 = nextlineoffset1 * 3;
  uint64_t bgcolor64 = drawMake64BitColorFrom32Bit(bgcolor);

  while (framebuffer != framebuffer_end)
  {
    drawSetPixel4x4_32Bit(framebuffer, nextlineoffset1, nextlineoffset2, nextlineoffset3, bgcolor64);
    framebuffer += 2;
  }

  draw_buffer_info.current_ptr = (uint8_t *)framebuffer;
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL2x1_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
  drawLineSegmentBG2x1_32Bit(linedescription->BG_pad_front, linedescription->colors[0], nextlineoffset);
  ((draw_line_func)(linedescription->draw_line_BPL_res_routine))(linedescription, nextlineoffset);
  drawLineSegmentBG2x1_32Bit(linedescription->BG_pad_back, linedescription->colors[0], nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL2x2_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
  drawLineSegmentBG2x2_32Bit(linedescription->BG_pad_front, linedescription->colors[0], nextlineoffset);
  ((draw_line_func)(linedescription->draw_line_BPL_res_routine))(linedescription, nextlineoffset);
  drawLineSegmentBG2x2_32Bit(linedescription->BG_pad_back, linedescription->colors[0], nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL4x2_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
  drawLineSegmentBG4x2_32Bit(linedescription->BG_pad_front, linedescription->colors[0], nextlineoffset);
  ((draw_line_func)(linedescription->draw_line_BPL_res_routine))(linedescription, nextlineoffset);
  drawLineSegmentBG4x2_32Bit(linedescription->BG_pad_back, linedescription->colors[0], nextlineoffset);
}

/*==============================================================================*/
/* Draw one bitplane line                                                       */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBPL4x4_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
  drawLineSegmentBG4x4_32Bit(linedescription->BG_pad_front, linedescription->colors[0], nextlineoffset);
  ((draw_line_func)(linedescription->draw_line_BPL_res_routine))(linedescription, nextlineoffset);
  drawLineSegmentBG4x4_32Bit(linedescription->BG_pad_back, linedescription->colors[0], nextlineoffset);
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  1x (scanlines or in interlace)                              */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG2x1_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dlsbg2x1_32bit_pixels += drawGetClipWidth();
  drawTscBefore(&dlsbg2x1_32bit_tmp);
#endif

  drawLineSegmentBG2x1_32Bit(drawGetInternalClip().GetWidth(), linedescription->colors[0], nextlineoffset);

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlsbg2x1_32bit_tmp, &dlsbg2x1_32bit, &dlsbg2x1_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    2x                                                          */
/* Vertical Scale:  2x (solid and not in interlace)                             */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG2x2_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dlsbg2x2_32bit_pixels += drawGetClipWidth();
  drawTscBefore(&dlsbg2x2_32bit_tmp);
#endif

  drawLineSegmentBG2x2_32Bit(drawGetInternalClip().GetWidth(), linedescription->colors[0], nextlineoffset);

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlsbg2x2_32bit_tmp, &dlsbg2x2_32bit, &dlsbg2x2_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  2x (scanlines or in interlace)                              */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG4x2_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dlsbg4x2_32bit_pixels += drawGetClipWidth();
  drawTscBefore(&dlsbg4x2_32bit_tmp);
#endif

  drawLineSegmentBG4x2_32Bit(drawGetInternalClip().GetWidth(), linedescription->colors[0], nextlineoffset);

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlsbg4x2_32bit_tmp, &dlsbg4x2_32bit, &dlsbg4x2_32bit_times);
#endif
}

/*==============================================================================*/
/* Draw one background line                                                     */
/*                                                                              */
/* Display size:    4x                                                          */
/* Vertical Scale:  4x (solid and not in interlace)                             */
/* Pixel format:    32 bit RGB                                                  */
/*==============================================================================*/

void drawLineBG4x4_32Bit(graph_line *linedescription, uint32_t nextlineoffset)
{
#ifdef DRAW_TSC_PROFILE
  dlsbg4x4_32bit_pixels += drawGetClipWidth();
  drawTscBefore(&dlsbg4x4_32bit_tmp);
#endif

  drawLineSegmentBG4x4_32Bit(drawGetInternalClip().GetWidth(), linedescription->colors[0], nextlineoffset);

#ifdef DRAW_TSC_PROFILE
  drawTscAfter(&dlsbg4x4_32bit_tmp, &dlsbg4x4_32bit, &dlsbg4x4_32bit_times);
#endif
}

/*============================================================================*/
/* Lookup tables that holds all the drawing routines for various Amiga and    */
/* host screen modes [color depth (3)][sizes (4)]                             */
/*============================================================================*/

draw_line_func draw_line_BPL_manage_funcs[3][4] = {
    {drawLineBPL2x1_16Bit, drawLineBPL2x2_16Bit, drawLineBPL4x2_16Bit, drawLineBPL4x4_16Bit},
    {drawLineBPL2x1_24Bit, drawLineBPL2x2_24Bit, drawLineBPL4x2_24Bit, drawLineBPL4x4_24Bit},
    {drawLineBPL2x1_32Bit, drawLineBPL2x2_32Bit, drawLineBPL4x2_32Bit, drawLineBPL4x4_32Bit}};

draw_line_func draw_line_BG_funcs[3][4] = {
    {drawLineBG2x1_16Bit, drawLineBG2x2_16Bit, drawLineBG4x2_16Bit, drawLineBG4x4_16Bit},
    {drawLineBG2x1_24Bit, drawLineBG2x2_24Bit, drawLineBG4x2_24Bit, drawLineBG4x4_24Bit},
    {drawLineBG2x1_32Bit, drawLineBG2x2_32Bit, drawLineBG4x2_32Bit, drawLineBG4x4_32Bit}};

draw_line_func draw_line_lores_funcs[3][4] = {
    {drawLineNormal2x1_16Bit, drawLineNormal2x2_16Bit, drawLineNormal4x2_16Bit, drawLineNormal4x4_16Bit},
    {drawLineNormal2x1_24Bit, drawLineNormal2x2_24Bit, drawLineNormal4x2_24Bit, drawLineNormal4x4_24Bit},
    {drawLineNormal2x1_32Bit, drawLineNormal2x2_32Bit, drawLineNormal4x2_32Bit, drawLineNormal4x4_32Bit}};

draw_line_func draw_line_hires_funcs[3][4] = {
    {drawLineNormal1x1_16Bit, drawLineNormal1x2_16Bit, drawLineNormal2x2_16Bit, drawLineNormal2x4_16Bit},
    {drawLineNormal1x1_24Bit, drawLineNormal1x2_24Bit, drawLineNormal2x2_24Bit, drawLineNormal2x4_24Bit},
    {drawLineNormal1x1_32Bit, drawLineNormal1x2_32Bit, drawLineNormal2x2_32Bit, drawLineNormal2x4_32Bit}};

draw_line_func draw_line_dual_lores_funcs[3][4] = {
    {drawLineDual2x1_16Bit, drawLineDual2x2_16Bit, drawLineDual4x2_16Bit, drawLineDual4x4_16Bit},
    {drawLineDual2x1_24Bit, drawLineDual2x2_24Bit, drawLineDual4x2_24Bit, drawLineDual4x4_24Bit},
    {drawLineDual2x1_32Bit, drawLineDual2x2_32Bit, drawLineDual4x2_32Bit, drawLineDual4x4_32Bit}};

draw_line_func draw_line_dual_hires_funcs[3][4] = {
    {drawLineDual1x1_16Bit, drawLineDual1x2_16Bit, drawLineDual2x2_16Bit, drawLineDual2x4_16Bit},
    {drawLineDual1x1_24Bit, drawLineDual1x2_24Bit, drawLineDual2x2_24Bit, drawLineDual2x4_24Bit},
    {drawLineDual1x1_32Bit, drawLineDual1x2_32Bit, drawLineDual2x2_32Bit, drawLineDual2x4_32Bit}};

draw_line_func draw_line_HAM_lores_funcs[3][4] = {
    {drawLineHAM2x1_16Bit, drawLineHAM2x2_16Bit, drawLineHAM4x2_16Bit, drawLineHAM4x4_16Bit},
    {drawLineHAM2x1_24Bit, drawLineHAM2x2_24Bit, drawLineHAM4x2_24Bit, drawLineHAM4x4_24Bit},
    {drawLineHAM2x1_32Bit, drawLineHAM2x2_32Bit, drawLineHAM4x2_32Bit, drawLineHAM4x4_32Bit}};

static uint32_t drawGetColorDepthIndex()
{
  if (draw_buffer_info.bits == 15 || draw_buffer_info.bits == 16)
  {
    return 0;
  }
  else if (draw_buffer_info.bits == 24)
  {
    return 1;
  }
  // draw_buffer_info.bits == 32
  return 2;
}

uint32_t drawGetScaleIndex()
{
  uint32_t internal_scale_factor = drawGetInternalScaleFactor();
  if (drawGetUseInterlacedRendering())
  {
    if (internal_scale_factor == 2)
    {
      return 0; // 2x1
    }
    else
    {
      return 2; // 4x2
    }
  }

  // <Not interlaced>
  if (internal_scale_factor == 2 && drawGetDisplayScaleStrategy() == DISPLAYSCALE_STRATEGY_SCANLINES)
  {
    return 0; // 2x1
  }
  else if (internal_scale_factor == 2 && drawGetDisplayScaleStrategy() == DISPLAYSCALE_STRATEGY_SOLID)
  {
    return 1; // 2x2
  }
  else if (internal_scale_factor == 4 && drawGetDisplayScaleStrategy() == DISPLAYSCALE_STRATEGY_SCANLINES)
  {
    return 2; // 4x2
  }
  else // if (scale_factor == 4 && drawGetDisplayScaleStrategy() == DISPLAYSCALE_STRATEGY_SOLID)
  {
    return 3; // 4x4
  }
}

void drawModeFunctionsInitialize()
{
  uint32_t colordepth_index = drawGetColorDepthIndex();
  uint32_t scale_index = drawGetScaleIndex();

  draw_line_BPL_manage_routine = draw_line_BPL_manage_funcs[colordepth_index][scale_index];
  draw_line_routine = draw_line_BG_routine = draw_line_BG_funcs[colordepth_index][scale_index];
  draw_line_BPL_res_routine = draw_line_lores_routine = draw_line_lores_funcs[colordepth_index][scale_index];
  draw_line_hires_routine = draw_line_hires_funcs[colordepth_index][scale_index];
  draw_line_dual_lores_routine = draw_line_dual_lores_funcs[colordepth_index][scale_index];
  draw_line_dual_hires_routine = draw_line_dual_hires_funcs[colordepth_index][scale_index];
  draw_line_HAM_lores_routine = draw_line_HAM_lores_funcs[colordepth_index][scale_index];
}
