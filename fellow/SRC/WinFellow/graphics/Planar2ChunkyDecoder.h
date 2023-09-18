#pragma once

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

#include "DEFS.H"
#include "CoreHost.h"

using namespace CustomChipset;

typedef union ByteLongArrayUnion_
{
  uint8_t barray[1024];
  ByteWordUnion bwu[512];
  ByteLongUnion blu[256];
} ByteLongArrayUnion;

class Planar2ChunkyDecoder
{
private:
  uint32_t _batch_size;
  ByteLongArrayUnion _playfield_odd;
  ByteLongArrayUnion _playfield_even;
  ByteLongArrayUnion _playfield_ham_sprites;

  uint32_t *GetEvenPlayfieldUint32Ptr(void);
  uint32_t *GetOddPlayfieldUint32Ptr(void);

  uint32_t P2COdd1(uint32_t dat1, uint32_t dat3, uint32_t dat5);
  uint32_t P2COdd2(uint32_t dat1, uint32_t dat3, uint32_t dat5);
  uint32_t P2CEven1(uint32_t dat2, uint32_t dat4, uint32_t dat6);
  uint32_t P2CEven2(uint32_t dat2, uint32_t dat4, uint32_t dat6);
  uint32_t P2CDual1(uint32_t dat1, uint32_t dat2, uint32_t dat3);
  uint32_t P2CDual2(uint32_t dat1, uint32_t dat2, uint32_t dat3);

  void P2CNextPixelsNormal(uint32_t pixelCount, uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6);
  void P2CNextPixelsDual(uint32_t pixelCount, uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6);
  void P2CNext4PixelsNormal(uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6);
  void P2CNext4PixelsDual(uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6);
  void P2CNext8PixelsNormal(uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6);
  void P2CNext8PixelsDual(uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6);

public:
  uint8_t *GetOddPlayfield(void);
  uint8_t *GetEvenPlayfield(void);
  uint8_t *GetHamSpritesPlayfield(void);
  uint32_t GetBatchSize(void);

  void NewBatch(void);
  void P2CNextPixels(uint32_t pixelCount, uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6);
  void P2CNext4Pixels(uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6);
  void P2CNext8Pixels(uint32_t dat1, uint32_t dat2, uint32_t dat3, uint32_t dat4, uint32_t dat5, uint32_t dat6);
};
