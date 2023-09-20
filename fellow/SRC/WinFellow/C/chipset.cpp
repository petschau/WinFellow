/*=========================================================================*/
/* Fellow                                                                  */
/* chipset.cpp                                                             */
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
#include "chipset.h"

chipset_information chipset;

bool chipsetGetECS()
{
  return chipset.ecs;
}

void chipsetUpdateAddressMasks()
{
  if (chipsetGetECS())
  {
    chipset.ptr_mask = 0x1ffffe;
    chipset.address_mask = 0x1fffff;
  }
  else // OCS
  {
    chipset.ptr_mask = 0x7fffe;
    chipset.address_mask = 0x7ffff;
  }
}

BOOLE chipsetSetECS(bool ecs)
{
  BOOLE needreset = (chipset.ecs != ecs);
  chipset.ecs = ecs;
  chipsetUpdateAddressMasks();
  return needreset;
}

void chipsetStartup()
{
  chipsetSetECS(false);
}
