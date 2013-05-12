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

#include "defs.h"

#ifdef GRAPH2

#include "bus.h"
#include "graph.h"
#include "draw.h"
#include "fmem.h"
#include "fileops.h"

#include "Graphics.h"

extern ULO graph_deco1[256][2];
extern ULO graph_deco2[256][2];
extern ULO graph_deco3[256][2];
extern ULO graph_deco4[256][2];
extern ULO graph_deco5[256][2];
extern ULO graph_deco6[256][2];

extern UBY *draw_buffer_current_ptr;

//----------------------------------------------------------------------------
// Planar to chunky conversion
// 
// One byte of bitplane data is converted to 8 chunky bytes in two steps
// via. lookup tables.
//
//----------------------------------------------------------------------------

ULO *Planar2ChunkyDecoder::GetEvenPlayfieldULOPtr(void) {return (ULO*) (_playfield_even.barray + _batch_size);}
ULO *Planar2ChunkyDecoder::GetOddPlayfieldULOPtr(void) {return (ULO*) (_playfield_odd.barray + _batch_size);}

// Decode the odd part of the first 4 pixels.
// Normal lores/hires output
ULO Planar2ChunkyDecoder::P2COdd1(ULO dat1, ULO dat3, ULO dat5)
{
  return graph_deco1[dat1][0] | graph_deco3[dat3][0] | graph_deco5[dat5][0]; 
}

// Decode the odd part of the last 4 pixels
// Normal lores/hires output
ULO Planar2ChunkyDecoder::P2COdd2(ULO dat1, ULO dat3, ULO dat5)
{
  return graph_deco1[dat1][1] | graph_deco3[dat3][1] | graph_deco5[dat5][1]; 
}

// Decode the even part of the first 4 pixels
// Normal lores/hires output
ULO Planar2ChunkyDecoder::P2CEven1(ULO dat2, ULO dat4, ULO dat6)
{
  return graph_deco2[dat2][0] | graph_deco4[dat4][0] | graph_deco6[dat6][0];
}

// Decode the even part of the last 4 pixels
// Normal lores/hires output
ULO Planar2ChunkyDecoder::P2CEven2(ULO dat2, ULO dat4, ULO dat6)
{
  return graph_deco2[dat2][1] | graph_deco4[dat4][1] | graph_deco6[dat6][1]; 
}

// Decode the first 4 pixels
// Dual playfield output.
ULO Planar2ChunkyDecoder::P2CDual1(ULO dat1, ULO dat2, ULO dat3)
{
  return graph_deco1[dat1][0] | graph_deco2[dat2][0] | graph_deco3[dat3][0]; 
}

// Decode the last 4 pixels
// Dual playfield output.
ULO Planar2ChunkyDecoder::P2CDual2(ULO dat1, ULO dat2, ULO dat3)
{
  return graph_deco1[dat1][1] | graph_deco2[dat2][1] | graph_deco3[dat3][1]; 
}

//----------------------------------------------------------------------------
// Mode dependent planar to chunky decoding of the next pixels.
// The result is stored in the playfield pixel output stream.

void Planar2ChunkyDecoder::P2CNextPixelsNormal(ULO pixelCount, ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  ULO pixels = P2COdd1(dat1, dat3, dat5) | P2CEven1(dat2, dat4, dat6);
  ULO *playfield = GetOddPlayfieldULOPtr();
  playfield[0] = pixels; 
  _batch_size += pixelCount;
}

void Planar2ChunkyDecoder::P2CNextPixelsDual(ULO pixelCount, ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  ULO *playfield_odd = GetOddPlayfieldULOPtr();
  playfield_odd[0] = P2CDual1(dat1, dat3, dat5);

  ULO *playfield_even = GetEvenPlayfieldULOPtr();
  playfield_even[0] = P2CDual1(dat2, dat4, dat6);

  _batch_size += pixelCount;
}

void Planar2ChunkyDecoder::P2CNext4PixelsNormal(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  ULO *playfield = GetOddPlayfieldULOPtr();
  playfield[0] = P2COdd1(dat1, dat3, dat5) | P2CEven1(dat2, dat4, dat6); 

  _batch_size += 4;
}

void Planar2ChunkyDecoder::P2CNext4PixelsDual(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  ULO *playfield_odd = GetOddPlayfieldULOPtr();
  playfield_odd[0] = P2CDual1(dat1, dat3, dat5); 

  ULO *playfield_even = GetEvenPlayfieldULOPtr();
  playfield_even[0] = P2CDual1(dat2, dat4, dat6);

  _batch_size += 4;
}

void Planar2ChunkyDecoder::P2CNext8PixelsNormal(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  ULO *playfield = GetOddPlayfieldULOPtr();
  playfield[0] = P2COdd1(dat1, dat3, dat5) | P2CEven1(dat2, dat4, dat6);
  playfield[1] = P2COdd2(dat1, dat3, dat5) | P2CEven2(dat2, dat4, dat6);

  _batch_size += 8;
}

void Planar2ChunkyDecoder::P2CNext8PixelsDual(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  ULO *playfield_odd = GetOddPlayfieldULOPtr();
  playfield_odd[0] = P2CDual1(dat1, dat3, dat5); 
  playfield_odd[1] = P2CDual2(dat1, dat3, dat5); 

  ULO *playfield_even = GetEvenPlayfieldULOPtr();
  playfield_even[0] = P2CDual1(dat2, dat4, dat6); 
  playfield_even[1] = P2CDual2(dat2, dat4, dat6);

  _batch_size += 8;
}

void Planar2ChunkyDecoder::P2CNextPixels(ULO pixelCount, ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  if (BitplaneUtility::IsDualPlayfield())
  {
    P2CNextPixelsDual(pixelCount, dat1, dat2, dat3, dat4, dat5, dat6);
  }
  else
  {
    P2CNextPixelsNormal(pixelCount, dat1, dat2, dat3, dat4, dat5, dat6);
  }
}

void Planar2ChunkyDecoder::P2CNext4Pixels(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  if (BitplaneUtility::IsDualPlayfield())
  {
    P2CNext4PixelsDual(dat1, dat2, dat3, dat4, dat5, dat6);
  }
  else
  {
    P2CNext4PixelsNormal(dat1, dat2, dat3, dat4, dat5, dat6);
  }
}

void Planar2ChunkyDecoder::P2CNext8Pixels(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6)
{
  if (BitplaneUtility::IsDualPlayfield())
  {
    P2CNext8PixelsDual(dat1, dat2, dat3, dat4, dat5, dat6);
  }
  else
  {
    P2CNext8PixelsNormal(dat1, dat2, dat3, dat4, dat5, dat6);
  }
}

void Planar2ChunkyDecoder::NewBatch(void)
{
  _batch_size = 0;
}

UBY *Planar2ChunkyDecoder::GetOddPlayfield(void)
{
  return _playfield_odd.barray;
}

UBY *Planar2ChunkyDecoder::GetEvenPlayfield(void)
{
  return _playfield_even.barray;
}

ULO Planar2ChunkyDecoder::GetBatchSize(void)
{
  return _batch_size;
}

#endif
