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
#include "fellow/chipset/Graphics.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/chipset/draw_interlace_control.h"
#include "fellow/chipset/LineExactSprites.h"
#include "fellow/chipset/DualPlayfieldMapper.h"
#include "fellow/chipset/draw_pixelrenderers.h"

const uint8_t *PixelRenderers::GetDualTranslatePtr(const graph_line *linedescription)
{
  return DualPlayfieldMapper::GetMappingPtr((linedescription->bplcon2 & 0x0040) == 0);
}

uint32_t PixelRenderers::Make32BitColorFrom16Bit(uint16_t color)
{
  return ((uint32_t)color) | ((uint32_t)color) << 16;
}

uint64_t PixelRenderers::Make64BitColorFrom16Bit(uint16_t color)
{
  return ((uint64_t)color) | ((uint64_t)color) << 16 | ((uint64_t)color) << 32 | ((uint64_t)color) << 48;
}

uint64_t PixelRenderers::Make64BitColorFrom32Bit(uint32_t color)
{
  return ((uint64_t)color) | ((uint64_t)color) << 32;
}

uint8_t PixelRenderers::GetDualColorIndex(const uint8_t *dualTranslatePtr, uint8_t playfield1Value, uint8_t playfield2Value)
{
  return *(dualTranslatePtr + ((playfield1Value << 8) + playfield2Value));
}

uint16_t PixelRenderers::GetDual16BitColor(const uint64_t *colors, const uint8_t *dualTranslatePtr, uint8_t playfield1Value, uint8_t playfield2Value)
{
  uint8_t colorIndex = GetDualColorIndex(dualTranslatePtr, playfield1Value, playfield2Value);
  return (uint16_t)colors[colorIndex];
}

uint32_t PixelRenderers::GetDual32BitColor(const uint64_t *colors, const uint8_t *dualTranslatePtr, uint8_t playfield1Value, uint8_t playfield2Value)
{
  uint8_t colorIndex = GetDualColorIndex(dualTranslatePtr, playfield1Value, playfield2Value);
  return (uint32_t)colors[colorIndex];
}

uint64_t PixelRenderers::GetDual64BitColor(const uint64_t *colors, const uint8_t *dualTranslatePtr, uint8_t playfield1Value, uint8_t playfield2Value)
{
  uint8_t colorIndex = GetDualColorIndex(dualTranslatePtr, playfield1Value, playfield2Value);
  return colors[colorIndex];
}

uint32_t PixelRenderers::MakeHoldMask(unsigned int pos, unsigned int size, bool longdestination)
{
  uint32_t holdmask = 0;
  for (unsigned int i = pos; i < (pos + size); i++)
  {
    holdmask |= (1 << i);
  }

  return longdestination ? (~holdmask) : ((~holdmask) & 0xffff) | ((~holdmask) << 16);
}

void PixelRenderers::HAMTableInit(const GfxDrvColorBitsInformation &colorBitsInformation)
{
  bool has24Or32BitPixels = colorBitsInformation.ColorBits > 16;

  draw_HAM_modify_table[0][0] = 0;
  draw_HAM_modify_table[0][1] = 0;
  draw_HAM_modify_table[1][0] = (uint32_t)(colorBitsInformation.BluePosition + colorBitsInformation.BlueSize - 4);
  draw_HAM_modify_table[1][1] = MakeHoldMask(colorBitsInformation.BluePosition, colorBitsInformation.BlueSize, has24Or32BitPixels);
  draw_HAM_modify_table[2][0] = (uint32_t)(colorBitsInformation.RedPosition + colorBitsInformation.RedSize - 4);
  draw_HAM_modify_table[2][1] = MakeHoldMask(colorBitsInformation.RedPosition, colorBitsInformation.RedSize, has24Or32BitPixels);
  draw_HAM_modify_table[3][0] = (uint32_t)(colorBitsInformation.GreenPosition + colorBitsInformation.GreenSize - 4);
  draw_HAM_modify_table[3][1] = MakeHoldMask(colorBitsInformation.GreenPosition, colorBitsInformation.GreenSize, has24Or32BitPixels);
}

uint32_t PixelRenderers::UpdateHAMPixel(uint32_t hampixel, uint8_t pixelValue)
{
  const uint8_t *holdmask = (uint8_t *)draw_HAM_modify_table + ((pixelValue & 0x30) >> 1);
  const uint32_t bitindex = *((const uint32_t *)(holdmask + draw_HAM_modify_table_bitindex));
  hampixel &= *((const uint32_t *)(holdmask + draw_HAM_modify_table_holdmask));
  return hampixel | ((pixelValue & 0xf) << (bitindex & 0xff));
}

uint32_t PixelRenderers::MakeHAMPixel(const uint64_t *colors, uint32_t hampixel, uint8_t pixelValue)
{
  return ((pixelValue & 0x30) == 0) ? (uint32_t)colors[pixelValue] : UpdateHAMPixel(hampixel, pixelValue);
}

uint32_t PixelRenderers::ProcessNonVisibleHAMPixels(const graph_line *linedescription, int32_t pixelCount)
{
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DDF_start;
  uint32_t hampixel = 0;
  while (pixelCount-- > 0)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
  }
  return hampixel;
}

uint32_t PixelRenderers::GetFirstHamPixelFromInitialInvisibleHAMPixels(const graph_line *linedescription)
{
  const int32_t nonVisiblePixelCount = (int32_t)(linedescription->DIW_first_draw - linedescription->DDF_start);
  if (nonVisiblePixelCount > 0)
  {
    return ProcessNonVisibleHAMPixels(linedescription, nonVisiblePixelCount);
  }

  return 0;
}

void PixelRenderers::SetPixel1x1_16Bit(uint16_t *framebuffer, uint16_t pixelColor)
{
  framebuffer[0] = pixelColor;
}

void PixelRenderers::SetPixel1x2_16Bit(uint16_t *framebuffer, ptrdiff_t nextlineoffset, uint16_t pixelColor)
{
  framebuffer[0] = pixelColor;
  framebuffer[nextlineoffset] = pixelColor;
}

void PixelRenderers::SetPixel2x1_16Bit(uint32_t *framebuffer, uint32_t pixelColor)
{
  framebuffer[0] = pixelColor;
}

void PixelRenderers::SetPixel2x2_16Bit(uint32_t *framebuffer, ptrdiff_t nextlineoffset, uint32_t pixelColor)
{
  framebuffer[0] = pixelColor;
  framebuffer[nextlineoffset] = pixelColor;
}

void PixelRenderers::SetPixel2x4_16Bit(uint32_t *framebuffer, ptrdiff_t nextlineoffset1, ptrdiff_t nextlineoffset2, ptrdiff_t nextlineoffset3, uint32_t pixelColor)
{
  framebuffer[0] = pixelColor;
  framebuffer[nextlineoffset1] = pixelColor;
  framebuffer[nextlineoffset2] = pixelColor;
  framebuffer[nextlineoffset3] = pixelColor;
}

void PixelRenderers::SetPixel4x2_16Bit(uint64_t *framebuffer, ptrdiff_t nextlineoffset, uint64_t pixelColor)
{
  framebuffer[0] = pixelColor;
  framebuffer[nextlineoffset] = pixelColor;
}

void PixelRenderers::SetPixel4x4_16Bit(uint64_t *framebuffer, ptrdiff_t nextlineoffset1, ptrdiff_t nextlineoffset2, ptrdiff_t nextlineoffset3, uint64_t pixelColor)
{
  framebuffer[0] = pixelColor;
  framebuffer[nextlineoffset1] = pixelColor;
  framebuffer[nextlineoffset2] = pixelColor;
  framebuffer[nextlineoffset3] = pixelColor;
}

//==========================================================
// general function for drawing one line using normal pixels
// single pixels
// single lines
//
// pixel format: 15/16 bit RGB
//==========================================================

uint8_t *PixelRenderers::DrawLineNormal1x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint16_t *framebuffer = (uint16_t *)framebufferLinePtr;
  const uint16_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    uint16_t pixelColor = (uint16_t)linedescription->colors[*sourcePtr++];
    SetPixel1x1_16Bit(framebuffer++, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//==========================================================
// general function for drawing one line using normal pixels
// single pixels
// double lines
//
// pixel format: 15/16 bit RGB
//==========================================================

uint8_t *PixelRenderers::DrawLineNormal1x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint16_t *framebuffer = (uint16_t *)framebufferLinePtr;
  const uint16_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 2;

  while (framebuffer != framebufferEnd)
  {
    uint16_t pixelColor = (uint16_t)linedescription->colors[*sourcePtr++];
    SetPixel1x2_16Bit(framebuffer++, nextlineoffset1, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//==========================================================
// general function for drawing one line using normal pixels
// double pixels
// single lines
//
// pixel format: 15/16 bit RGB
//==========================================================

uint8_t *PixelRenderers::DrawLineNormal2x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = (uint32_t)linedescription->colors[*sourcePtr++];
    SetPixel2x1_16Bit(framebuffer++, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//==========================================================
// general function for drawing one line using normal pixels
// double pixels
// double lines
//
// pixel format: 15/16 bit RGB
//==========================================================

uint8_t *PixelRenderers::DrawLineNormal2x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = (uint32_t)linedescription->colors[*sourcePtr++];
    SetPixel2x2_16Bit(framebuffer++, nextlineoffset1, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//==========================================================
// general function for drawing one line using normal pixels
// double pixels
// quad lines
//
// pixel format: 15/16 bit RGB
//==========================================================

uint8_t *PixelRenderers::DrawLineNormal2x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 4;
  ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = (uint32_t)linedescription->colors[*sourcePtr++];
    SetPixel2x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//==========================================================
// general function for drawing one line using normal pixels
// quad pixels
// double lines
//
// pixel format: 15/16 bit RGB
//==========================================================

uint8_t *PixelRenderers::DrawLineNormal4x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = linedescription->colors[*sourcePtr++];
    SetPixel4x2_16Bit(framebuffer++, nextlineoffset1, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//==========================================================
// general function for drawing one line using normal pixels
// quad pixels
// quad lines
//
// pixel format: 15/16 bit RGB
//==========================================================

uint8_t *PixelRenderers::DrawLineNormal4x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = linedescription->colors[*sourcePtr++];
    SetPixel4x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// single pixels
// single lines
//
// Pixel format: 15/16 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual1x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint16_t *framebuffer = (uint16_t *)framebufferLinePtr;
  const uint16_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);

  while (framebuffer != framebufferEnd)
  {
    const uint16_t pixelColor = GetDual16BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel1x1_16Bit(framebuffer++, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// single pixels
// double lines
//
// Pixel format: 15/16 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual1x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint16_t *framebuffer = (uint16_t *)framebufferLinePtr;
  const uint16_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  ptrdiff_t nextlineoffset1 = nextlineoffset / 2;

  while (framebuffer != framebufferEnd)
  {
    const uint16_t pixelColor = GetDual16BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel1x2_16Bit(framebuffer++, nextlineoffset1, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// double pixels
// single lines
//
// Pixel format: 15/16 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual2x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = GetDual32BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel2x1_16Bit(framebuffer++, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// double pixels
// double lines
//
// Pixel format: 15/16 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual2x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  ptrdiff_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = GetDual32BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel2x2_16Bit(framebuffer++, nextlineoffset1, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// double pixels
// quad lines
//
// Pixel format: 15/16 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual2x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  ptrdiff_t nextlineoffset1 = nextlineoffset / 4;
  ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = GetDual32BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel2x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// double pixels
// quad lines
//
// Pixel format: 15/16 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual4x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = GetDual64BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel4x2_16Bit(framebuffer++, nextlineoffset1, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// quad pixels
// quad lines
//
// Pixel format: 15/16 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual4x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = GetDual64BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel4x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//============================
// Draw one line of HAM data
// double pixels
// single lines
//
// Pixel format: 15/16 bit RGB
//============================

uint8_t *PixelRenderers::DrawLineHAM2x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
    SetPixel2x1_16Bit(framebuffer++, Make32BitColorFrom16Bit(hampixel));
  }

  line_exact_sprites->MergeHAM2x1x16((uint32_t *)framebufferLinePtr, linedescription);

  return (uint8_t *)framebuffer;
}

//============================
// Draw one line of HAM data
// double pixels
// double lines
//
// Pixel format: 15/16 bit RGB
//============================

uint8_t *PixelRenderers::DrawLineHAM2x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebufferEnd)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
    SetPixel2x2_16Bit(framebuffer++, nextlineoffset1, Make32BitColorFrom16Bit(hampixel));
  }

  line_exact_sprites->MergeHAM2x2x16((uint32_t *)framebufferLinePtr, linedescription, nextlineoffset1);

  return (uint8_t *)framebuffer;
}

//============================
// Draw one line of HAM data
// quad pixels
// double lines
//
// Pixel format: 15/16 bit RGB
//============================

uint8_t *PixelRenderers::DrawLineHAM4x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebufferEnd)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
    SetPixel4x2_16Bit(framebuffer++, nextlineoffset1, Make64BitColorFrom16Bit(hampixel));
  }

  line_exact_sprites->MergeHAM4x2x16((uint64_t *)framebufferLinePtr, linedescription, nextlineoffset1);

  return (uint8_t *)framebuffer;
}

//============================
// Draw one line of HAM data
// quad pixels
// quad lines
//
// Pixel format: 15/16 bit RGB
//============================

uint8_t *PixelRenderers::DrawLineHAM4x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebufferEnd)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
    SetPixel4x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, Make64BitColorFrom16Bit(hampixel));
  }

  line_exact_sprites->MergeHAM4x4x16((uint64_t *)framebufferLinePtr, linedescription, nextlineoffset1, nextlineoffset2, nextlineoffset3);

  return (uint8_t *)framebuffer;
}

//=============================================
// Draw one line segment using background color
// double pixels
// single lines
//
// Pixel format: 15/16 bit RGB
//=============================================

uint8_t *PixelRenderers::DrawLineSegmentBG2x1_16Bit(uint32_t pixelcount, uint32_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + pixelcount;

  while (framebuffer != framebufferEnd)
  {
    SetPixel2x1_16Bit(framebuffer++, bgcolor);
  }

  return (uint8_t *)framebuffer;
}

//=============================================
// Draw one line segment using background color
// double pixels
// double lines
//
// Pixel format: 15/16 bit RGB
//=============================================

uint8_t *PixelRenderers::DrawLineSegmentBG2x2_16Bit(uint32_t pixelcount, uint32_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + pixelcount;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebufferEnd)
  {
    SetPixel2x2_16Bit(framebuffer++, nextlineoffset1, bgcolor);
  }

  return (uint8_t *)framebuffer;
}

//=============================================
// Draw one line segment using background color
// quad pixels
// double lines
//
// Pixel format: 15/16 bit RGB
//=============================================

uint8_t *PixelRenderers::DrawLineSegmentBG4x2_16Bit(uint32_t pixelcount, uint64_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + pixelcount;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebufferEnd)
  {
    SetPixel4x2_16Bit(framebuffer++, nextlineoffset1, bgcolor);
  }

  return (uint8_t *)framebuffer;
}

//=============================================
// Draw one line segment using background color
// quad pixels
// quad lines
//
// Pixel format: 15/16 bit RGB
//=============================================

uint8_t *PixelRenderers::DrawLineSegmentBG4x4_16Bit(uint32_t pixelcount, uint64_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + pixelcount;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebufferEnd)
  {
    SetPixel4x4_16Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, bgcolor);
  }

  return (uint8_t *)framebuffer;
}

//============================
// Draw one bitplane line
// double pixels
// single lines
//
// Pixel format: 15/16 bit RGB
//============================

void PixelRenderers::DrawLineBPL2x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = DrawLineSegmentBG2x1_16Bit(linedescription->BG_pad_front, (uint32_t)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  // TODO: This must change probably unless we use std::function with lambdas linked to this instance.... ?
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  DrawLineSegmentBG2x1_16Bit(linedescription->BG_pad_back, (uint32_t)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//============================
// Draw one bitplane line
// double pixels
// double lines
//
// Pixel format: 15/16 bit RGB
//============================

void PixelRenderers::DrawLineBPL2x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = DrawLineSegmentBG2x2_16Bit(linedescription->BG_pad_front, (uint32_t)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  DrawLineSegmentBG2x2_16Bit(linedescription->BG_pad_back, (uint32_t)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//============================
// Draw one bitplane line
// quad pixels
// double lines
//
// Pixel format: 15/16 bit RGB
//============================

void PixelRenderers::DrawLineBPL4x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = DrawLineSegmentBG4x2_16Bit(linedescription->BG_pad_front, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  DrawLineSegmentBG4x2_16Bit(linedescription->BG_pad_back, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//============================
// Draw one bitplane line
// quad pixels
// quad lines
//
// Pixel format: 15/16 bit RGB
//============================

void PixelRenderers::DrawLineBPL4x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = DrawLineSegmentBG4x4_16Bit(linedescription->BG_pad_front, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  DrawLineSegmentBG4x4_16Bit(linedescription->BG_pad_back, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//============================
// Draw one background line
// double pixels
// single lines
//
// Pixel format: 15/16 bit RGB
//============================

void PixelRenderers::DrawLineBG2x1_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  DrawLineSegmentBG2x1_16Bit(linedescription->MaxClipWidth / 4, (uint32_t)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//============================
// Draw one background line
// double pixels
// double lines
//
// Pixel format: 15/16 bit RGB
//============================

void PixelRenderers::DrawLineBG2x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  DrawLineSegmentBG2x2_16Bit(linedescription->MaxClipWidth / 4, (uint32_t)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//============================
// Draw one background line
// quad pixels
// double lines
//
// Pixel format: 15/16 bit RGB
//============================

void PixelRenderers::DrawLineBG4x2_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  DrawLineSegmentBG4x2_16Bit(linedescription->MaxClipWidth / 4, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//============================
// Draw one background line
// quad pixels
// quad lines
//
// Pixel format: 15/16 bit RGB
//============================

void PixelRenderers::DrawLineBG4x4_16Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  DrawLineSegmentBG4x4_16Bit(linedescription->MaxClipWidth / 4, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

// 24 Bit

void PixelRenderers::SetPixel1x1_24Bit(uint8_t *framebuffer, uint32_t pixelColor)
{
  *((uint32_t *)framebuffer) = pixelColor;
}

void PixelRenderers::SetPixel1x2_24Bit(uint8_t *framebuffer, ptrdiff_t nextlineoffset, uint32_t pixelColor)
{
  *((uint32_t *)framebuffer) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset)) = pixelColor;
}

void PixelRenderers::SetPixel2x1_24Bit(uint8_t *framebuffer, uint32_t pixelColor)
{
  *((uint32_t *)framebuffer) = pixelColor;
  *((uint32_t *)(framebuffer + 3)) = pixelColor;
}

void PixelRenderers::SetPixel2x2_24Bit(uint8_t *framebuffer, ptrdiff_t nextlineoffset, uint32_t pixelColor)
{
  *((uint32_t *)framebuffer) = pixelColor;
  *((uint32_t *)(framebuffer + 3)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset + 3)) = pixelColor;
}

void PixelRenderers::SetPixel2x4_24Bit(uint8_t *framebuffer, ptrdiff_t nextlineoffset1, ptrdiff_t nextlineoffset2, ptrdiff_t nextlineoffset3, uint32_t pixelColor)
{
  *((uint32_t *)framebuffer) = pixelColor;
  *((uint32_t *)(framebuffer + 3)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset1)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset1 + 3)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset2)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset2 + 3)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset3)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset3 + 3)) = pixelColor;
}

void PixelRenderers::SetPixel4x2_24Bit(uint8_t *framebuffer, ptrdiff_t nextlineoffset, uint32_t pixelColor)
{
  *((uint32_t *)framebuffer) = pixelColor;
  *((uint32_t *)(framebuffer + 3)) = pixelColor;
  *((uint32_t *)(framebuffer + 6)) = pixelColor;
  *((uint32_t *)(framebuffer + 9)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset + 3)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset + 6)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset + 9)) = pixelColor;
}

void PixelRenderers::SetPixel4x4_24Bit(uint8_t *framebuffer, ptrdiff_t nextlineoffset1, ptrdiff_t nextlineoffset2, ptrdiff_t nextlineoffset3, uint32_t pixelColor)
{
  *((uint32_t *)framebuffer) = pixelColor;
  *((uint32_t *)(framebuffer + 3)) = pixelColor;
  *((uint32_t *)(framebuffer + 6)) = pixelColor;
  *((uint32_t *)(framebuffer + 9)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset1)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset1 + 3)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset1 + 6)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset1 + 9)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset2)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset2 + 3)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset2 + 6)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset2 + 9)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset3)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset3 + 3)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset3 + 6)) = pixelColor;
  *((uint32_t *)(framebuffer + nextlineoffset3 + 9)) = pixelColor;
}

//==================================
// Draw one line using normal pixels
// single pixels
// single lines
//
// Pixel format: 24 bit RGB
//==================================

uint8_t *PixelRenderers::DrawLineNormal1x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 3;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = (uint32_t)linedescription->colors[*sourcePtr++];
    SetPixel1x1_24Bit(framebuffer, pixelColor);
    framebuffer += 3;
  }

  return framebuffer;
}

//==================================
// Draw one line using normal pixels
// single pixels
// double lines
//
// Pixel format: 24 bit RGB
//==================================

uint8_t *PixelRenderers::DrawLineNormal1x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 3;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = (uint32_t)linedescription->colors[*sourcePtr++];
    SetPixel1x2_24Bit(framebuffer, nextlineoffset, pixelColor);
    framebuffer += 3;
  }

  return framebuffer;
}

//==================================
// Draw one line using normal pixels
// double pixels
// single lines
//
// Pixel format: 24 bit RGB
//==================================

uint8_t *PixelRenderers::DrawLineNormal2x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = (uint32_t)linedescription->colors[*sourcePtr++];
    SetPixel2x1_24Bit(framebuffer, pixelColor);
    framebuffer += 6;
  }

  return framebuffer;
}

//==================================
// Draw one line using normal pixels
// double pixels
// double lines
//
// Pixel format: 24 bit RGB
//==================================

uint8_t *PixelRenderers::DrawLineNormal2x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = (uint32_t)linedescription->colors[*sourcePtr++];
    SetPixel2x2_24Bit(framebuffer, nextlineoffset, pixelColor);
    framebuffer += 6;
  }

  return framebuffer;
}

//==================================
// Draw one line using normal pixels
// double pixels
// quad lines
//
// Pixel format: 24 bit RGB
//==================================

uint8_t *PixelRenderers::DrawLineNormal2x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset2 = nextlineoffset * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = (uint32_t)linedescription->colors[*sourcePtr++];
    SetPixel2x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, pixelColor);
    framebuffer += 6;
  }

  return framebuffer;
}

//==================================
// Draw one line using normal pixels
// quad pixels
// double lines
//
// Pixel format: 24 bit RGB
//==================================

uint8_t *PixelRenderers::DrawLineNormal4x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 12;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = (uint32_t)linedescription->colors[*sourcePtr++];
    SetPixel4x2_24Bit(framebuffer, nextlineoffset, pixelColor);
    framebuffer += 12;
  }

  return framebuffer;
}

//==================================
// Draw one line using normal pixels
// quad pixels
// quad lines
//
// Pixel format: 24 bit RGB
//==================================

uint8_t *PixelRenderers::DrawLineNormal4x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 12;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset2 = nextlineoffset * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = (uint32_t)linedescription->colors[*sourcePtr++];
    SetPixel4x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, pixelColor);
    framebuffer += 12;
  }

  return framebuffer;
}

//====================================
// Draw one line mixing two playfields
// single pixels
// single lines
//
// Pixel format: 24 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual1x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 3;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = GetDual32BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel1x1_24Bit(framebuffer, pixelColor);
    framebuffer += 3;
  }

  return framebuffer;
}

//====================================
// Draw one line mixing two playfields
// single pixels
// double lines
//
// Pixel format: 24 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual1x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 3;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = GetDual32BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel1x2_24Bit(framebuffer, nextlineoffset, pixelColor);
    framebuffer += 3;
  }

  return framebuffer;
}

//====================================
// Draw one line mixing two playfields
// double pixels
// single lines
//
// Pixel format: 24 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual2x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = GetDual32BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel2x1_24Bit(framebuffer, pixelColor);
    framebuffer += 6;
  }

  return framebuffer;
}

//====================================
// Draw one line mixing two playfields
// double pixels
// double lines
//
// Pixel format: 24 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual2x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = GetDual32BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel2x2_24Bit(framebuffer, nextlineoffset, pixelColor);
    framebuffer += 6;
  }

  return framebuffer;
}

//====================================
// Draw one line mixing two playfields
// double pixels
// quad lines
//
// Pixel format: 24 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual2x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  ptrdiff_t nextlineoffset2 = nextlineoffset * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = GetDual32BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel2x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, pixelColor);
    framebuffer += 6;
  }

  return framebuffer;
}

//====================================
// Draw one line mixing two playfields
// quad pixels
// double lines
//
// Pixel format: 24 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual4x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 12;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = GetDual32BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel4x2_24Bit(framebuffer, nextlineoffset, pixelColor);
    framebuffer += 12;
  }

  return framebuffer;
}

//====================================
// Draw one line mixing two playfields
// quad pixels
// quad lines
//
// Pixel format: 24 bit RGB
//====================================

uint8_t *PixelRenderers::DrawLineDual4x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 12;
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  ptrdiff_t nextlineoffset2 = nextlineoffset * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = GetDual32BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel4x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, pixelColor);
    framebuffer += 12;
  }

  return framebuffer;
}

//==========================
// Draw one line of HAM data
// double pixels
// single lines
//
// Pixel format: 24 bit RGB
//==========================

uint8_t *PixelRenderers::DrawLineHAM2x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
    SetPixel2x1_24Bit(framebuffer, hampixel);
    framebuffer += 6;
  }

  line_exact_sprites->MergeHAM2x1x24(framebufferLinePtr, linedescription);

  return framebuffer;
}

//==========================
// Draw one line of HAM data
// double pixels
// double lines
//
// Pixel format: 24 bit RGB
//==========================

uint8_t *PixelRenderers::DrawLineHAM2x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 6;
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
    SetPixel2x2_24Bit(framebuffer, nextlineoffset, hampixel);
    framebuffer += 6;
  }

  line_exact_sprites->MergeHAM2x2x24(framebufferLinePtr, linedescription, nextlineoffset);

  return framebuffer;
}

//==========================
// Draw one line of HAM data
// quad pixels
// double lines
//
// Pixel format: 24 bit RGB
//==========================

uint8_t *PixelRenderers::DrawLineHAM4x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 12;
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
    SetPixel4x2_24Bit(framebuffer, nextlineoffset, hampixel);
    framebuffer += 12;
  }

  line_exact_sprites->MergeHAM4x2x24(framebufferLinePtr, linedescription, nextlineoffset);

  return framebuffer;
}

//==========================
// Draw one line of HAM data
// quad pixels
// quad lines
//
// Pixel format: 24 bit RGB
//==========================

uint8_t *PixelRenderers::DrawLineHAM4x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 12;
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset2 = nextlineoffset * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebufferEnd)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
    SetPixel4x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, hampixel);
    framebuffer += 12;
  }

  line_exact_sprites->MergeHAM4x4x24(framebufferLinePtr, linedescription, nextlineoffset, nextlineoffset2, nextlineoffset3);

  return framebuffer;
}

//=============================================
// Draw one line segment using background color
// double pixels
// single lines
//
// Pixel format: 24 bit RGB
//=============================================

uint8_t *PixelRenderers::DrawLineSegmentBG2x1_24Bit(uint32_t pixelcount, uint32_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + pixelcount * 6;

  while (framebuffer != framebufferEnd)
  {
    SetPixel2x1_24Bit(framebuffer, bgcolor);
    framebuffer += 6;
  }

  return framebuffer;
}

//=============================================
// Draw one line segment using background color
// double pixels
// double lines
//
// Pixel format: 24 bit RGB
//=============================================

uint8_t *PixelRenderers::DrawLineSegmentBG2x2_24Bit(uint32_t pixelcount, uint32_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + pixelcount * 6;

  while (framebuffer != framebufferEnd)
  {
    SetPixel2x2_24Bit(framebuffer, nextlineoffset, bgcolor);
    framebuffer += 6;
  }

  return framebuffer;
}

//=============================================
// Draw one line segment using background color
// quad pixels
// double lines
//
// Pixel format: 24 bit RGB
//=============================================

uint8_t *PixelRenderers::DrawLineSegmentBG4x2_24Bit(uint32_t pixelcount, uint32_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + pixelcount * 12;

  while (framebuffer != framebufferEnd)
  {
    SetPixel4x2_24Bit(framebuffer, nextlineoffset, bgcolor);
    framebuffer += 12;
  }

  return framebuffer;
}

//=============================================
// Draw one line segment using background color
// quad pixels
// quad lines
//
// Pixel format: 24 bit RGB
//=============================================

uint8_t *PixelRenderers::DrawLineSegmentBG4x4_24Bit(uint32_t pixelcount, uint32_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint8_t *framebuffer = framebufferLinePtr;
  const uint8_t *framebufferEnd = framebuffer + pixelcount * 12;
  ptrdiff_t nextlineoffset2 = nextlineoffset * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset * 3;

  while (framebuffer != framebufferEnd)
  {
    SetPixel4x4_24Bit(framebuffer, nextlineoffset, nextlineoffset2, nextlineoffset3, bgcolor);
    framebuffer += 12;
  }

  return framebuffer;
}

//=========================
// Draw one bitplane line
// double pixels
// single lines
//
// Pixel format: 24 bit RGB
//=========================

void PixelRenderers::DrawLineBPL2x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = DrawLineSegmentBG2x1_24Bit(linedescription->BG_pad_front, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  DrawLineSegmentBG2x1_24Bit(linedescription->BG_pad_back, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one bitplane line
// double pixels
// double lines
//
// Pixel format: 24 bit RGB
//=========================

void PixelRenderers::DrawLineBPL2x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = DrawLineSegmentBG2x2_24Bit(linedescription->BG_pad_front, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  DrawLineSegmentBG2x2_24Bit(linedescription->BG_pad_back, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one bitplane line
// quad pixels
// double lines
//
// Pixel format: 24 bit RGB
//=========================

void PixelRenderers::DrawLineBPL4x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = DrawLineSegmentBG4x2_24Bit(linedescription->BG_pad_front, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  DrawLineSegmentBG4x2_24Bit(linedescription->BG_pad_back, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one bitplane line
// quad pixels
// quad lines
//
// Pixel format: 24 bit RGB
//=========================

void PixelRenderers::DrawLineBPL4x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = DrawLineSegmentBG4x4_24Bit(linedescription->BG_pad_front, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  DrawLineSegmentBG4x4_24Bit(linedescription->BG_pad_back, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one background line
// double pixels
// single lines
//
// Pixel format: 24 bit RGB
//=========================

void PixelRenderers::DrawLineBG2x1_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  DrawLineSegmentBG2x1_24Bit(linedescription->MaxClipWidth / 4, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one background line
// double pixels
// double lines
//
// Pixel format: 24 bit RGB
//=========================

void PixelRenderers::DrawLineBG2x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  DrawLineSegmentBG2x2_24Bit(linedescription->MaxClipWidth / 4, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one background line
// quad pixels
// double lines
//
// Pixel format: 24 bit RGB
//=========================

void PixelRenderers::DrawLineBG4x2_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  DrawLineSegmentBG4x2_24Bit(linedescription->MaxClipWidth / 4, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one background line
// quad pixels
// quad lines
//
// Pixel format: 24 bit RGB
//=========================

void PixelRenderers::DrawLineBG4x4_24Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  DrawLineSegmentBG4x4_24Bit(linedescription->MaxClipWidth / 4, (ULO)linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

// 32-Bit

void PixelRenderers::SetPixel1x1_32Bit(uint32_t *framebuffer, uint32_t pixelColor)
{
  framebuffer[0] = pixelColor;
}

void PixelRenderers::SetPixel1x2_32Bit(uint32_t *framebuffer, ptrdiff_t nextlineoffset1, uint32_t pixelColor)
{
  framebuffer[0] = pixelColor;
  framebuffer[nextlineoffset1] = pixelColor;
}

void PixelRenderers::SetPixel2x1_32Bit(uint64_t *framebuffer, uint64_t pixelColor)
{
  framebuffer[0] = pixelColor;
}

void PixelRenderers::SetPixel2x2_32Bit(uint64_t *framebuffer, ptrdiff_t nextlineoffset1, uint64_t pixelColor)
{
  framebuffer[0] = pixelColor;
  framebuffer[nextlineoffset1] = pixelColor;
}

void PixelRenderers::SetPixel2x4_32Bit(uint64_t *framebuffer, ptrdiff_t nextlineoffset1, ptrdiff_t nextlineoffset2, ptrdiff_t nextlineoffset3, uint64_t pixelColor)
{
  framebuffer[0] = pixelColor;
  framebuffer[nextlineoffset1] = pixelColor;
  framebuffer[nextlineoffset2] = pixelColor;
  framebuffer[nextlineoffset3] = pixelColor;
}

void PixelRenderers::SetPixel4x2_32Bit(uint64_t *framebuffer, ptrdiff_t nextlineoffset1, uint64_t pixelColor)
{
  framebuffer[0] = pixelColor;
  framebuffer[1] = pixelColor;
  framebuffer[nextlineoffset1] = pixelColor;
  framebuffer[nextlineoffset1 + 1] = pixelColor;
}

void PixelRenderers::SetPixel4x4_32Bit(uint64_t *framebuffer, ptrdiff_t nextlineoffset1, ptrdiff_t nextlineoffset2, ptrdiff_t nextlineoffset3, uint64_t pixelColor)
{
  framebuffer[0] = pixelColor;
  framebuffer[1] = pixelColor;
  framebuffer[nextlineoffset1] = pixelColor;
  framebuffer[nextlineoffset1 + 1] = pixelColor;
  framebuffer[nextlineoffset2] = pixelColor;
  framebuffer[nextlineoffset2 + 1] = pixelColor;
  framebuffer[nextlineoffset3] = pixelColor;
  framebuffer[nextlineoffset3 + 1] = pixelColor;
}

//==================================
// Draw one line using normal pixels
// single pixels
// single lines
//
// Pixel format: 32 bit RGB
//==================================

static uint8_t *DrawLineNormal1x1_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = (uint32_t)linedescription->colors[*sourcePtr++];
    SetPixel1x1_32Bit(framebuffer++, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//==================================
// Draw one line using normal pixels
// single pixels
// double lines
//
// Pixel format: 32 bit RGB
//==================================

static uint8_t *DrawLineNormal1x2_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = (uint32_t)linedescription->colors[*sourcePtr++];
    SetPixel1x2_32Bit(framebuffer++, nextlineoffset1, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//==================================
// Draw one line using normal pixels
// double pixels
// single lines
//
// Pixel format: 32 bit RGB
//==================================

static uint8_t *DrawLineNormal2x1_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = linedescription->colors[*sourcePtr++];
    SetPixel2x1_32Bit(framebuffer++, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//==================================
// Draw one line using normal pixels
// double pixels
// double lines
//
// Pixel format: 32 bit RGB
//==================================

static uint8_t *DrawLineNormal2x2_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = linedescription->colors[*sourcePtr++];
    SetPixel2x2_32Bit(framebuffer++, nextlineoffset1, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//==================================
// Draw one line using normal pixels
// double pixels
// quad lines
//
// Pixel format: 32 bit RGB
//==================================

static uint8_t *DrawLineNormal2x4_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = linedescription->colors[*sourcePtr++];
    SetPixel2x4_32Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//==================================
// Draw one line using normal pixels
// quad pixels
// double lines
//
// Pixel format: 32 bit RGB
//==================================

static uint8_t *DrawLineNormal4x2_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 2;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = linedescription->colors[*sourcePtr++];
    SetPixel4x2_32Bit(framebuffer, nextlineoffset1, pixelColor);
    framebuffer += 2;
  }

  return (uint8_t *)framebuffer;
}

//==================================
// Draw one line using normal pixels
// quad pixels
// quad lines
//
// Pixel format: 32 bit RGB
//==================================

static uint8_t *DrawLineNormal4x4_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 2;
  const uint8_t *sourcePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = linedescription->colors[*sourcePtr++];
    SetPixel4x4_32Bit(framebuffer, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixelColor);
    framebuffer += 2;
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// single pixels
// single lines
//
// Pixel format: 32 bit RGB
//====================================

static uint8_t *DrawLineDual1x1_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = GetDual32BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel1x1_32Bit(framebuffer++, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// single pixels
// double lines
//
// Pixel format: 32 bit RGB
//====================================

static uint8_t *DrawLineDual1x2_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t *framebuffer = (uint32_t *)framebufferLinePtr;
  const uint32_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 4;

  while (framebuffer != framebufferEnd)
  {
    uint32_t pixelColor = GetDual32BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel1x2_32Bit(framebuffer++, nextlineoffset1, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// double pixels
// single lines
//
// Pixel format: 32 bit RGB
//====================================

static uint8_t *DrawLineDual2x1_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = GetDual64BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel2x1_32Bit(framebuffer++, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// double pixels
// double lines
//
// Pixel format: 32 bit RGB
//====================================

static uint8_t *DrawLineDual2x2_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = GetDual64BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel2x2_32Bit(framebuffer++, nextlineoffset1, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// double pixels
// quad lines
//
// Pixel format: 32 bit RGB
//====================================

static uint8_t *DrawLineDual2x4_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = GetDual64BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel2x4_32Bit(framebuffer++, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixelColor);
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// quad pixels
// double lines
//
// Pixel format: 32 bit RGB
//====================================

static uint8_t *DrawLineDual4x2_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 2;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = GetDual64BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel4x2_32Bit(framebuffer, nextlineoffset1, pixelColor);
    framebuffer += 2;
  }

  return (uint8_t *)framebuffer;
}

//====================================
// Draw one line mixing two playfields
// quad pixels
// quad lines
//
// Pixel format: 32 bit RGB
//====================================

static uint8_t *DrawLineDual4x4_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 2;
  const uint8_t *dualTranslatePtr = GetDualTranslatePtr(linedescription);
  const uint8_t *sourceLine1Ptr = linedescription->line1 + linedescription->DIW_first_draw;
  const uint8_t *sourceLine2Ptr = linedescription->line2 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebufferEnd)
  {
    uint64_t pixelColor = GetDual64BitColor(linedescription->colors, dualTranslatePtr, *sourceLine1Ptr++, *sourceLine2Ptr++);
    SetPixel4x4_32Bit(framebuffer, nextlineoffset1, nextlineoffset2, nextlineoffset3, pixelColor);
    framebuffer += 2;
  }

  return (uint8_t *)framebuffer;
}

//==========================
// Draw one line of HAM data
// double pixels
// single lines
//
// Pixel format: 32 bit RGB
//==========================

static uint8_t *DrawLineHAM2x1_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DIW_first_draw;

  while (framebuffer != framebufferEnd)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
    SetPixel2x1_32Bit(framebuffer++, Make64BitColorFrom32Bit(hampixel));
  }

  line_exact_sprites->MergeHAM2x1x32((uint64_t *)framebufferLinePtr, linedescription);

  return (uint8_t *)framebuffer;
}

//==========================
// Draw one line of HAM data
// double pixels
// double lines
//
// Pixel format: 32 bit RGB
//==========================

static uint8_t *DrawLineHAM2x2_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count;
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebufferEnd)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
    SetPixel2x2_32Bit(framebuffer++, nextlineoffset1, Make64BitColorFrom32Bit(hampixel));
  }

  line_exact_sprites->MergeHAM2x2x32((uint64_t *)framebufferLinePtr, linedescription, nextlineoffset1);

  return (uint8_t *)framebuffer;
}

//==========================
// Draw one line of HAM data
// quad pixels
// double lines
//
// Pixel format: 32 bit RGB
//==========================

static uint8_t *DrawLineHAM4x2_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 2;
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebufferEnd)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
    SetPixel4x2_32Bit(framebuffer, nextlineoffset1, Make64BitColorFrom32Bit(hampixel));
    framebuffer += 2;
  }

  line_exact_sprites->MergeHAM4x2x32((uint64_t *)framebufferLinePtr, linedescription, nextlineoffset1);

  return (uint8_t *)framebuffer;
}

//==========================
// Draw one line of HAM data
// quad pixels
// quad lines
//
// Pixel format: 32 bit RGB
//==========================

static uint8_t *DrawLineHAM4x4_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint32_t hampixel = GetFirstHamPixelFromInitialInvisibleHAMPixels(linedescription);
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + linedescription->DIW_pixel_count * 2;
  const uint8_t *sourceLinePtr = linedescription->line1 + linedescription->DIW_first_draw;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebufferEnd)
  {
    hampixel = MakeHAMPixel(linedescription->colors, hampixel, *sourceLinePtr++);
    SetPixel4x4_32Bit(framebuffer, nextlineoffset1, nextlineoffset2, nextlineoffset3, DrawMake64BitColorFrom32Bit(hampixel));
    framebuffer += 2;
  }

  line_exact_sprites->MergeHAM4x4x32((uint64_t *)framebufferLinePtr, linedescription, nextlineoffset1, nextlineoffset2, nextlineoffset3);
  return (uint8_t *)framebuffer;
}

//=============================================
// Draw one line segment using background color
// double pixels
// single lines
//
// Pixel format: 32 bit RGB
//=============================================

static uint8_t *DrawLineSegmentBG2x1_32Bit(uint32_t pixelcount, uint64_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + pixelcount;

  while (framebuffer != framebufferEnd)
  {
    SetPixel2x1_32Bit(framebuffer++, bgcolor);
  }

  return (uint8_t *)framebuffer;
}

//=============================================
// Draw one line segment using background color
// double pixels
// double lines
//
// Pixel format: 32 bit RGB
//=============================================

static uint8_t *DrawLineSegmentBG2x2_32Bit(uint32_t pixelcount, uint64_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + pixelcount;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebufferEnd)
  {
    SetPixel2x2_32Bit(framebuffer++, nextlineoffset1, bgcolor);
  }

  return (uint8_t *)framebuffer;
}

//=============================================
// Draw one line segment using background color
// quad pixels
// double lines
//
// Pixel format: 32 bit RGB
//=============================================

static uint8_t *DrawLineSegmentBG4x2_32Bit(uint32_t pixelcount, uint64_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + pixelcount * 2;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;

  while (framebuffer != framebufferEnd)
  {
    SetPixel4x2_32Bit(framebuffer, nextlineoffset1, bgcolor);
    framebuffer += 2;
  }

  return (uint8_t *)framebuffer;
}

//=============================================
// Draw one line segment using background color
// quad pixels
// quad lines
//
// Pixel format: 32 bit RGB
//=============================================

static uint8_t *DrawLineSegmentBG4x4_32Bit(uint32_t pixelcount, uint64_t bgcolor, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  uint64_t *framebuffer = (uint64_t *)framebufferLinePtr;
  const uint64_t *framebufferEnd = framebuffer + pixelcount * 2;
  ptrdiff_t nextlineoffset1 = nextlineoffset / 8;
  ptrdiff_t nextlineoffset2 = nextlineoffset1 * 2;
  ptrdiff_t nextlineoffset3 = nextlineoffset1 * 3;

  while (framebuffer != framebufferEnd)
  {
    SetPixel4x4_32Bit(framebuffer, nextlineoffset1, nextlineoffset2, nextlineoffset3, bgcolor);
    framebuffer += 2;
  }

  return (uint8_t *)framebuffer;
}

//=========================
// Draw one bitplane line
// double pixels
// single lines
//
// Pixel format: 32 bit RGB
//=========================

void DrawLineBPL2x1_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = DrawLineSegmentBG2x1_32Bit(linedescription->BG_pad_front, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  DrawLineSegmentBG2x1_32Bit(linedescription->BG_pad_back, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one bitplane line
// double pixels
// double lines
//
// Pixel format: 32 bit RGB
//=========================

void DrawLineBPL2x2_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = DrawLineSegmentBG2x2_32Bit(linedescription->BG_pad_front, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  DrawLineSegmentBG2x2_32Bit(linedescription->BG_pad_back, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one bitplane line
// quad pixels
// double lines
//
// Pixel format: 32 bit RGB
//=========================

void DrawLineBPL4x2_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = DrawLineSegmentBG4x2_32Bit(linedescription->BG_pad_front, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  DrawLineSegmentBG4x2_32Bit(linedescription->BG_pad_back, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one bitplane line
// quad pixels
// quad lines
//
// Pixel format: 32 bit RGB
//=========================

void DrawLineBPL4x4_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  framebufferLinePtr = DrawLineSegmentBG4x4_32Bit(linedescription->BG_pad_front, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
  framebufferLinePtr = linedescription->draw_line_BPL_res_routine(linedescription, framebufferLinePtr, nextlineoffset);
  DrawLineSegmentBG4x4_32Bit(linedescription->BG_pad_back, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one background line
// double pixels
// single lines
//
// Pixel format: 32 bit RGB
//=========================

void DrawLineBG2x1_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  DrawLineSegmentBG2x1_32Bit(linedescription->MaxClipWidth / 4, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one background line
// double pixels
// double lines
//
// Pixel format: 32 bit RGB
//=========================

void DrawLineBG2x2_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  DrawLineSegmentBG2x2_32Bit(linedescription->MaxClipWidth / 4, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one background line
// quad pixels
// double lines
//
// Pixel format: 32 bit RGB
//=========================

void DrawLineBG4x2_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  DrawLineSegmentBG4x2_32Bit(linedescription->MaxClipWidth / 4, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//=========================
// Draw one background line
// quad pixels
// quad lines
//
// Pixel format: 32 bit RGB
//=========================

void DrawLineBG4x4_32Bit(const graph_line *linedescription, uint8_t *framebufferLinePtr, ptrdiff_t nextlineoffset)
{
  DrawLineSegmentBG4x4_32Bit(linedescription->MaxClipWidth / 4, linedescription->colors[0], framebufferLinePtr, nextlineoffset);
}

//========================================================================
// Lookup tables that holds all the drawing routines for various Amiga and
// host screen modes [color depth (3)][sizes (4)]
//========================================================================

draw_line_func PixelRenderers::draw_line_BPL_manage_funcs[3][4] = {
    {DrawLineBPL2x1_16Bit, DrawLineBPL2x2_16Bit, DrawLineBPL4x2_16Bit, DrawLineBPL4x4_16Bit},
    {DrawLineBPL2x1_24Bit, DrawLineBPL2x2_24Bit, DrawLineBPL4x2_24Bit, DrawLineBPL4x4_24Bit},
    {DrawLineBPL2x1_32Bit, DrawLineBPL2x2_32Bit, DrawLineBPL4x2_32Bit, DrawLineBPL4x4_32Bit}};

draw_line_func PixelRenderers::draw_line_BG_funcs[3][4] = {
    {DrawLineBG2x1_16Bit, DrawLineBG2x2_16Bit, DrawLineBG4x2_16Bit, DrawLineBG4x4_16Bit},
    {DrawLineBG2x1_24Bit, DrawLineBG2x2_24Bit, DrawLineBG4x2_24Bit, DrawLineBG4x4_24Bit},
    {DrawLineBG2x1_32Bit, DrawLineBG2x2_32Bit, DrawLineBG4x2_32Bit, DrawLineBG4x4_32Bit}};

draw_line_BPL_segment_func PixelRenderers::draw_line_lores_funcs[3][4] = {
    {DrawLineNormal2x1_16Bit, DrawLineNormal2x2_16Bit, DrawLineNormal4x2_16Bit, DrawLineNormal4x4_16Bit},
    {DrawLineNormal2x1_24Bit, DrawLineNormal2x2_24Bit, DrawLineNormal4x2_24Bit, DrawLineNormal4x4_24Bit},
    {DrawLineNormal2x1_32Bit, DrawLineNormal2x2_32Bit, DrawLineNormal4x2_32Bit, DrawLineNormal4x4_32Bit}};

draw_line_BPL_segment_func PixelRenderers::draw_line_hires_funcs[3][4] = {
    {DrawLineNormal1x1_16Bit, DrawLineNormal1x2_16Bit, DrawLineNormal2x2_16Bit, DrawLineNormal2x4_16Bit},
    {DrawLineNormal1x1_24Bit, DrawLineNormal1x2_24Bit, DrawLineNormal2x2_24Bit, DrawLineNormal2x4_24Bit},
    {DrawLineNormal1x1_32Bit, DrawLineNormal1x2_32Bit, DrawLineNormal2x2_32Bit, DrawLineNormal2x4_32Bit}};

draw_line_BPL_segment_func PixelRenderers::draw_line_dual_lores_funcs[3][4] = {
    {DrawLineDual2x1_16Bit, DrawLineDual2x2_16Bit, DrawLineDual4x2_16Bit, DrawLineDual4x4_16Bit},
    {DrawLineDual2x1_24Bit, DrawLineDual2x2_24Bit, DrawLineDual4x2_24Bit, DrawLineDual4x4_24Bit},
    {DrawLineDual2x1_32Bit, DrawLineDual2x2_32Bit, DrawLineDual4x2_32Bit, DrawLineDual4x4_32Bit}};

draw_line_BPL_segment_func PixelRenderers::draw_line_dual_hires_funcs[3][4] = {
    {DrawLineDual1x1_16Bit, DrawLineDual1x2_16Bit, DrawLineDual2x2_16Bit, DrawLineDual2x4_16Bit},
    {DrawLineDual1x1_24Bit, DrawLineDual1x2_24Bit, DrawLineDual2x2_24Bit, DrawLineDual2x4_24Bit},
    {DrawLineDual1x1_32Bit, DrawLineDual1x2_32Bit, DrawLineDual2x2_32Bit, DrawLineDual2x4_32Bit}};

draw_line_BPL_segment_func PixelRenderers::draw_line_HAM_lores_funcs[3][4] = {
    {DrawLineHAM2x1_16Bit, DrawLineHAM2x2_16Bit, DrawLineHAM4x2_16Bit, DrawLineHAM4x4_16Bit},
    {DrawLineHAM2x1_24Bit, DrawLineHAM2x2_24Bit, DrawLineHAM4x2_24Bit, DrawLineHAM4x4_24Bit},
    {DrawLineHAM2x1_32Bit, DrawLineHAM2x2_32Bit, DrawLineHAM4x2_32Bit, DrawLineHAM4x4_32Bit}};

unsigned int PixelRenderers::ColorBitsToFunctionLookupIndex(unsigned int colorBits)
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

unsigned int PixelRenderers::ScaleFactorToFunctionLookupIndex(ULO coreBufferScaleFactor, bool useInterlacedRendering, DisplayScaleStrategy displayScaleStrategy)
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

void PixelRenderers::ModeFunctionsInitialize(unsigned int activeBufferColorBits, unsigned int chipsetBufferScaleFactor, DisplayScaleStrategy displayScaleStrategy)
{
  unsigned int colordepthIndex = ColorBitsToFunctionLookupIndex(activeBufferColorBits);
  unsigned int scaleIndex = ScaleFactorToFunctionLookupIndex(chipsetBufferScaleFactor, drawGetUseInterlacedRendering(), displayScaleStrategy);

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

void PixelRenderers::SetLineRoutine(draw_line_func drawLineFunction)
{
  draw_line_routine = drawLineFunction;
}
