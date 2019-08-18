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

#include "fellow/api/defs.h"
#include "fellow/chipset/Planar2ChunkyDecoder.h"

//----------------------------------------------------------------------------
// Planar to chunky conversion
//
// One byte of bitplane data is converted to 8 chunky bytes in two steps
// via. lookup tables.
//----------------------------------------------------------------------------

ULO Planar2ChunkyDecoder::_batchSize;
ULO Planar2ChunkyDecoder::_batchStart;
ByteLongArrayUnion Planar2ChunkyDecoder::_playfieldOdd;
ByteLongArrayUnion Planar2ChunkyDecoder::_playfieldEven;
ByteLongArrayUnion Planar2ChunkyDecoder::_playfieldHamSprites;

ULO Planar2ChunkyDecoder::_decode1[256][2];
ULO Planar2ChunkyDecoder::_decode2[256][2];
ULO Planar2ChunkyDecoder::_decode3[256][2];
ULO Planar2ChunkyDecoder::_decode4[256][2];
ULO Planar2ChunkyDecoder::_decode5[256][2];
ULO Planar2ChunkyDecoder::_decode6[256][2];

template <typename T> T *Planar2ChunkyDecoder::GetEvenPlayfield()
{
  return (T *)(_playfieldEven.barray + _batchStart + _batchSize);
}

template <typename T> T *Planar2ChunkyDecoder::GetOddPlayfield()
{
  return (T *)(_playfieldOdd.barray + _batchStart + _batchSize);
}

// Decode the odd part of the first 4 pixels.
// Normal lores/hires output
ULO Planar2ChunkyDecoder::P2COdd1(ULO dat1, ULO dat3, ULO dat5)
{
  return _decode1[dat1][0] | _decode3[dat3][0] | _decode5[dat5][0];
}

// Decode the odd part of the last 4 pixels
// Normal lores/hires output
ULO Planar2ChunkyDecoder::P2COdd2(ULO dat1, ULO dat3, ULO dat5)
{
  return _decode1[dat1][1] | _decode3[dat3][1] | _decode5[dat5][1];
}

// Decode the even part of the first 4 pixels
// Normal lores/hires output
ULO Planar2ChunkyDecoder::P2CEven1(ULO dat2, ULO dat4, ULO dat6)
{
  return _decode2[dat2][0] | _decode4[dat4][0] | _decode6[dat6][0];
}

// Decode the even part of the last 4 pixels
// Normal lores/hires output
ULO Planar2ChunkyDecoder::P2CEven2(ULO dat2, ULO dat4, ULO dat6)
{
  return _decode2[dat2][1] | _decode4[dat4][1] | _decode6[dat6][1];
}

// Decode the first 4 pixels
// Dual playfield output.
ULO Planar2ChunkyDecoder::P2CDual1(ULO dat1, ULO dat2, ULO dat3)
{
  return _decode1[dat1][0] | _decode2[dat2][0] | _decode3[dat3][0];
}

// Decode the last 4 pixels
// Dual playfield output.
ULO Planar2ChunkyDecoder::P2CDual2(ULO dat1, ULO dat2, ULO dat3)
{
  return _decode1[dat1][1] | _decode2[dat2][1] | _decode3[dat3][1];
}

//----------------------------------------------------------------------------
// Mode dependent planar to chunky decoding of the next pixels.
// The result is stored in the playfield pixel output stream.

void Planar2ChunkyDecoder::P2CNextBackgroundPixels(ULO pixelCount)
{
  if (pixelCount >= 8)
  {
    ULL *playfieldULL = GetOddPlayfield<ULL>();
    ULO pixel8Iterations = pixelCount / 8;

    for (ULO i = 0; i < pixel8Iterations; i++)
    {
      playfieldULL[i] = 0;
    }

    _batchSize += pixel8Iterations * 8;
  }

  ULO remainingPixels = pixelCount & 7;
  if (remainingPixels > 0)
  {
    ULO *playfieldULO = GetOddPlayfield<ULO>();

    playfieldULO[0] = 0;

    if (remainingPixels > 4)
    {
      playfieldULO[1] = 0;
    }

    _batchSize += remainingPixels;
  }
}

void Planar2ChunkyDecoder::P2CNextBackgroundPixelsDual(ULO pixelCount)
{
  if (pixelCount >= 8)
  {
    ULL *playfieldOddULL = GetOddPlayfield<ULL>();
    ULL *playfieldEvenULL = GetEvenPlayfield<ULL>();
    ULO pixel8Iterations = pixelCount / 8;

    for (ULO i = 0; i < pixel8Iterations; i++)
    {
      playfieldOddULL[i] = 0;
      playfieldEvenULL[i] = 0;
    }

    _batchSize += pixel8Iterations * 8;
  }

  ULO remainingPixels = pixelCount & 7;
  if (remainingPixels > 0)
  {
    ULO *playfieldOddULO = GetOddPlayfield<ULO>();
    ULO *playfieldEvenULO = GetEvenPlayfield<ULO>();

    playfieldOddULO[0] = 0;
    playfieldEvenULO[0] = 0;

    if (remainingPixels > 4)
    {
      playfieldOddULO[1] = 0;
      playfieldEvenULO[1] = 0;
    }

    _batchSize += remainingPixels;
  }
}

void Planar2ChunkyDecoder::P2CNext8BackgroundPixelsDual()
{
  ULL *playfield_odd = GetOddPlayfield<ULL>();
  playfield_odd[0] = 0;

  ULL *playfield_even = GetEvenPlayfield<ULL>();
  playfield_even[0] = 0;

  _batchSize += 8;
}

// Assumes pixelCount is 4 or less
void Planar2ChunkyDecoder::P2CNextPixels(ULO pixelCount, ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  ULO pixels = P2COdd1(dat1, dat3, dat5) | P2CEven1(dat2, dat4, dat6);
  ULO *playfield = GetOddPlayfield<ULO>();
  playfield[0] = pixels;
  _batchSize += pixelCount;
}

// Assumes pixelCount is 4 or less
void Planar2ChunkyDecoder::P2CNextPixelsDual(ULO pixelCount, ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  ULO *playfield_odd = GetOddPlayfield<ULO>();
  playfield_odd[0] = P2CDual1(dat1, dat3, dat5);

  ULO *playfield_even = GetEvenPlayfield<ULO>();
  playfield_even[0] = P2CDual1(dat2, dat4, dat6);

  _batchSize += pixelCount;
}

void Planar2ChunkyDecoder::P2CNext4Pixels(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  ULO *playfield = GetOddPlayfield<ULO>();
  playfield[0] = P2COdd1(dat1, dat3, dat5) | P2CEven1(dat2, dat4, dat6);

  _batchSize += 4;
}

void Planar2ChunkyDecoder::P2CNext4PixelsDual(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  ULO *playfield_odd = GetOddPlayfield<ULO>();
  playfield_odd[0] = P2CDual1(dat1, dat3, dat5);

  ULO *playfield_even = GetEvenPlayfield<ULO>();
  playfield_even[0] = P2CDual1(dat2, dat4, dat6);

  _batchSize += 4;
}

void Planar2ChunkyDecoder::P2CNext8Pixels(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  ULO *playfield = GetOddPlayfield<ULO>();
  playfield[0] = P2COdd1(dat1, dat3, dat5) | P2CEven1(dat2, dat4, dat6);
  playfield[1] = P2COdd2(dat1, dat3, dat5) | P2CEven2(dat2, dat4, dat6);

  _batchSize += 8;
}

void Planar2ChunkyDecoder::P2CNext8PixelsDual(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  ULO *playfield_odd = GetOddPlayfield<ULO>();
  playfield_odd[0] = P2CDual1(dat1, dat3, dat5);
  playfield_odd[1] = P2CDual2(dat1, dat3, dat5);

  ULO *playfield_even = GetEvenPlayfield<ULO>();
  playfield_even[0] = P2CDual1(dat2, dat4, dat6);
  playfield_even[1] = P2CDual2(dat2, dat4, dat6);

  _batchSize += 8;
}

void Planar2ChunkyDecoder::NewImmediateBatch()
{
  _batchStart = 0;
  _batchSize = 0;
}

void Planar2ChunkyDecoder::NewChangelistBatch()
{
  _batchStart = _batchStart + _batchSize;
  _batchSize = 0;
}

void Planar2ChunkyDecoder::InitializeNewFrame()
{
  _batchStart = 0;
  _batchSize = 0;
}

ULO Planar2ChunkyDecoder::GetBatchSize()
{
  return _batchSize;
}

void Planar2ChunkyDecoder::ClearHAMSpritesPlayfield()
{
  const unsigned int iterations = (_batchSize + 7) / 8;
  ULL *hamSpritesPlayfieldULL = GetHamSpritesPlayfieldStart<ULL>();

  for (unsigned int i = 0; i < iterations; ++i)
  {
    hamSpritesPlayfieldULL[i] = 0;
  }
}

void Planar2ChunkyDecoder::InitializeP2CTables()
{
  for (unsigned int i = 0; i < 256; i++)
  {
    ULO d[2]{};

    for (unsigned int j = 0; j < 4; j++)
    {
      d[0] |= ((i & (0x80 >> j)) >> (4 + 3 - j)) << (j * 8);
      d[1] |= ((i & (0x8 >> j)) >> (3 - j)) << (j * 8);
    }

    for (unsigned int j = 0; j < 2; j++)
    {
      _decode1[i][j] = d[j];
      _decode2[i][j] = d[j] << 1;
      _decode3[i][j] = d[j] << 2;
      _decode4[i][j] = d[j] << 3;
      _decode5[i][j] = d[j] << 4;
      _decode6[i][j] = d[j] << 5;
    }
  }
}

void Planar2ChunkyDecoder::Startup()
{
  InitializeP2CTables();
}

void Planar2ChunkyDecoder::Shutdown()
{
}
