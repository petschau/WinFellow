/*=========================================================================*/
/* Fellow                                                                  */
/* Bitplane serializer state                                               */
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

#ifndef BITPLANESERIALIZERSTATE_H
#define BITPLANESERIALIZERSTATE_H

#include "DEFS.H"
#include "BitplaneUtility.h"
#include "BitplaneSerializerState.h"

// State and dumb data operations - no chipset logic
struct BitplaneSerializerState
{
  ByteWordUnion Active[6];
  UWO Pending[6];
  unsigned int CurrentLineNumber;
  unsigned int CurrentCylinder;
  const unsigned int MinimumCylinder = 56;
  const unsigned int MaximumCylinder = 25;
  bool Activated;

  void Commit(UWO dat1, UWO dat2, UWO dat3, UWO dat4, UWO dat5, UWO dat6);
  void ShiftActive(unsigned int pixelCount);
  void InitializeForNewLine(unsigned int lineNumber);
  void CopyPendingToActive(unsigned int playfieldNumber);
  void ClearActiveAndPendingWords();
  BitplaneSerializerState();
};

#endif
