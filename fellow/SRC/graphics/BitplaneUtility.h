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

#ifndef BITPLANEUTILITY_H
#define BITPLANEUTILITY_H

#include "DEFS.H"
#include "GRAPH.H"

// Bitplane data types
typedef union ByteLongUnion_
{
  ULO l;
  UBY b[4];
} ByteLongUnion;

typedef union ByteWordUnion_
{
  UWO w;
  UBY b[2];
} ByteWordUnion;

class BitplaneUtility
{
public:
  static bool IsLores(void) {return (bplcon0 & 0x8000) == 0;}
  static bool IsHires(void) {return !IsLores();}
  static bool IsDualPlayfield(void) {return (bplcon0 & 0x0400) == 0x0400;}
  static bool IsHam(void) {return (bplcon0 & 0x0800) == 0x0800;}
  static bool IsPlayfield1Pri(void) {return (bplcon2 & 0x0040) == 0;}
  static bool IsDMAEnabled(void) {return ((dmaconr & 0x0300) == 0x0300);}
  static ULO GetEnabledBitplaneCount(void) {return (bplcon0 >> 12) & 7;}
};

#endif
