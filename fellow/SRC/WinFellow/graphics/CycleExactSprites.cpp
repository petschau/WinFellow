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

#include "Defs.h"

#include "chipset.h"
#include "BusScheduler.h"
#include "GraphicsPipeline.h"
#include "Renderer.h"
#include "MemoryInterface.h"
#include "SpriteRegisters.h"
#include "SpriteP2CDecoder.h"
#include "SpriteMerger.h"
#include "CycleExactSprites.h"
#include "VirtualHost/Core.h"
#include "Graphics.h"

using namespace CustomChipset;

bool CycleExactSprites::Is16Color(uint32_t spriteNo)
{
  uint32_t evenSpriteNo = spriteNo & 0xe;
  return SpriteState[evenSpriteNo].attached || SpriteState[evenSpriteNo + 1].attached;
}

void CycleExactSprites::Arm(uint32_t sprite_number)
{
  // Actually this is kind of wrong since each sprite serialises independently
  bool is16Color = Is16Color(sprite_number);
  if (is16Color && (sprite_number & 1) == 0) // Only arm the odd sprite
  {
    Arm(sprite_number + 1);
    SpriteState[sprite_number].armed = false;
    return;
  }
  SpriteState[sprite_number].armed = true;
  SpriteState[sprite_number].pixels_output = 0;
  SpriteState[sprite_number].dat_hold[0].w = sprite_registers.sprdata[sprite_number];
  SpriteState[sprite_number].dat_hold[1].w = sprite_registers.sprdatb[sprite_number];
  if (is16Color)
  {
    SpriteState[sprite_number].dat_hold[2].w = sprite_registers.sprdata[sprite_number - 1];
    SpriteState[sprite_number].dat_hold[3].w = sprite_registers.sprdatb[sprite_number - 1];
    SpriteP2CDecoder::Decode16(
        &(SpriteState[sprite_number].dat_decoded.blu[0].l),
        SpriteState[sprite_number].dat_hold[2].w,
        SpriteState[sprite_number].dat_hold[3].w,
        SpriteState[sprite_number].dat_hold[0].w,
        SpriteState[sprite_number].dat_hold[1].w);
  }
  else
  {
    SpriteP2CDecoder::Decode4(
        sprite_number, &(SpriteState[sprite_number].dat_decoded.blu[0].l), SpriteState[sprite_number].dat_hold[1].w, SpriteState[sprite_number].dat_hold[0].w);
  }
}

void CycleExactSprites::MergeLores(uint32_t spriteNo, uint32_t source_pixel_index, uint32_t pixel_index, uint32_t pixel_count)
{
  uint8_t *playfield = &GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield()[pixel_index];
  uint8_t *sprite_data = &SpriteState[spriteNo].dat_decoded.barray[source_pixel_index];

  SpriteMerger::MergeLores(spriteNo, playfield, sprite_data, pixel_count);
}

void CycleExactSprites::MergeHires(uint32_t spriteNo, uint32_t source_pixel_index, uint32_t pixel_index, uint32_t pixel_count)
{
  uint8_t *playfield = &GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield()[pixel_index];
  uint8_t *sprite_data = &SpriteState[spriteNo].dat_decoded.barray[source_pixel_index];

  SpriteMerger::MergeHires(spriteNo, playfield, sprite_data, pixel_count);
}

void CycleExactSprites::MergeHam(uint32_t spriteNo, uint32_t source_pixel_index, uint32_t pixel_index, uint32_t pixel_count)
{
  uint8_t *playfield = &GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield()[pixel_index];
  uint8_t *ham_sprites_playfield = &GraphicsContext.Planar2ChunkyDecoder.GetHamSpritesPlayfield()[pixel_index];
  uint8_t *sprite_data = &SpriteState[spriteNo].dat_decoded.barray[source_pixel_index];

  SpriteMerger::MergeHam(spriteNo, playfield, ham_sprites_playfield, sprite_data, pixel_count);
}

void CycleExactSprites::Merge(uint32_t spriteNo, uint32_t source_pixel_index, uint32_t pixel_index, uint32_t pixel_count)
{
  if (_core.RegisterUtility.IsLoresEnabled())
  {
    MergeLores(spriteNo, source_pixel_index, pixel_index, pixel_count);
  }
  else
  {
    MergeHires(spriteNo, source_pixel_index, pixel_index, pixel_count);
  }
}

bool CycleExactSprites::InRange(uint32_t spriteNo, uint32_t startCylinder, uint32_t cylinderCount)
{
  // Comparison happens at x, sprite image visible one cylinder later
  uint32_t visible_at_cylinder = SpriteState[spriteNo].x + 1;

  return (visible_at_cylinder >= startCylinder) && (visible_at_cylinder < (startCylinder + cylinderCount));
}

void CycleExactSprites::OutputSprite(uint32_t spriteNo, uint32_t startCylinder, uint32_t cylinderCount)
{
  if (SpriteState[spriteNo].armed)
  {
    uint32_t pixel_index = 0;

    // Look for start of sprite output
    if (!SpriteState[spriteNo].serializing && InRange(spriteNo, startCylinder, cylinderCount))
    {
      SpriteState[spriteNo].serializing = true;
      pixel_index = SpriteState[spriteNo].x + 1 - startCylinder;
    }
    if (SpriteState[spriteNo].serializing)
    {
      // Some output of the sprite will occur in this range.
      uint32_t pixel_count = cylinderCount - pixel_index;
      uint32_t pixelsLeft = 16 - SpriteState[spriteNo].pixels_output;
      if (pixel_count > pixelsLeft)
      {
        pixel_count = pixelsLeft;
      }

      if (_core.RegisterUtility.IsHiresEnabled())
      {
        pixel_index = pixel_index * 2; // Hires coordinates
      }

      Merge(spriteNo, SpriteState[spriteNo].pixels_output, pixel_index, pixel_count);
      SpriteState[spriteNo].pixels_output += pixel_count;
      SpriteState[spriteNo].serializing = (SpriteState[spriteNo].pixels_output < 16);
    }
  }
}

void CycleExactSprites::OutputSprites(uint32_t startCylinder, uint32_t cylinderCount)
{
  for (uint32_t spriteNo = 0; spriteNo < 8; spriteNo++)
  {
    OutputSprite(spriteNo, startCylinder, cylinderCount);
  }
}

/* IO Register handlers */

/* SPRXPT - $dff120 to $dff13e */

void CycleExactSprites::NotifySprpthChanged(uint16_t data, unsigned int sprite_number)
{
  // Nothing
}

void CycleExactSprites::NotifySprptlChanged(uint16_t data, unsigned int sprite_number)
{
  SpriteState[sprite_number].DMAState.state = SpriteDMAStates::SPRITE_DMA_STATE_READ_CONTROL;
}

/* SPRXPOS - $dff140 to $dff178 */

void CycleExactSprites::NotifySprposChanged(uint16_t data, unsigned int sprite_number)
{
  SpriteState[sprite_number].x = (SpriteState[sprite_number].x & 1) | ((data & 0xff) << 1);
  SpriteState[sprite_number].DMAState.y_first = (SpriteState[sprite_number].DMAState.y_first & 0x100) | ((data & 0xff00) >> 8);
  SpriteState[sprite_number].armed = false;
}

/* SPRXCTL $dff142 to $dff17a */

void CycleExactSprites::NotifySprctlChanged(uint16_t data, unsigned int sprite_number)
{
  // retrieve the rest of the horizontal and vertical position bits
  SpriteState[sprite_number].x = (SpriteState[sprite_number].x & 0x1fe) | (data & 0x1);
  SpriteState[sprite_number].DMAState.y_first = (SpriteState[sprite_number].DMAState.y_first & 0x0ff) | ((data & 0x4) << 6);

  // attach bit only applies when having an odd sprite
  if (sprite_number & 1)
  {
    SpriteState[sprite_number - 1].attached = !!(data & 0x80);
  }
  SpriteState[sprite_number].attached = !!(data & 0x80);
  SpriteState[sprite_number].DMAState.y_last = ((data & 0xff00) >> 8) | ((data & 0x2) << 7);
  SpriteState[sprite_number].armed = false;
}

/* SPRXDATA $dff144 to $dff17c */

void CycleExactSprites::NotifySprdataChanged(uint16_t data, unsigned int sprite_number)
{
  Arm(sprite_number);
}

/* SPRXDATB $dff146 to $dff17e */

void CycleExactSprites::NotifySprdatbChanged(uint16_t data, unsigned int sprite_number)
{
  SpriteState[sprite_number].armed = false;
}

/* Sprite State Machine */

uint16_t CycleExactSprites::ReadWord(uint32_t spriteNo)
{
  uint16_t data = chipmemReadWord(sprite_registers.sprpt[spriteNo]);
  sprite_registers.sprpt[spriteNo] = chipsetMaskPtr(sprite_registers.sprpt[spriteNo] + 2);
  return data;
}

void CycleExactSprites::ReadControlWords(uint32_t spriteNo)
{
  uint16_t pos = ReadWord(spriteNo);
  uint16_t ctl = ReadWord(spriteNo);
  wsprxpos(pos, 0x140 + spriteNo * 8);
  wsprxctl(ctl, 0x142 + spriteNo * 8);
}

void CycleExactSprites::ReadDataWords(uint32_t spriteNo)
{
  wsprxdatb(ReadWord(spriteNo), 0x146 + spriteNo * 8);
  wsprxdata(ReadWord(spriteNo), 0x144 + spriteNo * 8);
}

bool CycleExactSprites::IsFirstLine(uint32_t spriteNo, uint32_t rasterY)
{
  return (rasterY >= 24) && (rasterY == SpriteState[spriteNo].DMAState.y_first);
}

bool CycleExactSprites::IsAboveFirstLine(uint32_t spriteNo, uint32_t rasterY)
{
  return rasterY > SpriteState[spriteNo].DMAState.y_first;
}

bool CycleExactSprites::IsLastLine(uint32_t spriteNo, uint32_t rasterY)
{
  return rasterY == SpriteState[spriteNo].DMAState.y_last;
}

void CycleExactSprites::DMAReadControl(uint32_t spriteNo, uint32_t rasterY)
{
  ReadControlWords(spriteNo);

  if (IsFirstLine(spriteNo, rasterY))
  {
    SpriteState[spriteNo].DMAState.state = SpriteDMAStates::SPRITE_DMA_STATE_READ_DATA;
  }
  else
  {
    SpriteState[spriteNo].DMAState.state = SpriteDMAStates::SPRITE_DMA_STATE_WAITING_FOR_FIRST_LINE;
  }
}

void CycleExactSprites::DMAWaitingForFirstLine(uint32_t spriteNo, uint32_t rasterY)
{
  if (IsFirstLine(spriteNo, rasterY))
  {
    ReadDataWords(spriteNo);
    if (IsLastLine(spriteNo, rasterY))
    {
      SpriteState[spriteNo].DMAState.state = SpriteDMAStates::SPRITE_DMA_STATE_READ_CONTROL;
    }
    else
    {
      SpriteState[spriteNo].DMAState.state = SpriteDMAStates::SPRITE_DMA_STATE_READ_DATA;
    }
  }
}

void CycleExactSprites::DMAReadData(uint32_t spriteNo, uint32_t rasterY)
{
  if (!IsLastLine(spriteNo, rasterY))
  {
    ReadDataWords(spriteNo);
  }
  else
  {
    ReadControlWords(spriteNo);
    if (IsFirstLine(spriteNo, rasterY))
    {
      SpriteState[spriteNo].DMAState.state = SpriteDMAStates::SPRITE_DMA_STATE_READ_DATA;
    }
    else
    {
      SpriteState[spriteNo].DMAState.state = SpriteDMAStates::SPRITE_DMA_STATE_WAITING_FOR_FIRST_LINE;
    }
  }
}

void CycleExactSprites::DMAHandler(uint32_t rasterY)
{
  if ((dmacon & 0x20) == 0 || rasterY < 0x18)
  {
    return;
  }

  rasterY++; // Do DMA for next line

  uint32_t spriteNo = 0;
  while (spriteNo < 8)
  {
    switch (SpriteState[spriteNo].DMAState.state)
    {
      case SpriteDMAStates::SPRITE_DMA_STATE_READ_CONTROL: DMAReadControl(spriteNo, rasterY); break;
      case SpriteDMAStates::SPRITE_DMA_STATE_WAITING_FOR_FIRST_LINE: DMAWaitingForFirstLine(spriteNo, rasterY); break;
      case SpriteDMAStates::SPRITE_DMA_STATE_READ_DATA: DMAReadData(spriteNo, rasterY); break;
    }
    spriteNo++;
  }
}

/* Module management */

void CycleExactSprites::EndOfLine(uint32_t rasterY)
{
  for (uint32_t i = 0; i < 8; ++i)
  {
    SpriteState[i].serializing = false;
  }
  DMAHandler(rasterY);
}

void CycleExactSprites::EndOfFrame()
{
  for (uint32_t spriteNo = 0; spriteNo < 8; ++spriteNo)
  {
    SpriteState[spriteNo].DMAState.state = SpriteDMAStates::SPRITE_DMA_STATE_READ_CONTROL;
  }
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

CycleExactSprites::~CycleExactSprites()
{
}
