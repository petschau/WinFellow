/*=========================================================================*/
/* Fellow                                                                  */
/* Bitplane utility functions                                              */
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
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/Graphics.h"

// Bitplane data types
typedef union ByteLongUnion_ {
  ULO l;
  UBY b[4];
} ByteLongUnion;

typedef union ByteWordUnion_ {
  UWO w;
  UBY b[2];
} ByteWordUnion;

class BitplaneUtility
{
public:
  static bool IsLores()
  {
    return bitplane_registers.IsLores;
  }
  static bool IsHires()
  {
    return bitplane_registers.IsHires;
  }
  static bool IsDualPlayfield()
  {
    return bitplane_registers.IsDualPlayfield;
  }
  static bool IsHam()
  {
    return bitplane_registers.IsHam;
  }
  static bool IsPlayfield1Pri()
  {
    return (bitplane_registers.bplcon2 & 0x0040) == 0;
  }
  static ULO GetEnabledBitplaneCount()
  {
    return bitplane_registers.EnabledBitplaneCount;
  }
  static ULO GetEvenScrollMask()
  {
    return bitplane_registers.bplcon1 & 0xf;
  }
  static ULO GetOddScrollMask()
  {
    return (bitplane_registers.bplcon1 >> 4) & 0xf;
  }
  //  static UWO GetDIWStartHorisontalOCS() { return bitplane_registers.diwstrt & 0xff; }
  //  static UWO GetDIWStopHorisontalOCS() { return (bitplane_registers.diwstop & 0xff) | 0x100; }
  static UWO GetDIWXStart()
  {
    return bitplane_registers.DiwXStart;
  }
  static UWO GetDIWXStop()
  {
    return bitplane_registers.DiwXStop;
  }
  static UWO GetDIWYStart()
  {
    return bitplane_registers.DiwYStart;
  }
  static UWO GetDIWYStop()
  {
    return bitplane_registers.DiwYStop;
  }

  static bool IsBitplaneDMAEnabled()
  {
    return (dmaconr & 0x0300) == 0x0300;
  }
  static bool IsSpriteDMAEnabled()
  {
    return (dmaconr & 0x0220) == 0x0220;
  }
  static bool IsCopperDMAEnabled()
  {
    return (dmaconr & 0x0280) == 0x0280;
  }
};
