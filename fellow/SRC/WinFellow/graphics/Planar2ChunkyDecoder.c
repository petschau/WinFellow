/*=========================================================================*/
/* Fellow                                                                  */
/* Planar2ChunkyDecoder                                                    */
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
#include "Graphics.h"

using namespace CustomChipset;

extern uint32_t graph_deco1[256][2];
extern uint32_t graph_deco2[256][2];
extern uint32_t graph_deco3[256][2];
extern uint32_t graph_deco4[256][2];
extern uint32_t graph_deco5[256][2];
extern uint32_t graph_deco6[256][2];

//----------------------------------------------------------------------------
// Planar to chunky conversion
//
// One byte of bitplane data is converted to 8 chunky bytes in two steps
// via. lookup tables.
//
//----------------------------------------------------------------------------

uint32_t *Planar2ChunkyDecoder::GetEvenPlayfieldUint32Ptr()
{
  return (uint32_t *)(_playfield_even.barray + _batch_size);
}
uint32_t *Planar2ChunkyDecoder::GetOddPlayfieldUint32Ptr()
{
  return (uint32_t *)(_playfield_odd.barray + _batch_size);
}

// Decode the odd part of the first 4 pixels.
// Normal lores/hires output
uint32_t Planar2ChunkyDecoder::P2COdd1(uint32_t dat1, uint32_t dat3, uint32_t dat5)
{
  return graph_deco1[dat1][0] | graph_deco3[dat3][0] | graph_deco5[dat5][0];
}

// Decode the odd part of the last 4 pixels
// Normal lores/hires output
uint32_t Planar2ChunkyDecoder::P2COdd2(uint32_t dat1, uint32_t dat3, uint32_t dat5)
{
  return graph_deco1[dat1][1] | graph_deco3[dat3][1] | graph_deco5[dat5][1];
}

// Decode the even part of the first 4 pixels
// Normal lores/hires output
uint32_t Planar2ChunkyDecoder::P2CEven1(uint32_t dat2, uint32_t dat4, uint32_t dat6)
{
  return graph_deco2[dat2][0] | graph_deco4[dat4][0] | graph_deco6[dat6][0];
}

// Decode the even part of the last 4 pixels
// Normal lores/hires output
uint32_t Planar2ChunkyDecoder::P2CEven2(uint32_t dat2, uint32_t dat4, uint32_t dat6)
{
  return graph_deco2[dat2][1] | graph_deco4[dat4][1] | graph_deco6[dat6][1];
}

// Decode the first 4 pixels
// Dual playfield output.
uint32_t Planar2ChunkyDecoder::P2CDual1(uint32_t dat1, uint32_t dat2, uint32_t dat3)
{
  return graph_deco1[dat1][0] | graph_deco2[dat2][0] | graph_deco3[dat3][0];
}

// Decode the last 4 pixels
// Dual playfield output.
uint32_t Planar2ChunkyDecoder::P2CDual2(uint32_t dat1, uint32_t dat2, uint32_t dat3)
{
  return graph_deco1[dat1][1] | graph_deco2[dat2][1] | graph_deco3[dat3][1];
}

//----------------------------------------------------------------------------
// Mode dependent planar to chunky decoding of the next pixels.
// The result is stored in the playfield pixel output stream.

void Planar2ChunkyDecoder::P2CNextPixelsNormal(uint32_t pixelCount, uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6)
{
  uint32_t pixels = P2COdd1(dat1, dat3, dat5) | P2CEven1(dat2, dat4, dat6);
  uint32_t *playfield = GetOddPlayfieldUint32Ptr();
  playfield[0] = pixels;
  _batch_size += pixelCount;
}

void Planar2ChunkyDecoder::P2CNextPixelsDual(uint32_t pixelCount, uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6)
{
  uint32_t *playfield_odd = GetOddPlayfieldUint32Ptr();
  playfield_odd[0] = P2CDual1(dat1, dat3, dat5);

  uint32_t *playfield_even = GetEvenPlayfieldUint32Ptr();
  playfield_even[0] = P2CDual1(dat2, dat4, dat6);

  _batch_size += pixelCount;
}

void Planar2ChunkyDecoder::P2CNext4PixelsNormal(uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6)
{
  uint32_t *playfield = GetOddPlayfieldUint32Ptr();
  playfield[0] = P2COdd1(dat1, dat3, dat5) | P2CEven1(dat2, dat4, dat6);

  _batch_size += 4;
}

void Planar2ChunkyDecoder::P2CNext4PixelsDual(uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6)
{
  uint32_t *playfield_odd = GetOddPlayfieldUint32Ptr();
  playfield_odd[0] = P2CDual1(dat1, dat3, dat5);

  uint32_t *playfield_even = GetEvenPlayfieldUint32Ptr();
  playfield_even[0] = P2CDual1(dat2, dat4, dat6);

  _batch_size += 4;
}

void Planar2ChunkyDecoder::P2CNext8PixelsNormal(uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6)
{
  uint32_t *playfield = GetOddPlayfieldUint32Ptr();
  playfield[0] = P2COdd1(dat1, dat3, dat5) | P2CEven1(dat2, dat4, dat6);
  playfield[1] = P2COdd2(dat1, dat3, dat5) | P2CEven2(dat2, dat4, dat6);

  _batch_size += 8;
}

void Planar2ChunkyDecoder::P2CNext8PixelsDual(uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6)
{
  uint32_t *playfield_odd = GetOddPlayfieldUint32Ptr();
  playfield_odd[0] = P2CDual1(dat1, dat3, dat5);
  playfield_odd[1] = P2CDual2(dat1, dat3, dat5);

  uint32_t *playfield_even = GetEvenPlayfieldUint32Ptr();
  playfield_even[0] = P2CDual1(dat2, dat4, dat6);
  playfield_even[1] = P2CDual2(dat2, dat4, dat6);

  _batch_size += 8;
}

void Planar2ChunkyDecoder::P2CNextPixels(uint32_t pixelCount, uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6)
{
  if (_core.RegisterUtility.IsDualPlayfieldEnabled())
  {
    P2CNextPixelsDual(pixelCount, dat1, dat2, dat3, dat4, dat5, dat6);
  }
  else
  {
    P2CNextPixelsNormal(pixelCount, dat1, dat2, dat3, dat4, dat5, dat6);
  }
}

void Planar2ChunkyDecoder::P2CNext4Pixels(uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6)
{
  if (_core.RegisterUtility.IsDualPlayfieldEnabled())
  {
    P2CNext4PixelsDual(dat1, dat2, dat3, dat4, dat5, dat6);
  }
  else
  {
    P2CNext4PixelsNormal(dat1, dat2, dat3, dat4, dat5, dat6);
  }
}

void Planar2ChunkyDecoder::P2CNext8Pixels(uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6)
{
  if (_core.RegisterUtility.IsDualPlayfieldEnabled())
  {
    P2CNext8PixelsDual(dat1, dat2, dat3, dat4, dat5, dat6);
  }
  else
  {
    P2CNext8PixelsNormal(dat1, dat2, dat3, dat4, dat5, dat6);
  }
}

void Planar2ChunkyDecoder::NewBatch()
{
  _batch_size = 0;
}

uint8_t *Planar2ChunkyDecoder::GetOddPlayfield()
{
  return _playfield_odd.barray;
}

uint8_t *Planar2ChunkyDecoder::GetEvenPlayfield()
{
  return _playfield_even.barray;
}

uint8_t *Planar2ChunkyDecoder::GetHamSpritesPlayfield()
{
  return _playfield_ham_sprites.barray;
}

uint32_t Planar2ChunkyDecoder::GetBatchSize()
{
  return _batch_size;
}
