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

#ifndef PLANAR2CHUNKYDECODER_H
#define PLANAR2CHUNKYDECODER_H

#include "DEFS.H"
#include "BitplaneUtility.h"

typedef union ByteLongArrayUnion_
{
  UBY barray[1024];
  ByteWordUnion bwu[512];
  ByteLongUnion blu[256];
} ByteLongArrayUnion;

class Planar2ChunkyDecoder
{
private:
  ULO _batch_size;
  ByteLongArrayUnion _playfield_odd;
  ByteLongArrayUnion _playfield_even;
  ByteLongArrayUnion _playfield_ham_sprites;

  ULO *GetEvenPlayfieldULOPtr();
  ULO *GetOddPlayfieldULOPtr();

  ULO P2COdd1(ULO dat1, ULO dat3, ULO dat5);
  ULO P2COdd2(ULO dat1, ULO dat3, ULO dat5);
  ULO P2CEven1(ULO dat2, ULO dat4, ULO dat6);
  ULO P2CEven2(ULO dat2, ULO dat4, ULO dat6);
  ULO P2CDual1(ULO dat1, ULO dat2, ULO dat3);
  ULO P2CDual2(ULO dat1, ULO dat2, ULO dat3);

  void P2CNextPixelsNormal(ULO pixelCount, ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);
  void P2CNextPixelsDual(ULO pixelCount, ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);
  void P2CNext4PixelsNormal(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);
  void P2CNext4PixelsDual(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);
  void P2CNext8PixelsNormal(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);
  void P2CNext8PixelsDual(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);

  void P2CNextBackgroundNormal(ULO pixelCount);
  void P2CNextBackgroundDual(ULO pixelCount);

public:
  UBY *GetOddPlayfield();
  UBY *GetEvenPlayfield();
  UBY *GetHamSpritesPlayfield();
  ULO GetBatchSize();

  void NewBatch();
  void P2CNextPixels(ULO pixelCount, ByteWordUnion* data);
  void P2CNext4Pixels(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);
  void P2CNext8Pixels(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);

  void P2CNextBackground(ULO pixelCount);
};

#endif
