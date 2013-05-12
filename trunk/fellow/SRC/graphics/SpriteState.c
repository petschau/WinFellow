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

#include "DEFS.H"

#ifdef GRAPH2

#include "bus.h"
#include "graph.h"
#include "draw.h"
#include "fmem.h"
#include "fileops.h"
#include "sprite.h"

#include "BitplaneUtility.h"
#include "Graphics.h"

typedef union sprite_deco_
{
  UBY i8[8];
  ULO i32[2];
} sprite_deco;

extern sprite_deco sprite_deco4[4][2][256];
extern sprite_deco sprite_deco16[4][256];
extern UBY sprite_translate[2][256][256];

void Sprites::Decode4(ULO spriteNo)
{
  ULO sprite_class = spriteNo / 2;
  ByteLongUnion *dat_decoded = SpriteState[spriteNo].dat_decoded.blu;
  ByteWordUnion *dat1 = &SpriteState[spriteNo].dat_hold[0];
  ByteWordUnion *dat2 = &SpriteState[spriteNo].dat_hold[1];
  UBY planardata;

  planardata = dat1->b[1];
  dat_decoded[0].l = sprite_deco4[sprite_class][0][planardata].i32[0];
  dat_decoded[1].l = sprite_deco4[sprite_class][0][planardata].i32[1];

  planardata = dat1->b[0];
  dat_decoded[2].l = sprite_deco4[sprite_class][0][planardata].i32[0];
  dat_decoded[3].l = sprite_deco4[sprite_class][0][planardata].i32[1];

  planardata = dat2->b[1];
  dat_decoded[0].l |= sprite_deco4[sprite_class][1][planardata].i32[0];
  dat_decoded[1].l |= sprite_deco4[sprite_class][1][planardata].i32[1];

  planardata = dat2->b[0];
  dat_decoded[2].l |= sprite_deco4[sprite_class][1][planardata].i32[0];
  dat_decoded[3].l |= sprite_deco4[sprite_class][1][planardata].i32[1];
}

void Sprites::Decode16(ULO spriteNo)
{
  ByteLongUnion *dat_decoded = SpriteState[spriteNo].dat_decoded.blu;
  ByteWordUnion *dat1 = &SpriteState[spriteNo].dat_hold[2];
  ByteWordUnion *dat2 = &SpriteState[spriteNo].dat_hold[3];
  ByteWordUnion *dat3 = &SpriteState[spriteNo].dat_hold[0];
  ByteWordUnion *dat4 = &SpriteState[spriteNo].dat_hold[1];
  UBY planardata;

  planardata = dat1->b[1];
  dat_decoded[0].l = sprite_deco16[0][planardata].i32[0];
  dat_decoded[1].l = sprite_deco16[0][planardata].i32[1];

  planardata = dat1->b[0];
  dat_decoded[2].l = sprite_deco16[0][planardata].i32[0];
  dat_decoded[3].l = sprite_deco16[0][planardata].i32[1];

  planardata = dat2->b[1];
  dat_decoded[0].l |= sprite_deco16[1][planardata].i32[0];
  dat_decoded[1].l |= sprite_deco16[1][planardata].i32[1];

  planardata = dat2->b[0];
  dat_decoded[2].l |= sprite_deco16[1][planardata].i32[0];
  dat_decoded[3].l |= sprite_deco16[1][planardata].i32[1];

  planardata = dat3->b[1];
  dat_decoded[0].l |= sprite_deco16[2][planardata].i32[0];
  dat_decoded[1].l |= sprite_deco16[2][planardata].i32[1];

  planardata = dat3->b[0];
  dat_decoded[2].l |= sprite_deco16[2][planardata].i32[0];
  dat_decoded[3].l |= sprite_deco16[2][planardata].i32[1];

  planardata = dat4->b[1];
  dat_decoded[0].l |= sprite_deco16[3][planardata].i32[0];
  dat_decoded[1].l |= sprite_deco16[3][planardata].i32[1];

  planardata = dat4->b[0];
  dat_decoded[2].l |= sprite_deco16[3][planardata].i32[0];
  dat_decoded[3].l |= sprite_deco16[3][planardata].i32[1];
}

bool Sprites::Is16Color(ULO spriteNo)
{
  ULO evenSpriteNo = spriteNo & 0xe;
  return SpriteState[evenSpriteNo].attached || SpriteState[evenSpriteNo + 1].attached;
}

void Sprites::Arm(ULO spriteNo)
{
  // Actually this is kind of wrong since each sprite serialises independently
  bool is16Color = Is16Color(spriteNo);
  if (is16Color && (spriteNo & 1) == 0) // Only arm the odd sprite
  {
    Arm(spriteNo + 1);
    SpriteState[spriteNo].armed = false;
    return;
  }
  SpriteState[spriteNo].armed = true;
  SpriteState[spriteNo].x_cylinder = SpriteState[spriteNo].x;
  SpriteState[spriteNo].pixels_output = 0;
  SpriteState[spriteNo].dat_hold[0].w = SpriteState[spriteNo].dat[0];
  SpriteState[spriteNo].dat_hold[1].w = SpriteState[spriteNo].dat[1];
  if (is16Color)
  {
    SpriteState[spriteNo].dat_hold[2].w = SpriteState[spriteNo - 1].dat[0];
    SpriteState[spriteNo].dat_hold[3].w = SpriteState[spriteNo - 1].dat[1];
    Decode16(spriteNo);
  }
  else
  {
    Decode4(spriteNo);
  }
}

void Sprites::MergeLores(ULO spriteNo, ULO source_pixel_index, ULO pixel_index, ULO pixel_count)
{
  UBY *playfield = &GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield()[pixel_index];
  UBY *sprite_data = &SpriteState[spriteNo].dat_decoded.barray[source_pixel_index];
  ULO in_front = ((bplcon2 & 0x38) > (4 * spriteNo)) ? 1 : 0;

  for (ULO i = 0; i < pixel_count; ++i)
  {
    *playfield++ = sprite_translate[in_front][*playfield][*sprite_data++];
  }
}

void Sprites::MergeHires(ULO spriteNo, ULO source_pixel_index, ULO pixel_index, ULO pixel_count)
{
  UBY *playfield = &GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield()[pixel_index];
  UBY *sprite_data = &SpriteState[spriteNo].dat_decoded.barray[source_pixel_index];
  ULO in_front = ((bplcon2 & 0x38) > (4 * spriteNo)) ? 1 : 0;
  UBY pixel_color;

  for (ULO i = 0; i < pixel_count; ++i)
  {
    pixel_color = sprite_translate[in_front][*playfield][*sprite_data++];
    *playfield++ = pixel_color;
    *playfield++ = pixel_color;
  }
}

void Sprites::MergeHam(ULO spriteNo, ULO source_pixel_index, ULO pixel_index, ULO pixel_count)
{
  UBY *playfield = &GraphicsContext.Planar2ChunkyDecoder.GetOddPlayfield()[pixel_index];
  UBY *ham_sprites_playfield = &GraphicsContext.Planar2ChunkyDecoder.GetHamSpritesPlayfield()[pixel_index];
  UBY *sprite_data = &SpriteState[spriteNo].dat_decoded.barray[source_pixel_index];
  ULO in_front = ((bplcon2 & 0x38) > (4 * spriteNo)) ? 1 : 0;

  for (ULO i = 0; i < pixel_count; ++i)
  {
    *ham_sprites_playfield++ = sprite_translate[in_front][*playfield][*sprite_data++];
  }
}

void Sprites::Merge(ULO spriteNo, ULO source_pixel_index, ULO pixel_index, ULO pixel_count)
{
  if (BitplaneUtility::IsLores())
  {
    MergeLores(spriteNo, source_pixel_index, pixel_index, pixel_count);
  }
  else
  {
    MergeHires(spriteNo, source_pixel_index, pixel_index, pixel_count);
  }
}

bool Sprites::InRange(ULO spriteNo, ULO startCylinder, ULO cylinderCount)
{
  return (SpriteState[spriteNo].x_cylinder >= startCylinder)
         && (SpriteState[spriteNo].x_cylinder < (startCylinder + cylinderCount));
}

void Sprites::OutputSprite(ULO spriteNo, ULO startCylinder, ULO cylinderCount)
{
  if (SpriteState[spriteNo].armed)
  {
    ULO pixel_index = 0;

    // Look for start of sprite output
    if (!SpriteState[spriteNo].serializing && InRange(spriteNo, startCylinder, cylinderCount))
    {
      SpriteState[spriteNo].serializing = true;
      pixel_index = SpriteState[spriteNo].x - startCylinder + 1;
    }
    if (SpriteState[spriteNo].serializing)
    {
      // Some output of the sprite will occur in this range.
      ULO pixel_count = cylinderCount - pixel_index;
      ULO pixelsLeft = 16 - SpriteState[spriteNo].pixels_output;
      if (pixel_count > pixelsLeft)
      {
        pixel_count = pixelsLeft;
      }

      if (BitplaneUtility::IsHires())
      {
        pixel_index *= 2;  // Hires coordinates
      }

      Merge(spriteNo, SpriteState[spriteNo].pixels_output, pixel_index, pixel_count);
      SpriteState[spriteNo].pixels_output += pixel_count;	
      SpriteState[spriteNo].serializing = (SpriteState[spriteNo].pixels_output < 16);
    }
  }
}

void Sprites::OutputSprites(ULO startCylinder, ULO cylinderCount)
{
  for (ULO spriteNo = 0; spriteNo < 8; spriteNo++)
  {
    OutputSprite(spriteNo, startCylinder, cylinderCount);
  }
}

/* IO Register handlers */

/* SPRXPT - $dff120 to $dff13e */

void Sprite_wsprxpth(UWO data, ULO address)
{
  GraphicsContext.Sprites.wsprxpth(data, address);
}

void Sprites::wsprxpth(UWO data, ULO address)
{
  ULO spriteNo = (address >> 2) & 7;

  SpriteState[spriteNo].DMAState.pt = ((data & 0x1f) << 16) | (SpriteState[spriteNo].DMAState.pt & 0xfffe);
}

void Sprite_wsprxptl(UWO data, ULO address)
{
  GraphicsContext.Sprites.wsprxptl(data, address);
}

void Sprites::wsprxptl(UWO data, ULO address)
{
  ULO spriteNo = (address >> 2) & 7;

  SpriteState[spriteNo].DMAState.pt = (data & 0xfffe) | (SpriteState[spriteNo].DMAState.pt & 0x1f0000);
  SpriteState[spriteNo].DMAState.state = SPRITE_DMA_STATE_READ_CONTROL;
}

/* SPRXPOS - $dff140 to $dff178 */

void Sprite_wsprxpos(UWO data, ULO address)
{
  GraphicsContext.Sprites.wsprxpos(data, address);
}

void Sprites::wsprxpos(UWO data, ULO address) 
{
  ULO spriteNo = (address >> 3) & 7;

  SpriteState[spriteNo].x = (SpriteState[spriteNo].x & 1) | ((data & 0xff) << 1);
  SpriteState[spriteNo].DMAState.y_first = (SpriteState[spriteNo].DMAState.y_first & 0x100) | ((data & 0xff00) >> 8);  
  SpriteState[spriteNo].armed = false;
}

/* SPRXCTL $dff142 to $dff17a */

void Sprite_wsprxctl(UWO data, ULO address)
{
  GraphicsContext.Sprites.wsprxctl(data, address);
}

void Sprites::wsprxctl(UWO data, ULO address)
{
  ULO spriteNo = (address >> 3) & 7;

  // retrieve the rest of the horizontal and vertical position bits
  SpriteState[spriteNo].x = (SpriteState[spriteNo].x & 0x1fe) | (data & 0x1);
  SpriteState[spriteNo].DMAState.y_first = (SpriteState[spriteNo].DMAState.y_first & 0x0ff) | ((data & 0x4) << 6);

  // attach bit only applies when having an odd sprite
  if (spriteNo & 1) 
  {
    SpriteState[spriteNo - 1].attached = !!(data & 0x80);
  }
  SpriteState[spriteNo].attached = !!(data & 0x80);
  SpriteState[spriteNo].DMAState.y_last = ((data & 0xff00) >> 8) | ((data & 0x2) << 7);
  SpriteState[spriteNo].armed = false;
}

/* SPRXDATA $dff144 to $dff17c */

void Sprite_wsprxdata(UWO data, ULO address)
{
  GraphicsContext.Sprites.wsprxdata(data, address);
}

void Sprites::wsprxdata(UWO data, ULO address)
{
  ULO spriteNo = (address >> 3) & 7;
  SpriteState[spriteNo].dat[1] = data;
  Arm(spriteNo);
}

void Sprite_wsprxdatb(UWO data, ULO address)
{
  GraphicsContext.Sprites.wsprxdatb(data, address);
}

void Sprites::wsprxdatb(UWO data, ULO address)
{
  ULO spriteNo = (address >> 3) & 7;
  SpriteState[spriteNo].dat[0] = data; 
  SpriteState[spriteNo].armed = false;
}

/* Sprite State Machine */

UWO Sprites::ReadWord(ULO spriteNo)
{
  UWO data = ((UWO) (memory_chip[SpriteState[spriteNo].DMAState.pt] << 8) | (UWO) memory_chip[SpriteState[spriteNo].DMAState.pt + 1]);
  SpriteState[spriteNo].DMAState.pt = (SpriteState[spriteNo].DMAState.pt + 2) & 0x1ffffe;
  return data;
}

void Sprites::ReadControlWords(ULO spriteNo)
{
  UWO pos = ReadWord(spriteNo);
  UWO ctl = ReadWord(spriteNo);
  wsprxpos(pos, 0x140 + spriteNo*8);
  wsprxctl(ctl, 0x142 + spriteNo*8);
}

void Sprites::ReadDataWords(ULO spriteNo)
{
  wsprxdatb(ReadWord(spriteNo), 0x146 + spriteNo*8);
  wsprxdata(ReadWord(spriteNo), 0x144 + spriteNo*8);
}

bool Sprites::IsFirstLine(ULO spriteNo, ULO rasterY)
{
  return (rasterY >= 24) && (rasterY == SpriteState[spriteNo].DMAState.y_first);
}

bool Sprites::IsAboveFirstLine(ULO spriteNo, ULO rasterY)
{
  return rasterY > SpriteState[spriteNo].DMAState.y_first;
}

bool Sprites::IsLastLine(ULO spriteNo, ULO rasterY)
{
  return rasterY == SpriteState[spriteNo].DMAState.y_last;
}

void Sprites::DMAReadControl(ULO spriteNo, ULO rasterY)
{
  ReadControlWords(spriteNo);

  if (IsFirstLine(spriteNo, rasterY))
  {
    SpriteState[spriteNo].DMAState.state = SPRITE_DMA_STATE_READ_DATA;	
  }
  else
  {
    SpriteState[spriteNo].DMAState.state = SPRITE_DMA_STATE_WAITING_FOR_FIRST_LINE;
  }
}

void Sprites::DMAWaitingForFirstLine(ULO spriteNo, ULO rasterY)
{
  if (IsFirstLine(spriteNo, rasterY))
  {
    ReadDataWords(spriteNo);
    if (IsLastLine(spriteNo, rasterY))
    {
      SpriteState[spriteNo].DMAState.state = SPRITE_DMA_STATE_READ_CONTROL;
    }
    else
    {
      SpriteState[spriteNo].DMAState.state = SPRITE_DMA_STATE_READ_DATA;
    }
  }
}

void Sprites::DMAReadData(ULO spriteNo, ULO rasterY)
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
      SpriteState[spriteNo].DMAState.state = SPRITE_DMA_STATE_READ_DATA;
    }
    else
    {
      SpriteState[spriteNo].DMAState.state = SPRITE_DMA_STATE_WAITING_FOR_FIRST_LINE;
    }
  }
}

void Sprites::DMAHandler(ULO rasterY)
{
  if ((dmacon & 0x20) == 0 || rasterY < 0x18)
  {
    return;
  }

  ULO spriteNo = 0;
  while (spriteNo < 8) 
  {
    switch(SpriteState[spriteNo].DMAState.state) 
    {
      case SPRITE_DMA_STATE_READ_CONTROL:
	DMAReadControl(spriteNo, rasterY);
	break;
      case SPRITE_DMA_STATE_WAITING_FOR_FIRST_LINE:
	DMAWaitingForFirstLine(spriteNo, rasterY);
        break;
      case SPRITE_DMA_STATE_READ_DATA:
	DMAReadData(spriteNo, rasterY);
  	break;
    }
    spriteNo++;
  }
}

/* Module management */

void Sprites::EndOfLine(ULO rasterY)
{
  for (ULO i = 0; i < 8; ++i)
  {
    SpriteState[i].serializing = false;
  }
  DMAHandler(rasterY);
}

void Sprites::EndOfFrame(void)
{
  for (ULO spriteNo = 0; spriteNo < 8; ++spriteNo)
  {
    SpriteState[spriteNo].DMAState.state = SPRITE_DMA_STATE_READ_CONTROL;
  }
}

void Sprites::IOHandlersInstall(void)
{
  for (ULO i = 0; i < 8; i++)
  {
    memorySetIoWriteStub(0x120 + i*4, Sprite_wsprxpth);
    memorySetIoWriteStub(0x122 + i*4, Sprite_wsprxptl);
    memorySetIoWriteStub(0x140 + i*8, Sprite_wsprxpos);
    memorySetIoWriteStub(0x142 + i*8, Sprite_wsprxctl);
    memorySetIoWriteStub(0x144 + i*8, Sprite_wsprxdata);
    memorySetIoWriteStub(0x146 + i*8, Sprite_wsprxdatb);
  }
}

void Sprites::ClearState(void)
{
  memset(SpriteState, 0, sizeof(Sprite)*8);
}

void Sprites::EmulationStart(void)
{
  IOHandlersInstall();
  ClearState();
}

#endif
