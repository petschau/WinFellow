/*=========================================================================*/
/* Fellow                                                                  */
/* Keeps track of sprite state                                             */
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
#include "fellow/chipset/SpriteRegisters.h"
#include "fellow/chipset/SpriteP2CDecoder.h"
#include "fellow/chipset/SpriteMerger.h"
#include "fellow/chipset/CycleExactSprites.h"
#include "fellow/chipset/BitplaneUtility.h"
#include "fellow/chipset/BitplaneShifter.h"
#include "fellow/chipset/Planar2ChunkyDecoder.h"
#include "fellow/chipset/SpriteDMA.h"

bool CycleExactSprites::Is16Color(const unsigned int spriteNo) const
{
  const unsigned int evenSpriteNo = spriteNo & 0xe;
  return SpriteState[evenSpriteNo].attached || SpriteState[evenSpriteNo + 1].attached;
}

void CycleExactSprites::Arm(const unsigned int spriteNumber)
{
  bitplane_shifter.Flush();

  // Actually this is kind of wrong since each sprite shifts independently
  const bool is16Color = Is16Color(spriteNumber);
  Sprite &state = SpriteState[spriteNumber];

  if (is16Color && (spriteNumber & 1) == 0) // Only arm the odd sprite
  {
    // Arm(spriteNumber + 1);
    state.armed = false;
    return;
  }

  state.armed = true;
  state.pixels_output = 0;
  state.dat_hold[0].w = sprite_registers.sprdata[spriteNumber];
  state.dat_hold[1].w = sprite_registers.sprdatb[spriteNumber];

  if (is16Color)
  {
    state.dat_hold[2].w = sprite_registers.sprdata[spriteNumber - 1];
    state.dat_hold[3].w = sprite_registers.sprdatb[spriteNumber - 1];

    SpriteP2CDecoder::Decode16(&(state.dat_decoded.blu[0].l), state.dat_hold[3].w, state.dat_hold[2].w, state.dat_hold[1].w, state.dat_hold[0].w);
  }
  else
  {
    SpriteP2CDecoder::Decode4(spriteNumber, &(state.dat_decoded.blu[0].l), state.dat_hold[1].w, state.dat_hold[0].w);
  }
}

void CycleExactSprites::MergeLores(unsigned int spriteNo, ULO sourcePixelIndex, ULO pixelIndex, ULO pixelCount)
{
  UBY *playfield = &Planar2ChunkyDecoder::GetOddPlayfieldStart<UBY>()[pixelIndex >> 2];
  UBY *spriteData = &SpriteState[spriteNo].dat_decoded.barray[sourcePixelIndex];

  SpriteMerger::MergeLores(spriteNo, playfield, spriteData, pixelCount >> 2);
}

void CycleExactSprites::MergeHires(unsigned int spriteNo, ULO sourcePixelIndex, ULO pixelIndex, ULO pixelCount)
{
  UBY *playfield = &Planar2ChunkyDecoder::GetOddPlayfieldStart<UBY>()[pixelIndex >> 1];
  UBY *spriteData = &SpriteState[spriteNo].dat_decoded.barray[sourcePixelIndex];

  SpriteMerger::MergeHires(spriteNo, playfield, spriteData, pixelCount >> 2);
}

void CycleExactSprites::MergeDualLores(unsigned int spriteNo, ULO sourcePixelIndex, ULO pixelIndex, ULO pixelCount)
{
  UBY *playfield1 = &Planar2ChunkyDecoder::GetOddPlayfieldStart<UBY>()[pixelIndex >> 2];
  UBY *playfield2 = &Planar2ChunkyDecoder::GetEvenPlayfieldStart<UBY>()[pixelIndex >> 2];
  UBY *spriteData = &SpriteState[spriteNo].dat_decoded.barray[sourcePixelIndex];

  SpriteMerger::MergeDualLores(spriteNo, playfield1, playfield2, spriteData, pixelCount >> 2);
}

void CycleExactSprites::MergeDualHires(unsigned int spriteNo, ULO sourcePixelIndex, ULO pixelIndex, ULO pixelCount)
{
  UBY *playfield1 = &Planar2ChunkyDecoder::GetOddPlayfieldStart<UBY>()[pixelIndex >> 1];
  UBY *playfield2 = &Planar2ChunkyDecoder::GetEvenPlayfieldStart<UBY>()[pixelIndex >> 1];
  UBY *spriteData = &SpriteState[spriteNo].dat_decoded.barray[sourcePixelIndex];

  SpriteMerger::MergeDualHires(spriteNo, playfield1, playfield2, spriteData, pixelCount >> 2);
}

void CycleExactSprites::MergeHam(unsigned int spriteNo, ULO sourcePixelIndex, ULO pixelIndex, ULO pixelCount)
{
  UBY *hamSpritesPlayfield = &Planar2ChunkyDecoder::GetHamSpritesPlayfieldStart<UBY>()[pixelIndex >> 2];
  UBY *spriteData = &SpriteState[spriteNo].dat_decoded.barray[sourcePixelIndex];

  SpriteMerger::MergeHam(spriteNo, hamSpritesPlayfield, spriteData, pixelCount >> 2);
}

void CycleExactSprites::Merge(const unsigned int spriteNo, ULO sourcePixelIndex, ULO pixelIndex, ULO pixelCount)
{
  // Not used
}

bool CycleExactSprites::StartsInRange(const unsigned int spriteNo, ULO startX, ULO pixelCount) const
{
  const ULO visibleAtX = SpriteState[spriteNo].x;

  return (visibleAtX >= startX) && (visibleAtX < (startX + pixelCount));
}

// Parameters are in shres pixel units in batch
void CycleExactSprites::OutputPartialSprite(unsigned int spriteNo, Sprite &state, ULO pixelIndex, ULO pixelCount)
{
  ULO outputSpritePixelCount = pixelCount - pixelIndex;
  ULO spritePixelsLeftToSerialize = 64 - state.pixels_output;
  if (outputSpritePixelCount > spritePixelsLeftToSerialize)
  {
    outputSpritePixelCount = spritePixelsLeftToSerialize;
  }

  if (BitplaneUtility::IsLores())
  {
    if (BitplaneUtility::IsDualPlayfield())
    {
      MergeDualLores(spriteNo, state.pixels_output >> 2, pixelIndex, outputSpritePixelCount);
    }
    else if (BitplaneUtility::IsHam())
    {
      MergeHam(spriteNo, state.pixels_output >> 2, pixelIndex, outputSpritePixelCount);
    }
    else
    {
      MergeLores(spriteNo, state.pixels_output >> 2, pixelIndex, outputSpritePixelCount);
    }
  }
  else // Hires
  {
    if (BitplaneUtility::IsDualPlayfield())
    {
      MergeDualHires(spriteNo, state.pixels_output >> 2, pixelIndex, outputSpritePixelCount);
    }
    else
    {
      MergeHires(spriteNo, state.pixels_output >> 2, pixelIndex, outputSpritePixelCount);
    }
  }

  state.pixels_output += outputSpritePixelCount;
  state.serializing = (state.pixels_output < 64);
}

// Parameters are in shres pixel units in line
void CycleExactSprites::OutputSprite(const unsigned int spriteNo, ULO batchStartX, ULO batchPixelCount)
{
  ULO spriteNextXInBatch = 0;
  Sprite &state = SpriteState[spriteNo];

  if (state.serializing && StartsInRange(spriteNo, batchStartX, batchPixelCount))
  {
    // Output current sprite until the new position activates
    OutputPartialSprite(spriteNo, state, 0, state.x - batchStartX);
    state.serializing = false;
  }

  // Look for start of sprite output
  if (!state.serializing && StartsInRange(spriteNo, batchStartX, batchPixelCount))
  {
    state.serializing = true;
    spriteNextXInBatch = state.x - batchStartX;
    state.pixels_output = 0;

    F_ASSERT(((LON)spriteNextXInBatch) >= 0);
  }

  if (state.serializing)
  {
    OutputPartialSprite(spriteNo, state, spriteNextXInBatch, batchPixelCount);
  }
}

// Parameters are shres pixel units in a line
void CycleExactSprites::OutputSprites(ULO startX, ULO pixelCount)
{
  if (BitplaneUtility::IsLores() && BitplaneUtility::IsHam())
  {
    Planar2ChunkyDecoder::ClearHAMSpritesPlayfield();
  }

  for (unsigned int spriteNumber = 0; spriteNumber < 8; ++spriteNumber)
  {
    if (SpriteState[spriteNumber].armed)
    {
      OutputSprite(spriteNumber, startX, pixelCount);
    }
  }
}

/* IO Register handlers */

/* SPRXPT - $dff120 to $dff13e */

void CycleExactSprites::NotifySprpthChanged(UWO data, const unsigned int spriteNumber)
{
}

void CycleExactSprites::NotifySprptlChanged(UWO data, const unsigned int spriteNumber)
{
}

/* SPRXPOS - $dff140 to $dff178 */

void CycleExactSprites::NotifySprposChanged(UWO data, const unsigned int spriteNumber)
{
  Sprite &state = SpriteState[spriteNumber];
  ULO newX = (state.x & 7) | ((data & 0xff) << 3);

  if (state.armed && (newX != state.x))
  {
    bitplane_shifter.Flush();
  }

  state.x = newX;
  sprite_dma.SetVStartLow(spriteNumber, data >> 8);
}

/* SPRXCTL $dff142 to $dff17a */

void CycleExactSprites::NotifySprctlChanged(UWO data, const unsigned int spriteNumber)
{
  Sprite &state = SpriteState[spriteNumber];

  if (state.armed)
  {
    bitplane_shifter.Flush();
  }

  state.x = (state.x & 0x7f8) | ((data & 1) << 2);
  sprite_dma.SetVStartHigh(spriteNumber, (data & 0x4) << 6);
  sprite_dma.SetVStop(spriteNumber, (data >> 8) | ((data & 0x2) << 7));

  // Odd sprite attach bit controls both sprites
  state.attached = !!(data & 0x80);
  if (spriteNumber & 1)
  {
    SpriteState[spriteNumber - 1].attached = state.attached;
  }

  state.armed = false;
}

/* SPRXDATA $dff144 to $dff17c */

void CycleExactSprites::NotifySprdataChanged(UWO data, const unsigned int spriteNumber)
{
  Arm(spriteNumber);
}

/* SPRXDATB $dff146 to $dff17e */

void CycleExactSprites::NotifySprdatbChanged(UWO data, const unsigned int spriteNumber)
{
  SpriteState[spriteNumber].armed = false;
}

/* Module management */

void CycleExactSprites::EndOfLine(ULO line)
{
  for (auto &i : SpriteState)
  {
    i.serializing = false;
  }
}

void CycleExactSprites::EndOfFrame()
{
}

void CycleExactSprites::ClearState()
{
  memset(SpriteState, 0, sizeof(Sprite) * 8);
}

void CycleExactSprites::HardReset()
{
}

void CycleExactSprites::EmulationStart()
{
  ClearState(); // ??????
}

void CycleExactSprites::EmulationStop()
{
}

CycleExactSprites::CycleExactSprites() : Sprites()
{
}

CycleExactSprites::~CycleExactSprites() = default;
