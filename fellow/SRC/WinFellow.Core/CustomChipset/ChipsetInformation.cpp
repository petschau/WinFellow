/*=========================================================================*/
/* Fellow                                                                  */
/* ChipsetInformation.cpp                                                  */
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

#include "CustomChipset/ChipsetInformation.h"

namespace CustomChipset
{
  bool ChipsetInformation::GetIsEcs()
  {
    return _isEcs;
  }

  void ChipsetInformation::UpdateAddressMasksFromConfiguration()
  {
    if (GetIsEcs())
    {
      _pointerMask = 0x1ffffe;
      _addressMask = 0x1fffff;
    }
    else // OCS
    {
      _pointerMask = 0x7fffe;
      _addressMask = 0x7ffff;
    }
  }

  bool ChipsetInformation::SetIsEcs(bool isEcs)
  {
    if (_isEcs == isEcs) return false;

    _isEcs = isEcs;
    UpdateAddressMasksFromConfiguration();
    return true;
  }

  uint32_t ChipsetInformation::MaskAddress(uint32_t address)
  {
    return address & _addressMask;
  }

  uint32_t ChipsetInformation::MaskPointer(uint32_t pointer)
  {
    return pointer & _pointerMask;
  }

  uint32_t ChipsetInformation::ReplaceLowPointer(uint32_t originalPointer, uint16_t newLowPart)
  {
    return MaskPointer((originalPointer & 0xffff0000) | (((uint32_t)newLowPart) & 0xfffe));
  }

  uint32_t ChipsetInformation::ReplaceHighPointer(uint32_t originalPointer, uint16_t newHighPart)
  {
    return MaskPointer((originalPointer & 0xfffe) | (((uint32_t)newHighPart) << 16));
  }

  void ChipsetInformation::Startup()
  {
    _isEcs = false;
    _isCycleExact = false;
    UpdateAddressMasksFromConfiguration();
  }
}