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

#pragma once

#include "fellow/api/defs.h"
#include "fellow/chipset/BitplaneUtility.h"

// constexpr auto Planar2ChunkyDecoderArraySize = 2048;
constexpr auto Planar2ChunkyDecoderArraySize = 2048 * 314;

typedef union ByteLongArrayUnion_ {
  UBY barray[Planar2ChunkyDecoderArraySize];
  ULO larray[Planar2ChunkyDecoderArraySize / 4];
} ByteLongArrayUnion;

class Planar2ChunkyDecoder
{
private:
  static ULO _batchStart;
  static ULO _batchSize;

  static ByteLongArrayUnion _playfieldOdd;
  static ByteLongArrayUnion _playfieldEven;
  static ByteLongArrayUnion _playfieldHamSprites;

  static ULO _decode1[256][2];
  static ULO _decode2[256][2];
  static ULO _decode3[256][2];
  static ULO _decode4[256][2];
  static ULO _decode5[256][2];
  static ULO _decode6[256][2];

  static void InitializeP2CTables();

  template <typename T> static T *GetEvenPlayfield();
  template <typename T> static T *GetOddPlayfield();

public:
  template <typename T> static T *GetOddPlayfieldStart()
  {
    return (T *)(_playfieldOdd.barray + _batchStart);
  }

  template <typename T> static T *GetEvenPlayfieldStart()
  {
    return (T *)(_playfieldEven.barray + _batchStart);
  }

  template <typename T> static T *GetHamSpritesPlayfieldStart()
  {
    return (T *)(_playfieldHamSprites.barray + _batchStart);
  }

  static ULO GetBatchSize();
  static void ClearHAMSpritesPlayfield();

  static void NewImmediateBatch();
  static void NewChangelistBatch();
  static void InitializeNewFrame();

  static ULO P2COdd1(ULO dat1, ULO dat3, ULO dat5);
  static ULO P2COdd2(ULO dat1, ULO dat3, ULO dat5);
  static ULO P2CEven1(ULO dat2, ULO dat4, ULO dat6);
  static ULO P2CEven2(ULO dat2, ULO dat4, ULO dat6);
  static ULO P2CDual1(ULO dat1, ULO dat2, ULO dat3);
  static ULO P2CDual2(ULO dat1, ULO dat2, ULO dat3);

  static void P2CNextBackgroundPixels(ULO pixelCount);

  static void P2CNext8BackgroundPixelsDual();
  static void P2CNextBackgroundPixelsDual(ULO pixelCount);

  static void P2CNextPixels(ULO pixelCount, ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);
  static void P2CNext4Pixels(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);
  static void P2CNext8Pixels(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);

  static void P2CNextPixelsDual(ULO pixelCount, ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);
  static void P2CNext4PixelsDual(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);
  static void P2CNext8PixelsDual(ULO dat1, ULO dat2, ULO dat3, ULO dat4, ULO dat5, ULO dat6);

  static void Startup();
  static void Shutdown();
};
