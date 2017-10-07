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

#include "DEFS.H"
#include "BitplaneSerializerState.h"

void BitplaneSerializerState::Commit(UWO dat1, UWO dat2, UWO dat3, UWO dat4, UWO dat5, UWO dat6)
{
  Activated = true;
  Pending[0] = dat1;
  Pending[1] = dat2;
  Pending[2] = dat3;
  Pending[3] = dat4;
  Pending[4] = dat5;
  Pending[5] = dat6;
}

void BitplaneSerializerState::ShiftActive(unsigned int pixelCount)
{
  for (unsigned int i = 0; i < 6; i++)
  {
    Active[i].w <<= pixelCount;
  }
}

void BitplaneSerializerState::CopyPendingToActive(unsigned int playfieldNumber)
{
  for (unsigned int i = playfieldNumber; i < 6; i += 2)
  {
    Active[i].w = Pending[i];
    Pending[i] = 0;
  }
}

void BitplaneSerializerState::ClearActiveAndPendingWords()
{
  for (unsigned int i = 0; i < 6; ++i)
  {
    Active[i].w = 0;
    Pending[i] = 0;
  }
}

void BitplaneSerializerState::InitializeForNewLine(unsigned int lineNumber)
{
  Activated = false;
  CurrentLineNumber = lineNumber;
  CurrentCylinder = MinimumCylinder;
  ClearActiveAndPendingWords();
}

BitplaneSerializerState::BitplaneSerializerState()
  : MinimumCylinder(56),
  MaximumCylinder(25)
{
  InitializeForNewLine(0);
}
