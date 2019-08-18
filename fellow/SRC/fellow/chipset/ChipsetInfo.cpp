/*=========================================================================*/
/* Fellow                                                                  */
/* Chipset.cpp                                                             */
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

#include "fellow/chipset/ChipsetInfo.h"

#include "SPRITE.H"

chipset_information chipset_info;

bool chipsetGetECS()
{
  return chipset_info.ecs;
}

void chipsetUpdateAddressMasks()
{
  if (chipsetGetECS())
  {
    chipset_info.ptr_mask = 0x1ffffe;
    chipset_info.address_mask = 0x1fffff;
  }
  else // OCS
  {
    chipset_info.ptr_mask = 0x7fffe;
    chipset_info.address_mask = 0x7ffff;
  }
}

bool chipsetSetECS(bool ecs)
{
  bool needreset = (chipset_info.ecs != ecs);
  chipset_info.ecs = ecs;
  chipsetUpdateAddressMasks();
  chipset_info.DDFMask = ecs ? 0xfe : 0xfc;
  return needreset;
}

bool chipsetSetIsCycleExact(bool isCycleExact)
{
  bool needReset = chipset_info.IsCycleExact != isCycleExact;
  chipset_info.IsCycleExact = isCycleExact;
  return needReset;
}

bool chipsetIsCycleExact()
{
  return chipset_info.IsCycleExact;
}

void chipsetSetDDFMask(const uint16_t ddfMask)
{
  chipset_info.DDFMask = ddfMask;
}

uint16_t chipsetGetDDFMask()
{
  return chipset_info.DDFMask;
}

void chipsetSetMaximumDDF(const uint16_t maximumDDF)
{
  chipset_info.MaximumDDF = maximumDDF;
}

uint16_t chipsetGetMaximumDDF()
{
  return chipset_info.MaximumDDF;
}

void chipsetStartup()
{
  chipsetSetECS(false);
  // chipsetSetMinimumDDF(0x18);
  chipsetSetMaximumDDF(0xd8);
}
