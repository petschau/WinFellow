#pragma once

/*=========================================================================*/
/* Fellow                                                                  */
/* Register utility functions                                              */
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

#include <cstdint>

#include "CustomChipset/Registers.h"

// Bitplane data types
typedef union ByteLongUnion_ {
  uint32_t l;
  uint8_t b[4];
} ByteLongUnion;

typedef union ByteWordUnion_ {
  uint16_t w;
  uint8_t b[2];
} ByteWordUnion;

namespace CustomChipset
{
  class RegisterUtility
  {
  private:
    const Registers &_registers;

  public:
    bool IsLoresEnabled() const;
    bool IsHiresEnabled() const;
    bool IsDualPlayfieldEnabled() const;
    bool IsHAMEnabled() const;
    bool IsInterlaceEnabled() const;
    unsigned int GetEnabledBitplaneCount() const;

    bool IsPlayfield1PriorityEnabled() const;
    bool IsPlayfield2PriorityEnabled() const;

    bool IsMasterDMAEnabled() const;
    bool IsMasterDMAAndBitplaneDMAEnabled() const;
    bool IsDiskDMAEnabled() const;
    bool IsBlitterPriorityEnabled() const;

    RegisterUtility(const Registers &registers);
  };
}