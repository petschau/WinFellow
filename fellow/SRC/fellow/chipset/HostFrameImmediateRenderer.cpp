/*=========================================================================*/
/* Fellow                                                                  */
/* Translates Amiga pixels to host pixels                                  */
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

#include "fellow/chipset/HostFrameImmediateRenderer.h"
#include "fellow/chipset/HostFrame.h"
#include "fellow/chipset/DualPlayfieldMapper.h"
#include "fellow/chipset/Planar2ChunkyDecoder.h"
#include "fellow/application/HostRenderer.h"

extern ULO draw_HAM_modify_table[4][2];

constexpr auto drawHAMModifyTableBitIndex = 0;
constexpr auto drawHAMModifyTableHoldMask = 4;

HostFrameImmediateRenderer host_frame_immediate_renderer;

void HostFrameImmediateRenderer::SetPixel1xULL(ULL *dst, ULL hostColor)
{
  *dst = hostColor;
}

void HostFrameImmediateRenderer::SetPixel2xULL(ULL *dst, ULL hostColor)
{
  dst[0] = hostColor;
  dst[1] = hostColor;
}

void HostFrameImmediateRenderer::SetPixel4xULO(ULO *dst, ULO hostColor)
{
  dst[0] = hostColor;
  dst[1] = hostColor;
  dst[2] = hostColor;
  dst[3] = hostColor;
}

void HostFrameImmediateRenderer::DrawLores(ULO line, ULO startX, ULO pixelCount)
{
  ULL *tmpline = (ULL *)(host_frame.GetLine(line) + startX);
  UBY *playfield = Planar2ChunkyDecoder::GetOddPlayfieldStart<UBY>();
  const ULL *hostColors = Draw.GetHostColors();

  for (ULO i = 0; i < pixelCount; i++)
  {
    // Each lores pixel is 4 host pixels wide (shres host scale)
    SetPixel2xULL(tmpline, hostColors[playfield[i]]);
    tmpline += 2;
  }
}

void HostFrameImmediateRenderer::DrawLoresDual(ULO line, ULO startX, ULO pixelCount)
{
  DualPlayfieldMapper dualPlayfieldMapper(BitplaneUtility::IsPlayfield1Pri());
  ULL *tmpline = (ULL *)(host_frame.GetLine(line) + startX);

  const ULL *hostColors = Draw.GetHostColors();
  UBY *playfieldOdd = Planar2ChunkyDecoder::GetOddPlayfieldStart<UBY>();
  UBY *playfieldEven = Planar2ChunkyDecoder::GetEvenPlayfieldStart<UBY>();

  for (ULO i = 0; i < pixelCount; i++)
  {
    // Each lores pixel is 4 host pixels wide (shres host scale)
    SetPixel2xULL(tmpline, hostColors[dualPlayfieldMapper.Map(playfieldOdd[i], playfieldEven[i])]);
    tmpline += 2;
  }
}

void HostFrameImmediateRenderer::DrawLoresHam(ULO line, ULO startX, ULO pixelCount)
{
  ULO *tmpline = host_frame.GetLine(line) + startX;
  static ULO pixelColor;
  ULO outputColor;
  UBY *playfield = Planar2ChunkyDecoder::GetOddPlayfieldStart<UBY>();
  UBY *spritePlayfield = Planar2ChunkyDecoder::GetHamSpritesPlayfieldStart<UBY>();
  const ULL *hostColors = Draw.GetHostColors();

  for (ULO i = 0; i < pixelCount; i++)
  {
    UBY amigaColorIndex = playfield[i];
    UBY controlBits = amigaColorIndex & 0x30;
    if (controlBits == 0)
    {
      pixelColor = outputColor = (ULO)hostColors[amigaColorIndex];
    }
    else
    {
      UBY *holdMask = (UBY *)draw_HAM_modify_table + (controlBits >> 1);
      ULO bitIndex = *(ULO *)(holdMask + drawHAMModifyTableBitIndex);
      pixelColor &= *(ULO *)(holdMask + drawHAMModifyTableHoldMask);
      pixelColor |= (amigaColorIndex & 0xf) << (bitIndex & 0xff);
      outputColor = pixelColor;
    }

    if (spritePlayfield[i] != 0)
    {
      outputColor = (ULO)hostColors[spritePlayfield[i]];
    }

    // Each lores pixel is 4 host pixels wide (shres host scale)
    SetPixel4xULO(tmpline, outputColor);
    tmpline += 4;
  }
}

void HostFrameImmediateRenderer::DrawHires(ULO line, ULO startX, ULO pixelCount)
{
  ULL *tmpline = (ULL *)(host_frame.GetLine(line) + startX);
  UBY *playfield = Planar2ChunkyDecoder::GetOddPlayfieldStart<UBY>();
  const ULL *hostColors = Draw.GetHostColors();

  for (ULO i = 0; i < pixelCount; i++)
  {
    SetPixel1xULL(tmpline + i, hostColors[playfield[i]]);
  }
}

void HostFrameImmediateRenderer::DrawHiresDual(ULO line, ULO startX, ULO pixelCount)
{
  DualPlayfieldMapper dualPlayfieldMapper(BitplaneUtility::IsPlayfield1Pri());
  ULL *tmpline = (ULL *)(host_frame.GetLine(line) + startX);

  const ULL *hostColors = Draw.GetHostColors();
  UBY *playfieldOdd = Planar2ChunkyDecoder::GetOddPlayfieldStart<UBY>();
  UBY *playfieldEven = Planar2ChunkyDecoder::GetEvenPlayfieldStart<UBY>();
  for (ULO i = 0; i < pixelCount; i++)
  {
    SetPixel1xULL(tmpline + i, hostColors[dualPlayfieldMapper.Map(playfieldOdd[i], playfieldEven[i])]);
  }
}

void HostFrameImmediateRenderer::DrawNothing(ULO line, ULO startX, ULO pixelCount)
{
  ULL *tmpline = (ULL *)(host_frame.GetLine(line) + startX);
  const ULL pixelColor = Draw.GetHostColors()[0];

  for (ULO i = 0; i < pixelCount; i++)
  {
    SetPixel2xULL(tmpline, pixelColor);
    tmpline += 2;
  }
}

void HostFrameImmediateRenderer::UpdateDrawBatchFunc()
{
  if (BitplaneUtility::IsHires())
  {
    if (BitplaneUtility::IsDualPlayfield())
    {
      _currentDrawBatchFunc = &HostFrameImmediateRenderer::DrawHiresDual;
    }
    else
    {
      _currentDrawBatchFunc = &HostFrameImmediateRenderer::DrawHires;
    }
  }
  else
  {
    if (BitplaneUtility::IsDualPlayfield())
    {
      _currentDrawBatchFunc = &HostFrameImmediateRenderer::DrawLoresDual;
    }
    else if (BitplaneUtility::IsHam())
    {
      _currentDrawBatchFunc = &HostFrameImmediateRenderer::DrawLoresHam;
    }
    else
    {
      _currentDrawBatchFunc = &HostFrameImmediateRenderer::DrawLores;
    }
  }
}

void HostFrameImmediateRenderer::DrawBatchImmediate(ULO line, ULO startX)
{
  (this->*_currentDrawBatchFunc)(line, startX, Planar2ChunkyDecoder::GetBatchSize());
}

void HostFrameImmediateRenderer::InitializeNewFrame(unsigned int linesInFrame)
{
  host_frame.LinesInFrame = linesInFrame;
}

HostFrameImmediateRenderer::HostFrameImmediateRenderer() : _currentDrawBatchFunc(&HostFrameImmediateRenderer::DrawLores)
{
}
