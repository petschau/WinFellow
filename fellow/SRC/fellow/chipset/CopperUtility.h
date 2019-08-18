/*=========================================================================*/
/* Fellow                                                                  */
/* Copper utility functions                                                */
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
#include "fellow/chipset/CopperRegisters.h"
#include "fellow/chipset/ChipsetInfo.h"

class CopperUtility
{
public:
  static bool IsMove(const UWO ir1)
  {
    return (ir1 & 1) == 0;
  }
  static bool IsWait(const UWO ir2)
  {
    return (ir2 & 1) == 0;
  }
  static bool IsSkip(const UWO ir2)
  {
    return (ir2 & 1) == 1;
  }
  static bool HasBlitterFinishedDisable(const UWO ir2)
  {
    return (ir2 & 0x8000) != 0;
  }
  static ULO GetRegisterNumber(const UWO ir1)
  {
    return (ULO)(ir1 & 0x1fe);
  }

  static bool IsRegisterAllowed(const ULO regno)
  {
    if (chipsetGetECS())
    {
      return (regno >= 0x40) || (copper_registers.copcon != 0);
    }

    // OCS
    return (regno >= 0x80) || ((regno >= 0x40) && (copper_registers.copcon != 0));
  }
};
