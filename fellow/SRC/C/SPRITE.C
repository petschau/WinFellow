/*=========================================================================*/
/* Fellow                                                                  */
/* Sprite emulation                                                        */
/*                                                                         */
/* Author: Petter Schau                                                    */
/*         Worfje                                                          */
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
#include "fellow.h"
#include "chipset.h"
#include "fmem.h"
#include "graph.h"
#include "draw.h"
#include "ListTree.h"
#include "bus.h"

#include "SpriteRegisters.h"
#include "SpriteP2CDecoder.h"
#include "SpriteMerger.h"
#include "LineExactSprites.h"
#include "CycleExactSprites.h"

Sprites *sprites = nullptr;
LineExactSprites *line_exact_sprites = nullptr;
CycleExactSprites *cycle_exact_sprites = nullptr;

void spriteInitializeFromEmulationMode()
{
  if (sprites != nullptr)
  {
    delete sprites;
    sprites = nullptr;
    line_exact_sprites = nullptr;
    cycle_exact_sprites = nullptr;
  }

  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    sprites = cycle_exact_sprites = new CycleExactSprites();
  }
  else
  {
    sprites = line_exact_sprites = new LineExactSprites();
  }
}

/*===========================================================================*/
/* Called on emulation hard reset                                            */
/*===========================================================================*/

void spriteHardReset()
{
  sprite_registers.ClearState();
  sprites->HardReset();
}

/*===========================================================================*/
/* Called on emulation end of line                                           */
/*===========================================================================*/

void spriteEndOfLine(ULO rasterY)
{
  sprites->EndOfLine(rasterY);
}

/*===========================================================================*/
/* Called on emulation end of frame                                          */
/*===========================================================================*/

void spriteEndOfFrame()
{
  sprites->EndOfFrame();
}

/*===========================================================================*/
/* Called on emulation start                                                 */
/*===========================================================================*/

void spriteEmulationStart()
{
  sprite_registers.InstallIOHandlers();
  sprites->EmulationStart();
}

/*===========================================================================*/
/* Called on emulation stop                                                  */
/*===========================================================================*/

void spriteEmulationStop()
{
  sprites->EmulationStop();
}

/*===========================================================================*/
/* Initializes the graphics module                                           */
/*===========================================================================*/

void spriteStartup()
{
  sprite_registers.ClearState();
  spriteInitializeFromEmulationMode();
  SpriteP2CDecoder::Initialize();
  SpriteMerger::Initialize();
}

/*===========================================================================*/
/* Release resources taken by the sprites module                             */
/*===========================================================================*/

void spriteShutdown()
{
  if (sprites != nullptr)
  {
    delete sprites;
    sprites = nullptr;
  }
}
