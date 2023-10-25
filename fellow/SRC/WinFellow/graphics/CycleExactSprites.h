#pragma once

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
#include "SPRITE.H"
#include "VirtualHost/Core.h"

typedef enum SpriteDMAStates_
{
  SPRITE_DMA_STATE_READ_CONTROL = 1,
  SPRITE_DMA_STATE_WAITING_FOR_FIRST_LINE = 2,
  SPRITE_DMA_STATE_READ_DATA = 3,
  SPRITE_DMA_STATE_DISABLED = 4
} SpriteDMAStates;

typedef union SpriteDecodedUnion_ {
  uint8_t barray[16];
  ByteLongUnion blu[4];
} SpriteDecodedUnion;

typedef struct SpriteDMAStateMachine_
{
  SpriteDMAStates state;
  uint32_t y_first;
  uint32_t y_last;
} SpriteDMAStateMachine;

typedef struct Sprite_
{
  SpriteDMAStateMachine DMAState;
  bool armed;
  bool attached;
  uint32_t x;
  ByteWordUnion dat_hold[4];      // Sprite image "armed" for output at x.
  SpriteDecodedUnion dat_decoded; // The armed image, decoded
  bool serializing;
  uint32_t pixels_output;
} Sprite;

class CycleExactSprites : public Sprites
{
private:
  Sprite SpriteState[8];

  void Arm(uint32_t spriteNo);

  void MergeLores(uint32_t spriteNo, uint32_t source_pixel_index, uint32_t pixel_index, uint32_t pixel_count);
  void MergeHires(uint32_t spriteNo, uint32_t source_pixel_index, uint32_t pixel_index, uint32_t pixel_count);
  void MergeHam(uint32_t spriteNo, uint32_t source_pixel_index, uint32_t pixel_index, uint32_t pixel_count);
  void Merge(uint32_t spriteNo, uint32_t source_pixel_index, uint32_t pixel_index, uint32_t pixel_count);
  bool InRange(uint32_t spriteNo, uint32_t startCylinder, uint32_t cylinderCount);
  uint16_t ReadWord(uint32_t spriteNo);
  void ReadControlWords(uint32_t spriteNo);
  void ReadDataWords(uint32_t spriteNo);
  bool IsFirstLine(uint32_t spriteNo, uint32_t rasterY);
  bool IsAboveFirstLine(uint32_t spriteNo, uint32_t rasterY);
  bool IsLastLine(uint32_t spriteNo, uint32_t rasterY);
  bool Is16Color(uint32_t spriteNo);

  void DMAReadControl(uint32_t spriteNo, uint32_t rasterY);
  void DMAReadData(uint32_t spriteNo, uint32_t rasterY);
  void DMAWaitingForFirstLine(uint32_t spriteNo, uint32_t rasterY);
  void DMAHandler(uint32_t rasterY);

  void ClearState();

  void OutputSprite(uint32_t spriteNo, uint32_t startCylinder, uint32_t cylinderCount);

public:
  virtual void NotifySprpthChanged(uint16_t data, unsigned int sprite_number);
  virtual void NotifySprptlChanged(uint16_t data, unsigned int sprite_number);
  virtual void NotifySprposChanged(uint16_t data, unsigned int sprite_number);
  virtual void NotifySprctlChanged(uint16_t data, unsigned int sprite_number);
  virtual void NotifySprdataChanged(uint16_t data, unsigned int sprite_number);
  virtual void NotifySprdatbChanged(uint16_t data, unsigned int sprite_number);

  void OutputSprites(uint32_t startCylinder, uint32_t cylinderCount);

  virtual void HardReset();
  virtual void EndOfLine(uint32_t rasterY);
  virtual void EndOfFrame();
  virtual void EmulationStart();
  virtual void EmulationStop();

  CycleExactSprites();
  virtual ~CycleExactSprites();
};
